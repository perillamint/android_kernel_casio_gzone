/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lg4573b.h"

#define MIPI_LG4573B_PWM_LEVEL 255

static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	
	{0x03, 0x0a, 0x04, 0x00, 0x20}, 
	
	{0xae, 0x2a, 0x15,  
	 0x00, 
	 0x91, 0x93, 0x1a, 0x2a, 
	 0x19, 0x03, 0x04, 0xa0  }, 
	
	{0x5f, 0x00, 0x00, 0x10}, 
	
	{0xff, 0x00, 0x06, 0x00}, 
	
  {0x0,
  
   0x65, 0xb1, 0xda, 
   0x00, 0x50, 0x48, 0x63, 
   0x31, 0x0f, 0x07,
   0x00, 0x14, 0x03, 0x00, 0x02,
   0x00, 0x20, 0x00, 0x01  }, 

};

static int __init mipi_video_lg4573b_wvga_pt_init(void)
{
	int ret;

	pr_info("[DVE068_LCD]%s mipi-dsi lg4573b_wvga (800x480) driver ver 0.5.\n", __func__);






	pinfo.xres = 480;
	pinfo.yres = 800;

	pinfo.lcdc.xres_pad = 0;
	pinfo.lcdc.yres_pad = 0;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 99;
	pinfo.lcdc.h_front_porch = 5;
	pinfo.lcdc.h_pulse_width = 5;
	pinfo.lcdc.v_back_porch = 27;
	pinfo.lcdc.v_front_porch = 12;
	pinfo.lcdc.v_pulse_width = 5;
	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;
	
	
	pinfo.bl_max = MIPI_LG4573B_PWM_LEVEL;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 358000000;


	pinfo.mipi.mode = DSI_VIDEO_MODE;
	
	pinfo.mipi.pulse_mode_hsa_he = TRUE; 
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
    pinfo.mipi.data_lane2 = FALSE;
    pinfo.mipi.data_lane3 = FALSE;
	pinfo.mipi.tx_eot_append = TRUE;	
	pinfo.mipi.t_clk_post = 0x19;
	pinfo.mipi.t_clk_pre = 0x2E;
	pinfo.mipi.stream = 0; 
	pinfo.mipi.mdp_trigger = 0;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;

	pinfo.mipi.esc_byte_ratio = 2;


	ret = mipi_lg4573b_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}






module_init(mipi_video_lg4573b_wvga_pt_init);

