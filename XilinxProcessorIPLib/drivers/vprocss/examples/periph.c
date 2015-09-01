/******************************************************************************
*
* (c) Copyright 2014 - 2015 Xilinx, Inc. All rights reserved.
*
* This file contains confidential and proprietary information
* of Xilinx, Inc. and is protected under U.S. and
* international copyright and other intellectual property
* laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any
* rights to the materials distributed herewith. Except as
* otherwise provided in a valid license issued to you by
* Xilinx, and to the maximum extent permitted by applicable
* law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
* WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
* AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
* BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
* INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
* (2) Xilinx shall not be liable (whether in contract or tort,
* including negligence, or under any other theory of
* liability) for any loss or damage of any kind or nature
* related to, arising under or in connection with these
* materials, including for any direct, or any indirect,
* special, incidental, or consequential loss or damage
* (including loss of data, profits, goodwill, or any type of
* loss or damage suffered as a result of any action brought
* by a third party) even if such damage or loss was
* reasonably foreseeable or Xilinx had been advised of the
* possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-
* safe, or for use in any application requiring fail-safe
* performance, such as life-support or safety devices or
* systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any
* other applications that could lead to death, personal
* injury, or severe property or environmental damage
* (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and
* liability of any use of Xilinx products in Critical
* Applications, subject only to applicable laws and
* regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
* PART OF THIS FILE AT ALL TIMES.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file periph.c
*
* This is top level resource file that will initialize all system level
* peripherals
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 0.01  rc   07/07/14 First release

* </pre>
*
******************************************************************************/
#include "xparameters.h"
#include "xdebug.h"
#include "periph.h"
#include "xuartlite_l.h"


/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/**************************** Local Global *******************************/
/* Peripheral IP driver Instance */
XUartLite Uartl;
XV_tpg Tpg;
XVtc Vtc;
XGpio VidLocMonitor;

/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/

/************************** Function Definition ******************************/

/*****************************************************************************/
/**
 * This function reports system wide common peripherals included in the design
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 *
 *****************************************************************************/
void XPeriph_ReportDeviceInfo(XPeriph *InstancePtr)
{
  u32 numInstances;

  xil_printf("\r\n  ->System Peripherals Included\r\n");

  numInstances = XPAR_XUARTLITE_NUM_INSTANCES;
  if(numInstances > 0)
  {
    xil_printf("    : %d UART-Lite core\r\n", numInstances);
  }

  numInstances = XPAR_XV_TPG_NUM_INSTANCES;
  if(numInstances > 0)
  {
     xil_printf("    : %d TPG\r\n", numInstances);
  }

  numInstances = XPAR_XVTC_NUM_INSTANCES;
  if(numInstances > 0)
  {
    xil_printf("    : %d VTC\r\n", numInstances);
  }

  numInstances = XPAR_XGPIO_NUM_INSTANCES;
  if(numInstances > 1)
  {
     xil_printf("    : 1 Video Lock Monitor\r\n");
  }
}

/*****************************************************************************/
/**
 * This function initializes system wide common peripherals.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 * @return XST_SUCCESS
 *
 *****************************************************************************/
