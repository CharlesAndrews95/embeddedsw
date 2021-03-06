/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of Mentor Graphics Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************************
* This is a sample demonstration application that showcases usage of remoteproc
* and rpmsg APIs on the remote core. This application is meant to run on the remote CPU
* running FreeRTOS code. It receives two matrices from the master,
* multiplies them and returns the result to the master core.
*
* The init_system is called in the main function which defines a shared memory region in
* MPU settings for the communication between master and remote using
* zynqMP_r5_map_mem_region API,it also initializes interrupt controller
* GIC and register the interrupt service routine for IPI using
* zynqMP_r5_gic_initialize API.
*
* The remoteproc_resource_init API is being called to create the virtio/RPMsg devices
* required for IPC with the master context. Invocation of this API causes remoteproc on
* the FreeRTOS to use the rpmsg name service announcement feature to advertise the
* rpmsg channels served by the application.
*
* The master receives the advertisement messages and performs the following tasks:
* 	1. Invokes the channel created callback registered by the master application
* 	2. Responds to remote context with a name service acknowledgement message
* After the acknowledgement is received from master, remoteproc on the FreeRTOS
* invokes the RPMsg channel-created callback registered by the remote application.
* The RPMsg channel is established at this point. All RPMsg APIs can be used subsequently
* on both sides for run time communications between the master and remote software contexts.
*
* Upon running the master application to send data to remote core, master will
* generate the matrices and send to remote (FreeRTOS) by informing the FreeRTOS with
* an IPI, the remote will compute the resulting matrix and send the data back to
* master. Once the application is ran and task by the
* FreeRTOS application is done, master needs to properly shut down the remote
* processor
*
* To shut down the remote processor, the following steps are performed:
* 	1. The master application sends an application-specific shutdown message
* 	   to the remote context
* 	2. This FreeRTOS application cleans up application resources,
* 	   sends a shutdown acknowledge to master, and invokes remoteproc_resource_deinit
* 	   API to de-initialize remoteproc on the FreeRTOS side.
* 	3. On receiving the shutdown acknowledge message, the master application invokes
* 	   the remoteproc_shutdown API to shut down the remote processor and de-initialize
* 	   remoteproc using remoteproc_deinit on its side.
*
**************************************************************************************/

#include "xil_printf.h"
#include "xil_exception.h"
#include "rsc_table.h"

#include "FreeRTOS.h"
#include "task.h"

#define SHUTDOWN_MSG	0xEF56A55A

#define	MAX_SIZE		6
#define NUM_MATRIX      2

typedef struct _matrix {
	unsigned int size;
	unsigned int elements[MAX_SIZE][MAX_SIZE];
} matrix;


/* Global variables */
extern const struct remote_resource_table resources;

/* from helper.c */
extern int init_system(void);

/* from system_helper.c */
extern void buffer_create(void);
extern int buffer_push(void *data, int len);
extern void buffer_pull(void **data, int *len);

/* Local variables */
static struct rpmsg_channel *app_rp_chnl;
static struct rpmsg_endpoint *rp_ept;
static struct remote_proc *proc = NULL;
static struct rsc_table_info rsc_info;
static TaskHandle_t comm_task;
static matrix matrix_array[NUM_MATRIX];
static matrix matrix_result;

/*-----------------------------------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/
static void rpmsg_read_cb(struct rpmsg_channel *rp_chnl, void *data, int len,
                void * priv, unsigned long src)
{
	/* callback data are lost on return and need to be saved */
	if (!buffer_push(data, len)) {
		xil_printf("warning: cannot save data\n");
	}
}

static void rpmsg_channel_created(struct rpmsg_channel *rp_chnl)
{
	app_rp_chnl = rp_chnl;
	rp_ept = rpmsg_create_ept(rp_chnl, rpmsg_read_cb, RPMSG_NULL,
				RPMSG_ADDR_ANY);
}

static void rpmsg_channel_deleted(struct rpmsg_channel *rp_chnl)
{
	rpmsg_destroy_ept(rp_ept);
}

/*-----------------------------------------------------------------------------*
 *  Application specific
 *-----------------------------------------------------------------------------*/
static void Matrix_Multiply(const matrix *m, const matrix *n, matrix *r) {
	int i, j, k;

	env_memset(r, 0x0, sizeof(matrix));
	r->size = m->size;

	for (i = 0; i < m->size; ++i) {
		for (j = 0; j < n->size; ++j) {
			for (k = 0; k < r->size; ++k) {
				r->elements[i][j] += m->elements[i][k] * n->elements[k][j];
			}
		}
	}
}

static void mat_mul_demo(void *unused_arg)
{
	int status = 0;

	(void)unused_arg;

	/* Create buffer to send data between RPMSG callback and processing task */
	buffer_create();

	/* Initialize HW and SW components/objects */
	init_system();

	/* Resource table needs to be provided to remoteproc_resource_init() */
	rsc_info.rsc_tab = (struct resource_table *)&resources;
	rsc_info.size = sizeof(resources);

	/* Initialize OpenAMP framework */
	status = remoteproc_resource_init(&rsc_info, rpmsg_channel_created,
								 rpmsg_channel_deleted, rpmsg_read_cb,
								 &proc);
	if (RPROC_SUCCESS != status) {
		xil_printf("Error: initializing OpenAMP framework\n");
		return;
	}

	/* Stay in data processing loop until we receive a 'shutdown' message */
	while (1) {
		void *data;
		int len;

		/* wait for data... */
		buffer_pull(&data, &len);

		/* Process incoming message/data */
		if (*(int *)data == SHUTDOWN_MSG) {
			/* disable interrupts and free resources */
			remoteproc_resource_deinit(proc);

			/* Terminate this task */
			vTaskDelete(NULL);
			break;
		} else {
			env_memcpy(matrix_array, data, len);

			/* Process received data and multiple matrices. */
			Matrix_Multiply(&matrix_array[0], &matrix_array[1], &matrix_result);

			/* Send result back */
			if (RPMSG_SUCCESS != rpmsg_send(app_rp_chnl, &matrix_result, sizeof(matrix))) {
				xil_printf("Error: rpmsg_send failed\n");
			}
		}
	}
}

/*-----------------------------------------------------------------------------*
 *  Application entry point
 *-----------------------------------------------------------------------------*/
int main(void)
{
	BaseType_t stat;

	Xil_ExceptionDisable();

	/* Create the tasks */
	stat = xTaskCreate(mat_mul_demo, ( const char * ) "HW2",
				1024, NULL, 2, &comm_task);
	if (stat != pdPASS) {
		xil_printf("Error: cannot create task\n");
	} else {
		/* Start running FreeRTOS tasks */
		vTaskStartScheduler();
	}

	/* Will not get here, unless a call is made to vTaskEndScheduler() */
	while (1) {
		__asm__("wfi\n\t");
	}

	/* suppress compilation warnings*/
	return 0;
}
