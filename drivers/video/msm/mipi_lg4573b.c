/* Copyright (c) 2008-2011, Code Aurora Forum. All rights reserved.
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
#include <mach/board_gg3.h>
#include <mach/gpio.h>

#define DVE068_LCD_DEBUG 0

#include "mdp4.h"

//* M7-B-Changwoo-2012.05.11 LCD initial sequence and Off Command FIX*/

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)


extern void lm3530_lcd_backlight_enable(int level);
extern void lm3530_lcd_backlight_disable(int level);

extern void mipi_dsi_cdp_panel_power_interface(int on);
extern int mipi_dsi_cdp_panel_power_dstb(int on);
extern void mipi_dsi_panel_power_wrap( int on );


int lcd_status=0;
int lcd_first_init=1;
int gg3_backlight_status;
int bright_only = 0;

bool dstb_check_flag = false;


static int bl_level_table[2][256]={
{

	0,0,0,0,0,0,								
	6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,									
	20,20,20,20,20,20,20, 						
	27,27,27,27,27,27,27, 						
	34,34,34,34,34,34,34,34, 					
	42,42,									
	44,44,44,44,44, 							
	49,49,49,49,49, 							
	54,54, 									
	56,56,56,56,56,56,56,56, 					
	64,64,64,64,64,64,64,64,64,					
	73,73,73,73,73,73,73,73,73,73,73, 			
	84,84,84,84,84,84,84,84,84,84, 				
	94,94,94,94,94,94,94,94,94,94, 				
	104,104,104,104,104, 						
	109,109,109,109,109,109, 					
	115,115,115,115,115,115,115,115,
	115,115, 									
	125,125,125,125,125,125,125,125,
	125,125, 									
	135,135,135,135,135, 						
	140,140,140,140,140,140, 					
	146,146,146,146,146,146,146,146,
	146,146,146,								
	157,157,157,157,157,157,157,157,
	157,157,157, 								
	168,168,168,168,168, 						
	173,173,173,173,173,173, 					
	179,179,179,179,179,179,179,179,
	179,179,179, 								
	190,190,190,190,190,190,190,190,
	190,190,190,190, 							
	202,202,202,202,202,202,202,202,202,202, 	
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,



































































},





































































{

	0,0,0,0,0,0,								
	6,6,6,6,6,6,6,6,6,6,
	6,6,6,6,									
	20,20,20,20,20,20,20, 						
	27,27,27,27,27,27,27, 						
	34,34,34,34,34,34,34,34, 					
	42,42,									
	44,44,44,44,44, 							
	49,49,49,49,49, 							
	54,54, 									
	56,56,56,56,56,56,56,56, 					
	64,64,64,64,64,64,64,64,64,					
	73,73,73,73,73,73,73,73,73,73,73, 			
	84,84,84,84,84,84,84,84,84,84, 				
	94,94,94,94,94,94,94,94,94,94, 				
	104,104,104,104,104, 						
	109,109,109,109,109,109, 					
	115,115,115,115,115,115,115,115,
	115,115, 									
	125,125,125,125,125,125,125,125,
	125,125, 									
	135,135,135,135,135, 						
	140,140,140,140,140,140, 					
	146,146,146,146,146,146,146,146,
	146,146,146,								
	157,157,157,157,157,157,157,157,
	157,157,157, 								
	168,168,168,168,168, 						
	173,173,173,173,173,173, 					
	179,179,179,179,179,179,179,179,
	179,179,179, 								
	190,190,190,190,190,190,190,190,
	190,190,190,190, 							
	202,202,202,202,202,202,202,202,202,202, 	
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,202,202,202,202,202,202,
	202,202,202,202,



































































}

};



#define BL_WINDOW_SIZE 6
#define BL_LM3530_BRIGHT_MAX 0xCA

#define LG4573B_CMD_DELAY 0 
#define LG4573B_BL_DELAY 0 
#define LG4573B_DISPLAY_ON_DELAY 120
static struct mipi_dsi_panel_platform_data *mipi_lg4573b_pdata;