int XPeriph_PowerOnInit(XPeriph *InstancePtr)
{
  int status = XST_FAILURE;
  XUartLite_Config *UartlCfgPtr;
  XVtc_Config *VtcConfigPtr;
  XGpio_Config *VidLockMonCfgPtr;

  Xil_AssertNonvoid(InstancePtr != NULL);

  //Bind the peripheral instance to ip drivers
  InstancePtr->UartlPtr = &Uartl;
  InstancePtr->TpgPtr   = &Tpg;
  InstancePtr->VtcPtr   = &Vtc;
  InstancePtr->VidLockMonitorPtr = &VidLocMonitor;

  //UART-Lite
  xdbg_printf(XDBG_DEBUG_GENERAL,"    ->Initializing UART Lite.... \r\n");
  UartlCfgPtr = XUartLite_LookupConfig(XPAR_MICROBLAZE_SS_AXI_UARTLITE_0_DEVICE_ID);
  if(UartlCfgPtr == NULL)
  {
	xil_printf("ERR:: UART Lite device not found\r\n");
    return(XST_DEVICE_NOT_FOUND);
  }
  status = XUartLite_CfgInitialize(InstancePtr->UartlPtr,
		                           UartlCfgPtr,
		                           UartlCfgPtr->RegBaseAddr);
  if(status != XST_SUCCESS)
  {
    xil_printf("ERR:: UART Lite Initialization failed %d\r\n", status);
	return(XST_FAILURE);
  }
  //Reset UART FIFO
  XUartLite_ResetFifos(InstancePtr->UartlPtr);

  //TPG
  xdbg_printf(XDBG_DEBUG_GENERAL,"    ->Initializing TPG.... \r\n");
  status = XV_tpg_Initialize(InstancePtr->TpgPtr, XPAR_V_TPG_0_DEVICE_ID);
  if(status == XST_DEVICE_NOT_FOUND)
  {
    xil_printf("ERR:: TPG device not found\r\n");
    return(status);
  }

  //VTC
  xdbg_printf(XDBG_DEBUG_GENERAL,"    ->Initializing VTC.... \r\n");
  VtcConfigPtr = XVtc_LookupConfig(XPAR_V_TC_0_DEVICE_ID);
  if(VtcConfigPtr == NULL)
  {
	xil_printf("ERR:: VTC device not found\r\n");
    return(XST_DEVICE_NOT_FOUND);
  }
  status = XVtc_CfgInitialize(InstancePtr->VtcPtr,
		                      VtcConfigPtr,
		                      VtcConfigPtr->BaseAddress);
  if(status != XST_SUCCESS)
  {
	  xil_printf("ERR:: VTC Initialization failed %d\r\n", status);
	  return(XST_FAILURE);
  }


  //GPIO - Video Lock Monitor
  xdbg_printf(XDBG_DEBUG_GENERAL,"    ->Initializing Video Lock Monitor.... \r\n");
  VidLockMonCfgPtr = XGpio_LookupConfig(XPAR_VIDEO_LOCK_MONITOR_DEVICE_ID);
  if(VidLockMonCfgPtr == NULL)
  {
	xil_printf("ERR:: GPIO device not found\r\n");
    return(XST_DEVICE_NOT_FOUND);
  }
  status = XGpio_CfgInitialize(InstancePtr->VidLockMonitorPtr,
		                       VidLockMonCfgPtr,
		                       VidLockMonCfgPtr->BaseAddress);
  if(status != XST_SUCCESS)
  {
	  xil_printf("ERR:: Video Lock Monitor Initialization failed %d\r\n", status);
	  return(XST_FAILURE);
  }

  return(status);
}

/*****************************************************************************/
/**
 * This function configures TPG to user defined parameters
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 *
 *****************************************************************************/
void XPeriph_ConfigTpg(XPeriph *InstancePtr)
{
  XV_tpg *pTpg = InstancePtr->TpgPtr;

  //Stop TPG
  XV_tpg_DisableAutoRestart(pTpg);

  XV_tpg_Set_height(pTpg, InstancePtr->TpgConfig.Height);
  XV_tpg_Set_width(pTpg,  InstancePtr->TpgConfig.Width);
  XV_tpg_Set_colorFormat(pTpg, InstancePtr->TpgConfig.ColorFmt);
  XV_tpg_Set_bckgndId(pTpg, InstancePtr->TpgConfig.Pattern);
  XV_tpg_Set_ovrlayId(pTpg, 0);

  //Start TPG
  XV_tpg_EnableAutoRestart(pTpg);
  XV_tpg_Start(pTpg);
}

/*****************************************************************************/
/**
 * This function programs TPG to user defined resolution
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 * @param  width is the new active width
 * @param  height is the new active height
 *****************************************************************************/
