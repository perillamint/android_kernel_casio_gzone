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
 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/

#if !defined(MIPI_LG4573B_H)
#define MIPI_LG4573B_H


int mipi_lg4573b_user_request_ctrl( struct msmfb_request_parame *data );

int mipi_lg4573b_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

void mipi_lg4573b_disable_display(struct msm_fb_data_type *mfd);
void mipi_lg4573b_disable_display_dstb(struct msm_fb_data_type *mfd);
void mipi_lg4573b_reg_ctrl(int on);

#endif  