static struct dsi_buf lg4573b_tx_buf;
static struct dsi_buf lg4573b_rx_buf;
static int board_revision; 
static char config_sleep_out[2] = {0x11, 0x00};
static char config_otp1[4] = {0xf8, 0x08, 0x00, 0x00};
static char config_otp2[4] = {0xf8, 0x00, 0x00, 0x00};
static char config_otp3[2] = {0xf9, 0x40};
static char config_display_inversion_off[2] = {0x20, 0x00};
static char config_panel_character_setting[3] = {0xB2, 0x00, 0xC8};
static char config_panel_drive_setting[2] = {0xB3, 0x00};
static char config_display_mode_control[2] = {0xB4, 0x04};
static char config_display_control1[2] = {0xB5, 0x42};
static char config_display_control2[7] = {0xB6, 0x0B, 0x0F, 0x3C, 0x18, 0x18, 0xE8};
static char config_osc_setting[3] = {0xC0, 0x01, 0x1A};
static char config_power_control1[2] = {0xC1, 0x00};

static char config_power_control3[6] = {0xC3, 0x07, 0x04, 0x03, 0x03, 0x02};
static char config_power_control4[7] = {0xC4, 0x12, 0x24, 0x13, 0x13, 0x02, 0x4C};
static char config_power_control5[2] = {0xC5, 0x6D};
static char config_power_control6[4] = {0xC6, 0x42, 0x63, 0x00};
static char config_p_gamma_red[10] = {0xD0, 0x23, 0x13, 0x41, 0x16, 0x00, 0x0A, 0x61, 0x16, 0x03};
static char config_n_gamma_red[10] = {0xD1, 0x23, 0x13, 0x41, 0x16, 0x00, 0x0A, 0x61, 0x16, 0x03};
static char config_p_gamma_green[10] = {0xD2, 0x23, 0x13, 0x41, 0x16, 0x00, 0x0A, 0x61, 0x16, 0x03};
static char config_n_gamma_green[10] = {0xD3, 0x23, 0x13, 0x41, 0x16, 0x00, 0x0A, 0x61, 0x16, 0x03};
static char config_p_gamma_blue[10] = {0xD4, 0x23, 0x13, 0x41, 0x16, 0x00, 0x0A, 0x61, 0x16, 0x03};
static char config_n_gamma_blue[10] = {0xD5, 0x23, 0x13, 0x41, 0x16, 0x00, 0x0A, 0x61, 0x16, 0x03};
static char config_display_on[2] = {0x29, 0x00};


static char config_display_off[2] = {0x28, 0x00};
static char config_sleep_in[2] = {0x10, 0x00};

static char config_write_display_brightness[2] = {0x51, 0x66}; 
static char config_write_control_display[2] = {0x53, 0x2C};  
static char config_write_control_display_1[2] = {0x53, 0x24};  







static char config_adptive_brightness_control[2] = {0x55, 0x00}; 
static char config_adptive_brightness_control_1[2] = {0x55, 0x00}; 
static char config_write_cabc_minimum_brightness[2] = {0x5E, 0x00};

static char config_bl_control[3] = {0xC8, 0x11, 0x03}; 
static char config_bl_control_1[3] = {0xC8, 0x11, 0x03}; 
static char config_bl_control_2[3] = {0xC8, 0x11, 0x01}; 



static char config_dstb_on[2] = {0xC1, 0x01};


static struct dsi_cmd_desc lg4573b_power_up_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_otp1), config_otp1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_otp2), config_otp2 },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_otp3), config_otp3 },
	{DTYPE_DCS_WRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_display_inversion_off), config_display_inversion_off },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_panel_character_setting), config_panel_character_setting },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_panel_drive_setting), config_panel_drive_setting },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_display_mode_control), config_display_mode_control },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_display_control1), config_display_control1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_display_control2), config_display_control2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_osc_setting), config_osc_setting },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_power_control1), config_power_control1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_power_control3), config_power_control3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_power_control4), config_power_control4 },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_power_control5), config_power_control5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_power_control6), config_power_control6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_p_gamma_red), config_p_gamma_red },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_n_gamma_red), config_n_gamma_red },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_p_gamma_green), config_p_gamma_green },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_n_gamma_green), config_n_gamma_green },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_p_gamma_blue), config_p_gamma_blue },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_n_gamma_blue), config_n_gamma_blue },




};














static struct dsi_cmd_desc lg4573b_power_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_display_off), config_display_off },




	{DTYPE_DCS_WRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_sleep_in), config_sleep_in },



};