void XPeriph_SetTpgParams(XPeriph *InstancePtr,
		                  u16 width,
		                  u16 height,
			              XVidC_ColorFormat Cformat,
			              u16 Pattern,
			              u16 IsInterlaced)
{
  xdbg_printf(XDBG_DEBUG_GENERAL,"\r\nConfigure TPG to %d x %d\r\n",
		                        (int)width, (int)height);

  XPeriph_SetTPGWidth(InstancePtr,  width);
  XPeriph_SetTPGHeight(InstancePtr, height);
  XPeriph_SetTPGColorFormat(InstancePtr, Cformat);
  XPeriph_SetTPGPattern(InstancePtr, Pattern);
  XPeriph_SetTPGInterlacedMode(InstancePtr, IsInterlaced);
}

/*****************************************************************************/
/**
 * This function stops TPG IP
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 *
 *****************************************************************************/
void XPeriph_DisableTpg(XPeriph *InstancePtr)
{
  //Stop TPG
  XV_tpg_DisableAutoRestart(InstancePtr->TpgPtr);
}

/*****************************************************************************/
/**
 * This function reports TPG Status
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 *
 *****************************************************************************/
void XPeriph_TpgDbgReportStatus(XPeriph *InstancePtr)
{
  u32 done, idle, ready, ctrl;
  u32 width, height, bgid, cfmt;
  XV_tpg *pTpg = InstancePtr->TpgPtr;

  if(pTpg)
  {
    xil_printf("\r\n\r\n----->TPG STATUS<----\r\n");

	done  = XV_tpg_IsDone(pTpg);
	idle  = XV_tpg_IsIdle(pTpg);
	ready = XV_tpg_IsReady(pTpg);
	ctrl  = XV_tpg_ReadReg(pTpg->Config.BaseAddress, XV_TPG_CTRL_ADDR_AP_CTRL);

	width  = XV_tpg_Get_width(pTpg);
	height = XV_tpg_Get_height(pTpg);
	bgid   = XV_tpg_Get_bckgndId(pTpg);
	cfmt   = XV_tpg_Get_colorFormat(pTpg);

    xil_printf("IsDone:  %d\r\n", done);
    xil_printf("IsIdle:  %d\r\n", idle);
    xil_printf("IsReady: %d\r\n", ready);
    xil_printf("Ctrl:    0x%x\r\n\r\n", ctrl);

    xil_printf("Width:        %d\r\n",width);
    xil_printf("Height:       %d\r\n",height);
	xil_printf("Backgnd Id:   %d\r\n",bgid);
	xil_printf("Color Format: %d\r\n",cfmt);
  }
}

/*****************************************************************************/
/**
 * This function configures VTC to output parameters
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 * @param  StreamPtr is a pointer output stream
 *
 *****************************************************************************/
