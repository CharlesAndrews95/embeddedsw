# ==============================================================
# File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
# Version: 2015.1
# Copyright (C) 2015 Xilinx Inc. All rights reserved.
#
# ==============================================================

proc generate {drv_handle} {
    xdefine_include_file $drv_handle "xparameters.h" "XV_csc" \
        "NUM_INSTANCES" \
        "DEVICE_ID" \
        "C_S_AXI_CTRL_BASEADDR" \
        "C_S_AXI_CTRL_HIGHADDR" \
		"SAMPLES_PER_CLOCK" \
		"V_CSC_MAX_WIDTH" \
		"V_CSC_MAX_HEIGHT" \
		"MAX_DATA_WIDTH"

    xdefine_config_file $drv_handle "xv_csc_g.c" "XV_csc" \
        "DEVICE_ID" \
        "C_S_AXI_CTRL_BASEADDR" \
		"SAMPLES_PER_CLOCK" \
		"V_CSC_MAX_WIDTH" \
		"V_CSC_MAX_HEIGHT" \
		"MAX_DATA_WIDTH"

    xdefine_canonical_xpars $drv_handle "xparameters.h" "XV_csc" \
        "DEVICE_ID" \
        "C_S_AXI_CTRL_BASEADDR" \
        "C_S_AXI_CTRL_HIGHADDR" \
		"SAMPLES_PER_CLOCK" \
		"V_CSC_MAX_WIDTH" \
		"V_CSC_MAX_HEIGHT" \
		"MAX_DATA_WIDTH"
}