static struct dsi_cmd_desc lg4573b_display_bl_control_cmds[] = {  
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_display_brightness), config_write_display_brightness },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_control_display), config_write_control_display },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_adptive_brightness_control), config_adptive_brightness_control },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_cabc_minimum_brightness), config_write_cabc_minimum_brightness },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_bl_control_1), config_bl_control_1 }
};










static struct dsi_cmd_desc lg4573b_display_bl_brightness_control_cmds[] = {  
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_display_brightness), config_write_display_brightness },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_control_display_1), config_write_control_display_1 },		
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_adptive_brightness_control_1), config_adptive_brightness_control_1 },			
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_cabc_minimum_brightness), config_write_cabc_minimum_brightness },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_bl_control), config_bl_control },
};



static struct dsi_cmd_desc lg4573b_display_bl_brightness_control_cmds_default_case[] = { 
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_display_brightness), config_write_display_brightness },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_control_display_1), config_write_control_display_1 },		
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_adptive_brightness_control_1), config_adptive_brightness_control_1 },			
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_bl_control), config_bl_control },
};


static struct dsi_cmd_desc lg4573b_display_bl_brightness_control_only[] = {  
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_display_brightness), config_write_display_brightness },
};









static struct dsi_cmd_desc lg4573b_display_bl_control_cmds_1[] = {  
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_display_brightness), config_write_display_brightness },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_control_display), config_write_control_display },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_adptive_brightness_control), config_adptive_brightness_control },
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_cabc_minimum_brightness), config_write_cabc_minimum_brightness },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_bl_control_2), config_bl_control_2 }
};

static struct dsi_cmd_desc lg4573b_display_bl_brightness_control_cmds_1[] = {  
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_display_brightness), config_write_display_brightness },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_control_display_1), config_write_control_display_1 },		
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_adptive_brightness_control_1), config_adptive_brightness_control_1 },			
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_write_cabc_minimum_brightness), config_write_cabc_minimum_brightness },
	{DTYPE_DCS_LWRITE, 1, 0, 0, LG4573B_BL_DELAY, sizeof(config_bl_control_2), config_bl_control_2 },
};



extern void mxt224E_power_on(void);



static struct dsi_cmd_desc lg4573b_lcdm_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_display_on), config_display_on }
};
static struct dsi_cmd_desc lg4573b_sleep_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_sleep_out), config_sleep_out }
};


static struct dsi_cmd_desc lg4573b_dstb_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, LG4573B_CMD_DELAY, sizeof(config_dstb_on), config_dstb_on }
};

DEFINE_MUTEX(g_lcd_mutex);
static struct delayed_work displayon_delaywork;
static struct msm_fb_data_type *mipi_lg4573b_mfd = NULL;
static void mipi_lg4573b_displayon_delayed_work(struct work_struct *work)
{
	mutex_lock(&g_lcd_mutex);
	if (mipi_lg4573b_mfd)
	{
		mutex_lock(&mipi_lg4573b_mfd->dma->ov_mutex);
		mipi_set_tx_power_mode(0);
		mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_lcdm_display_on_cmds,
								ARRAY_SIZE(lg4573b_lcdm_display_on_cmds));
		mutex_unlock(&mipi_lg4573b_mfd->dma->ov_mutex);
		pr_info("mipi_lg4573b_displayon_delayed_work execute Display On!!\n");

		if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A) )
			lm3530_lcd_backlight_enable(0x7F); 

		lcd_status = 2;
	}
	mutex_unlock(&g_lcd_mutex);
	return;
}