void XPeriph_ConfigVtc(XPeriph *InstancePtr,
		               XVidC_VideoStream *StreamPtr,
		               u32 PixPerClk)
{
  XVtc_Polarity Polarity;
  XVtc_SourceSelect SourceSelect;
  XVtc_Timing VideoTiming;

  /* Disable Generator */
  XVtc_Reset(InstancePtr->VtcPtr);
  XVtc_DisableGenerator(InstancePtr->VtcPtr);
  XVtc_Disable(InstancePtr->VtcPtr);

  /* Set up source select
     1 = Generator registers, 0 = Detector registers
  **/
  memset((void *)&SourceSelect, 1, sizeof(SourceSelect));

  /* 1 = Generator registers, 0 = Detector registers */
  SourceSelect.VChromaSrc = 1;
  SourceSelect.VActiveSrc = 1;
  SourceSelect.VBackPorchSrc = 1;
  SourceSelect.VSyncSrc = 1;
  SourceSelect.VFrontPorchSrc = 1;
  SourceSelect.VTotalSrc = 1;
  SourceSelect.HActiveSrc = 1;
  SourceSelect.HBackPorchSrc = 1;
  SourceSelect.HSyncSrc = 1;
  SourceSelect.HFrontPorchSrc = 1;
  SourceSelect.HTotalSrc = 1;

//  XVtc_SetSource(InstancePtr->VtcPtr, &SourceSelect);
  // Note: Can not use SetSource function call because the XVtc_SourceSelect struct
  // does not have the interlace field. Workaround is to write it manually.
  XVtc_WriteReg(InstancePtr->VtcPtr->Config.BaseAddress, (XVTC_CTL_OFFSET), 0x07FFFF00);


  VideoTiming.HActiveVideo  = StreamPtr->Timing.HActive;
  VideoTiming.HFrontPorch   = StreamPtr->Timing.HFrontPorch;
  VideoTiming.HSyncWidth    = StreamPtr->Timing.HSyncWidth;
  VideoTiming.HBackPorch    = StreamPtr->Timing.HBackPorch;
  VideoTiming.HSyncPolarity = StreamPtr->Timing.HSyncPolarity;

  /* Vertical Timing */
  VideoTiming.VActiveVideo = StreamPtr->Timing.VActive;

  // The VTC has an offset issue.
  // This results into a wrong front porch and back porch value.
  // As a workaround the front porch and back porch need to be adjusted.
  VideoTiming.V0FrontPorch = StreamPtr->Timing.F0PVFrontPorch;
  VideoTiming.V0BackPorch  = StreamPtr->Timing.F0PVBackPorch;
  VideoTiming.V0SyncWidth  = StreamPtr->Timing.F0PVSyncWidth;
  VideoTiming.V1FrontPorch = StreamPtr->Timing.F1VFrontPorch;
  VideoTiming.V1SyncWidth  = StreamPtr->Timing.F1VSyncWidth;
  VideoTiming.V1BackPorch  = StreamPtr->Timing.F1VBackPorch;

  VideoTiming.VSyncPolarity = StreamPtr->Timing.VSyncPolarity;

  VideoTiming.Interlaced = FALSE;

  /* YUV420 */
  if(StreamPtr->ColorFormatId == XVIDC_CSF_YCRCB_420) {
	VideoTiming.HActiveVideo = VideoTiming.HActiveVideo/4;
	VideoTiming.HFrontPorch  = VideoTiming.HFrontPorch/4;
	VideoTiming.HBackPorch   = VideoTiming.HBackPorch/4;
	VideoTiming.HSyncWidth   = VideoTiming.HSyncWidth/4;
  }
  /* Other color spaces */
  else
  {
	VideoTiming.HActiveVideo = VideoTiming.HActiveVideo/PixPerClk;
	VideoTiming.HFrontPorch  = VideoTiming.HFrontPorch/PixPerClk;
	VideoTiming.HBackPorch   = VideoTiming.HBackPorch/PixPerClk;
	VideoTiming.HSyncWidth   = VideoTiming.HSyncWidth/PixPerClk;
  }

  XVtc_SetGeneratorTiming(InstancePtr->VtcPtr, &VideoTiming);

  /* Set up Polarity of all outputs */
  memset((void *)&Polarity, 0, sizeof(XVtc_Polarity));
  Polarity.ActiveChromaPol = 1;
  Polarity.ActiveVideoPol = 1;
//  Polarity.FieldIdPol = ((VideoTiming.Interlaced) ? 0 : 1);
  // Note: With this fix the polarity of field-id does not have to be switched.
  // I have found that the DELL monitor I am using does not care
  // about polarity for interlaced video. As a result setting polarity HIGH or
  // LOW for interlaced video does not make a difference.
  Polarity.FieldIdPol = 0;

  Polarity.VBlankPol = VideoTiming.VSyncPolarity;
  Polarity.VSyncPol  = VideoTiming.VSyncPolarity;
  Polarity.HBlankPol = VideoTiming.HSyncPolarity;
  Polarity.HSyncPol  = VideoTiming.HSyncPolarity;

  XVtc_SetPolarity(InstancePtr->VtcPtr, &Polarity);

  /* Enable generator module */
  XVtc_Enable(InstancePtr->VtcPtr);
  XVtc_EnableGenerator(InstancePtr->VtcPtr);
  XVtc_RegUpdateEnable(InstancePtr->VtcPtr);
}