static int mipi_lg4573b_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int ret;
	static int  gpio43; 

	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD on Enter\n", __func__);
	mfd = platform_get_drvdata(pdev);

	bright_only = 0;


	if(dstb_check_flag){
		
		dstb_check_flag = false;
	}









	if (!mfd){
		pr_info("[DVE068_LCD]%s !mfd\n", __func__);
		return -ENODEV;
	}
	if (mfd->key != MFD_KEY){
		pr_info("[DVE068_LCD]%s mfd->key != MFD_KEY\n", __func__);
		return -EINVAL;
	}




	if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A) ) 
	{
		if(lcd_first_init)
		{
			int gpio24;
			pr_info("[DVE068_LCD]%s LG4573B LCD first init 1 [%d] \n", __func__, lcd_first_init); 
			
			gpio24 = PM8921_GPIO_PM_TO_SYS(24);

			if(!gpio_get_value_cansleep(gpio24))
			{
				pr_info("[DVE068_LCD]%s LG4573B LCD first init 2 \n", __func__);	
				gpio_set_value_cansleep(gpio24,1);
				mdelay(10);
			}
			lm3530_lcd_backlight_enable(0x7F);
		}
	}


	if(lcd_first_init || !((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A) )){

		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
		
		gpio_set_value_cansleep(gpio43, 0);
		mdelay(5);
		gpio_set_value_cansleep(gpio43, 1);
		mdelay(10);

		mipi_set_tx_power_mode(0);

	}

	ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_power_up_cmds,
			ARRAY_SIZE(lg4573b_power_up_cmds));

	if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V6A) ) 
	{

		if(board_revision == M7SYSTEM_REV_V6A)  
		{
			config_write_display_brightness[1]= 0xFF;
			ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_control_cmds, 
				ARRAY_SIZE(lg4573b_display_bl_control_cmds));
		}
		else if(lcd_first_init){
			pr_info("[DVE068_LCD]%s LG4573B LCD first init 3 =========[%d] \n", __func__, lcd_first_init); 

			if(0) ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_control_cmds, ARRAY_SIZE(lg4573b_display_bl_control_cmds));
			config_write_display_brightness[1]=(unsigned char)0x66;
			ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_cmds,ARRAY_SIZE(lg4573b_display_bl_brightness_control_cmds));

			lcd_first_init = 0;
		}

		else{
			if(DVE068_LCD_DEBUG)
				pr_info("[DVE068_LCD]%s LG4573B command tx : lg4573b_display_bl_brightness_control_cmds_default_case \n", __func__);	
			ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_cmds_default_case,ARRAY_SIZE(lg4573b_display_bl_brightness_control_cmds_default_case));
		}

	}
	else
	{
		if(board_revision == M7SYSTEM_REV_V2A){  
			config_write_display_brightness[1]= 0xFF;
		}
		ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_control_cmds_1,
			ARRAY_SIZE(lg4573b_display_bl_control_cmds_1));
	}


	lcd_status = 1;

	ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_sleep_off_cmds,
			ARRAY_SIZE(lg4573b_sleep_off_cmds));
	if (pdev->id == 0) {
		
		msleep(LG4573B_DISPLAY_ON_DELAY);
		mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_lcdm_display_on_cmds,
							ARRAY_SIZE(lg4573b_lcdm_display_on_cmds));
	}
	else {
		cancel_delayed_work_sync(&displayon_delaywork);
		mutex_lock(&g_lcd_mutex);
		mipi_lg4573b_mfd = mfd;
		if(!schedule_delayed_work(&displayon_delaywork, msecs_to_jiffies(LG4573B_DISPLAY_ON_DELAY))){
			
			msleep(LG4573B_DISPLAY_ON_DELAY);
			mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_lcdm_display_on_cmds,
								ARRAY_SIZE(lg4573b_lcdm_display_on_cmds));
		}
		mutex_unlock(&g_lcd_mutex);
	}
	


	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD on Exit \n", __func__);	

	gg3_backlight_status = 1;
	return 0;
}

static int mipi_lg4573b_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD off Enter\n", __func__);
	mfd = platform_get_drvdata(pdev);

	bright_only = 0;

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	

	cancel_delayed_work_sync(&displayon_delaywork);
	mutex_lock(&g_lcd_mutex);
	mipi_lg4573b_mfd = NULL;
	mutex_unlock(&g_lcd_mutex);

	
	if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A))  
	{
		pr_info("[DVE068_LCD]%s LG4573B LCD off Enter2 [%p][%d]\n", __func__, mfd, mfd->key);
		if(0) mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_power_off_cmds,	ARRAY_SIZE(lg4573b_power_off_cmds));
		lm3530_lcd_backlight_disable(0x00);
	}
	

	lcd_status = 0;

	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD off Exit\n", __func__);

	gg3_backlight_status = 0;
	return 0;
}

static void mipi_lg4573b_set_backlight(struct msm_fb_data_type *mfd)
{

	struct mipi_panel_info *mipi;
	static int bl_level_old;
	
	static int gpio24;
	int ret,num_revision;


	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s in \n", __func__);	


	if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A) ) 
	{
		extern int get_backlight_status(void);







		if(lcd_status >= 1 && mfd->bl_level)
		{
			if(lcd_status==1)
			{
				cancel_delayed_work_sync(&displayon_delaywork);
				mutex_lock(&g_lcd_mutex);
				if (mipi_lg4573b_mfd)
				{
					mdelay(50);
					mutex_lock(&mipi_lg4573b_mfd->dma->ov_mutex);
					mipi_set_tx_power_mode(0);
					mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_lcdm_display_on_cmds,
											ARRAY_SIZE(lg4573b_lcdm_display_on_cmds));
					mutex_unlock(&mipi_lg4573b_mfd->dma->ov_mutex);
					pr_info("mipi_lg4573b_displayon_delayed_work execute Display On leesh !!\n");
				}
				mutex_unlock(&g_lcd_mutex);
			}
			
			lm3530_lcd_backlight_enable(0x7F); 
			lcd_status = 2;
		}

		num_revision = 0;
	}
	else
	{
	

		if(board_revision == M7SYSTEM_REV_V2A ||board_revision == M7SYSTEM_REV_V6A )
			return;
		
		num_revision = 1;
	}
	mipi  = &mfd->panel_info.mipi;
	if(0){
		return;
	
		
	}

	mutex_lock(&mfd->dma->ov_mutex);







	if(mdp4_overlay_dsi_state_get() <= ST_DSI_SUSPEND) {
		msleep(500);
		if(mdp4_overlay_dsi_state_get() <= ST_DSI_SUSPEND){
			pr_info("[DVE068_LCD]%s NOP Exit \n", __func__);
			mutex_unlock(&mfd->dma->ov_mutex);
			return;
		}
		else{
			pr_info("[DVE068_LCD]%s wait OK \n", __func__);
		}
	}



	






	mipi_dsi_mdp_busy_wait();

	
	if(DVE068_LCD_DEBUG)
	pr_info("[DVE068_LCD]%s get_m7system_board_revision = %x\n", __func__, board_revision);

	if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A) ) 
	{
	
		config_write_cabc_minimum_brightness[1]=(unsigned char)( (bl_level_table[num_revision][mfd->bl_level] - BL_WINDOW_SIZE) > 0 ? (bl_level_table[num_revision][mfd->bl_level] - BL_WINDOW_SIZE) : 0 );


		if(mfd->bl_level >= BL_WINDOW_SIZE ) 
		{
			if(mfd->bl_level >= BL_LM3530_BRIGHT_MAX)
			{
				config_write_display_brightness[1]=(unsigned char)(BL_LM3530_BRIGHT_MAX);
			}
			else
			{
				config_write_display_brightness[1]=(unsigned char)(mfd->bl_level);
			}

			if(bright_only)
				ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_only,ARRAY_SIZE(lg4573b_display_bl_brightness_control_only));
			else
			{
				bright_only = 1;




				ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_only,ARRAY_SIZE(lg4573b_display_bl_brightness_control_only));


			}
		}

		if( (lcd_status >= 1) && (mfd->bl_level == 0))
		{
			





			lm3530_lcd_backlight_disable(0x00);
		}


	}
	else
	{
		gpio24 = PM8921_GPIO_PM_TO_SYS(24);

		if(!gpio_get_value_cansleep(gpio24)) 
			gpio_set_value_cansleep(gpio24,1);
		
		
			config_write_display_brightness[1]=(unsigned char)(bl_level_table[num_revision][mfd->bl_level]);
			config_write_cabc_minimum_brightness[1]=(unsigned char)( (bl_level_table[num_revision][mfd->bl_level] - BL_WINDOW_SIZE) > 0 ? (bl_level_table[num_revision][mfd->bl_level] - BL_WINDOW_SIZE) : 0 );
		
		
		


		if(board_revision == M7SYSTEM_REV_V7A)
		{
			if(config_write_display_brightness[1] < BL_WINDOW_SIZE ){
				lg4573b_display_bl_brightness_control_cmds[4].wait = 30;
			}	
			
			if(config_write_display_brightness[1] != 0){




				ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_only,ARRAY_SIZE(lg4573b_display_bl_brightness_control_only));



				}
			
	
			if(config_write_display_brightness[1] < BL_WINDOW_SIZE ){
				lg4573b_display_bl_brightness_control_cmds[4].wait = 0; 
			}
		}
		else
		{
			if(config_write_display_brightness[1] < BL_WINDOW_SIZE ){ 
				lg4573b_display_bl_brightness_control_cmds_1[4].wait = 30; 
			}
				
				if(config_write_display_brightness[1] != 0){
					ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_cmds_1,
					ARRAY_SIZE(lg4573b_display_bl_brightness_control_cmds_1));
					}
	

			
			if(config_write_display_brightness[1] < BL_WINDOW_SIZE ){ 
				lg4573b_display_bl_brightness_control_cmds_1[4].wait = 0; 
				
			}
		}


	}

	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s: bl_level_table[mfd->bl_level(=%d)]=%d, config_write_display_brightness[1]= %d\n",
			__func__, mfd->bl_level, bl_level_table[num_revision][mfd->bl_level], config_write_display_brightness[1]);


		
	bl_level_old = mfd->bl_level;

	mutex_unlock(&mfd->dma->ov_mutex);

  
	if((config_write_display_brightness[1] == 0) && ((board_revision != M7SYSTEM_REV_V3A) && (board_revision != M7SYSTEM_REV_V8A) && (board_revision != M7SYSTEM_REV_V9A)))
	{
		gpio_set_value_cansleep(gpio24, 0);
	}
	
	pr_info("[DVE068_LCD]%s exit\n", __func__);
	return;
}


ssize_t mipi_lg4573b_lcd_show_onoff(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("%s : start\n", __func__);
	return 0;
}

ssize_t mipi_lg4573b_lcd_store_onoff(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	
	int onoff, ret;
	struct msm_fb_data_type *mfd;
	struct platform_device *pdev = to_platform_device(dev);

	mfd = platform_get_drvdata(pdev);

	sscanf(buf, "%d", &onoff);
	printk("%s: onoff : %d\n", __func__, onoff);






	
	config_write_display_brightness[1]= (unsigned char)onoff;
	config_write_cabc_minimum_brightness[1]= (unsigned char)onoff;


	if((board_revision == M7SYSTEM_REV_V3A) || (board_revision >= M7SYSTEM_REV_V8A) ) 
	{   
	
		

		ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_cmds,
				ARRAY_SIZE(lg4573b_display_bl_brightness_control_cmds));
		
		
	}
	else
	{
		ret=mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_display_bl_brightness_control_cmds_1,
				ARRAY_SIZE(lg4573b_display_bl_brightness_control_cmds_1));
	}



	printk("%s: end :\n", __func__);


	

	return count;
}


DEVICE_ATTR(lcd_onoff, 0664, mipi_lg4573b_lcd_show_onoff, mipi_lg4573b_lcd_store_onoff);

  
static int __devinit mipi_lg4573b_lcd_probe(struct platform_device *pdev)
{
	int rc = 0;    
	if (pdev->id == 0) {
		mipi_lg4573b_pdata = pdev->dev.platform_data;
		return 0;
	}

	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD probe\n", __func__);
	msm_fb_add_device(pdev);

	rc = device_create_file(&pdev->dev, &dev_attr_lcd_onoff);    

	INIT_DELAYED_WORK(&displayon_delaywork, mipi_lg4573b_displayon_delayed_work);


	return 0;
}

static void __devinit mipi_lg4573b_lcd_shutdown(struct platform_device *pdev)
{












	printk("%s: mipi_lg4573b_lcd_shutdown --------------------1 :\n", __func__);


}

static struct platform_driver this_driver = {
	.probe  = mipi_lg4573b_lcd_probe,
	.driver = {
		.name   = "mipi_lg4573b",
	},
	.shutdown  = mipi_lg4573b_lcd_shutdown,
};

static struct msm_fb_panel_data lg4573b_panel_data = {
	.on		= mipi_lg4573b_lcd_on,
	.off		= mipi_lg4573b_lcd_off,
	.set_backlight  = mipi_lg4573b_set_backlight,
};

static int ch_used[3];

int mipi_lg4573b_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD device register\n", __func__);
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_lg4573b", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	lg4573b_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lg4573b_panel_data,
		sizeof(lg4573b_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_lg4573b_lcd_init(void)
{
	if(DVE068_LCD_DEBUG)
		pr_info("[DVE068_LCD]%s LG4573B LCD init\n", __func__);
	mipi_dsi_buf_alloc(&lg4573b_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&lg4573b_rx_buf, DSI_BUF_SIZE);


	board_revision = get_m7system_board_revision();
	return platform_driver_register(&this_driver);
}

void mipi_lg4573b_disable_display(struct msm_fb_data_type *mfd){
	
	pr_info("[DVE068_LCD]%s start\n", __func__);

	cancel_delayed_work_sync(&displayon_delaywork);
	mutex_lock(&g_lcd_mutex);
	mipi_lg4573b_mfd = NULL;
	mutex_unlock(&g_lcd_mutex);
	
	if (!mfd){
		pr_info("[DVE068_LCD]%s LG4573B LCD off !mfd \n", __func__);
		return;
	}
	if (mfd->key != MFD_KEY){
		pr_info("[DVE068_LCD]%s LG4573B LCD off mfd->key != MFD_KEY \n", __func__);
		return;
	}
	if(lcd_status == 0){
		pr_info("[DVE068_LCD]%s lcd_status OFF \n", __func__);
		return;
	}
	
	mutex_lock(&mfd->dma->ov_mutex);
	mipi_set_tx_power_mode(0);
	mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_power_off_cmds,	ARRAY_SIZE(lg4573b_power_off_cmds));
	msleep(100);
	mutex_unlock(&mfd->dma->ov_mutex);

	lcd_status = 0;
	gg3_backlight_status = 0;
	pr_info("[DVE068_LCD]%s end\n", __func__);
}
void mipi_lg4573b_disable_display_dstb(struct msm_fb_data_type *mfd){
	
	pr_info("[DVE068_LCD]%s start\n", __func__);

	cancel_delayed_work_sync(&displayon_delaywork);
	mutex_lock(&g_lcd_mutex);
	mipi_lg4573b_mfd = NULL;
	mutex_unlock(&g_lcd_mutex);
	
	if (!mfd){
		pr_info("[DVE068_LCD]%s LG4573B LCD off !mfd \n", __func__);
		return;
	}
	if (mfd->key != MFD_KEY){
		pr_info("[DVE068_LCD]%s LG4573B LCD off mfd->key != MFD_KEY \n", __func__);
		return;
	}
	if(lcd_status == 0){
		pr_info("[DVE068_LCD]%s lcd_status OFF \n", __func__);
		return;
	}
	
	mutex_lock(&mfd->dma->ov_mutex);
	mipi_set_tx_power_mode(0);
	mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_power_off_cmds,	ARRAY_SIZE(lg4573b_power_off_cmds));
	msleep(100);
	
	mipi_dsi_cmds_tx(&lg4573b_tx_buf, lg4573b_dstb_on_cmds, ARRAY_SIZE(lg4573b_dstb_on_cmds));
	msleep(11);
	mutex_unlock(&mfd->dma->ov_mutex);
	
	dstb_check_flag = true;
	lcd_status = 0;
	gg3_backlight_status = 0;
	pr_info("[DVE068_LCD]%s end\n", __func__);
}

int mipi_lg4573b_user_request_ctrl( struct msmfb_request_parame *data )
{
    int ret = 0;
    
    switch( data->request )
    {
        case MSM_FB_REQUEST_OVERLAY_ALPHA:
            ret = copy_from_user(&mdp4_overlay_argb_enable, data->data, sizeof(mdp4_overlay_argb_enable));
            break;
        
        default:
            
            
            pr_info("[DVE068_LCD]%s mddi_ta8851_user_request_ctrl\n", __func__);
            break;
    }
    
    return ret;
}


void mipi_lg4573b_reg_ctrl(int on){
	
	pr_info("[DVE068_LCD]%s start on : %d \n", __func__, on);

	if(!dstb_check_flag){
		mipi_dsi_panel_power_wrap(on);
	}
	else{
		mipi_dsi_cdp_panel_power_dstb(on);
	}

	pr_info("[DVE068_LCD]%s end\n", __func__);
}







module_init(mipi_lg4573b_lcd_init);

