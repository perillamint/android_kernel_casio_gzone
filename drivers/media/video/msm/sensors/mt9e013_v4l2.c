/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include "msm_sensor.h"

#include "msm.h"
#include "msm_ispif.h"

#define SENSOR_NAME "mt9e013"
#define PLATFORM_DRIVER_NAME "msm_camera_mt9e013"
#define mt9e013_obj mt9e013_##obj

#define REG_DELAY(x) if((x) > 20) mdelay((x)); else usleep_range((x)*1000,((x)+1)*1000);


DEFINE_MUTEX(mt9e013_mut);
static struct msm_sensor_ctrl_t mt9e013_s_ctrl;



#define MT9E013_GPIO_CAM2_RST_N       76       
#define MT9E013_GPIO_CAM2_PD_N        77       
#define MT9E013_GPIO_CAM1_RST_N      107       


#define MT9E013_GPIOF_OUT_INIT_LOW_DELAY    1000
#define MT9E013_GPIOF_OUT_INIT_HIGH_DELAY   4000


#define CAM_VANA_MINUV           2800000
#define CAM_VANA_MAXUV           2850000
#define CAM_VDIG_MINUV           2800000
#define CAM_VDIG_MAXUV           2800000


#define CAM_VANA_LOAD_UA           85600
#define CAM_VDIG_LOAD_UA          105000

static struct regulator *cam_vana;
static struct regulator *cam_vio;
static struct regulator *cam_vdig;


static struct msm_camera_i2c_reg_conf mt9e013_groupon_settings[] = {
	{0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf mt9e013_groupoff_settings[] = {
	{0x0104, 0x00},
};



































































































































































































































static struct v4l2_subdev_info mt9e013_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};
























static struct msm_sensor_output_info_t mt9e013_dimensions[] = {

	{
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0x1258,
		.frame_length_lines = 0xA2F,
		.vt_pixel_clk = 185600000,
		.op_pixel_clk = 185600000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0x1196,
		.frame_length_lines = 0x563,
		.vt_pixel_clk = 185600000,
		.op_pixel_clk = 185600000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x668,
		.y_output = 0x39A,
		.line_length_pclk = 0x16B4,
		.frame_length_lines = 0x42D,
		.vt_pixel_clk = 185600000,
		.op_pixel_clk = 185600000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0x1196,
		.frame_length_lines = 0x563,
		.vt_pixel_clk = 185600000,
		.op_pixel_clk = 185600000,
		.binning_factor = 2,
	},
	{
		.x_output = 0xCD0,
		.y_output = 0x734,
		.line_length_pclk = 0x16CC,
		.frame_length_lines = 0x7C3,
		.vt_pixel_clk = 185600000,
		.op_pixel_clk = 185600000,
		.binning_factor = 1,
	},

	{
		.x_output = 0x330,
		.y_output = 0x264,
		.line_length_pclk = 0x1000,
		.frame_length_lines = 0x2F3,
		.vt_pixel_clk = 185600000,
		.op_pixel_clk = 185600000,
		.binning_factor = 1,
	},

















































};



static struct msm_camera_csid_vc_cfg mt9e013_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params mt9e013_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = mt9e013_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};













static struct msm_camera_csi2_params *mt9e013_csi_params_array[] = {
	&mt9e013_csi_params,
	&mt9e013_csi_params,
	&mt9e013_csi_params,
	&mt9e013_csi_params,
	&mt9e013_csi_params,

	&mt9e013_csi_params,

};











static struct msm_sensor_output_reg_addr_t mt9e013_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t mt9e013_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x4B00,
};

static struct msm_sensor_exp_gain_info_t mt9e013_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x305E,
	.vert_offset = 0,
};


struct reg_access_param_t{
    uint16_t  reg_addr;
    uint16_t* reg_data;
};

static int32_t mt9e013_get_exposure_info(struct msm_sensor_ctrl_t *s_ctrl, uint16_t *fine_integration_time)
{
	int32_t rc = 0;
	int cnt = 0;
	uint16_t val = 0xFFFF;

	struct reg_access_param_t reg_access_params[]={
		{0x3014, fine_integration_time},
	};

	CDBG("%s: START", __func__);

	for(cnt=0; cnt < ARRAY_SIZE(reg_access_params); cnt++)
	{
		rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
								 reg_access_params[cnt].reg_addr, &val, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			pr_err("%s: msm_camera_i2c_read failed rc=%d\n", __func__, rc);
			return rc;
		}
		*reg_access_params[cnt].reg_data = val;
	}

	CDBG("%s: fine_integration_time = 0x%04X", __func__, *fine_integration_time);

	CDBG("%s: END(%d)", __func__, rc);
	return rc;
}

static int32_t mt9e013_get_exif_param(struct msm_sensor_ctrl_t *s_ctrl,
                                      struct get_exif_param_inf *get_exif_param)
{
	return 0;
}



static int32_t mt9e013_get_device_id(struct msm_sensor_ctrl_t *s_ctrl, uint16_t *device_id)
{
	int32_t rc = 0;
	uint16_t chipid = 0xFFFF;

	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 
				s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid, 
				MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
		pr_err("%s: msm_camera_i2c_read failed rc=%d\n", __func__,rc);
		return rc;
	}

	*device_id = chipid;

	return rc;
}



















#define MODIFY_WRITE(x,y,z) (((~(y)) & (x)) | (z))
static uint16_t msm_camera_cal_bitfield(struct msm_camera_i2c_client *client, uint16_t bit_addr, uint16_t bit_mask, uint8_t bit_data)
{
	uint32_t i;
	uint32_t rc;
	uint16_t bit_num;
	uint16_t bit_val = 0;
	uint16_t read_val = 0xFFFF;
	uint16_t set_val = 0xFFFF;

	rc = msm_camera_i2c_read(
			client,
			bit_addr,
			&read_val,
			MSM_CAMERA_I2C_WORD_DATA );
	if(rc < 0){
		pr_err("msm_camera_i2c_read failed rc = %d\n", rc);
		return set_val;
	}

	for(i = 0; i < 16; i++){
		bit_num = (bit_mask >> i) & 1;
		if(bit_num){
			bit_val = (bit_mask>>i) & bit_data;
			bit_val = (bit_val<<i);
			break;
		}
	}
	CDBG("%s\n", __func__);
	CDBG("bit_addr = 0x%x, read_val = 0x%x, bit_mask = 0x%x\n", bit_addr, read_val, bit_mask);
	CDBG("bit_data = 0x%x, first bit_data = %d, bit_val = 0x%x\n", bit_data, i, bit_val);
	set_val = MODIFY_WRITE(read_val, bit_mask, bit_val);
	return set_val;
}

















void msm_camera_set_bitfield(struct msm_camera_i2c_client *client, uint16_t bit_addr, uint16_t bit_mask, uint8_t bit_data)
{
	uint16_t set_val = 0xFFFF;

	set_val = msm_camera_cal_bitfield(client, bit_addr, bit_mask, bit_data);

	CDBG("msm_camera_set_bitfield set_val = 0x%x\n", set_val);
	msm_camera_i2c_write(
			client,
			bit_addr,
			set_val,
			MSM_CAMERA_I2C_WORD_DATA);
}

static void mt9e013_prev_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_prev_settings\n");
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0300, 0x4,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0302, 0x1,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0304, 0x2,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0306, 0x3A,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0308, 0xA,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x030A, 0x1,    MSM_CAMERA_I2C_WORD_DATA);
	REG_DELAY(1);

	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0344, 0x0,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0348, 0xCD1,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0346, 0x0,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034A, 0x9A1,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034C, 0x668,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034E, 0x4D0,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x01C0, 0x3);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x003F, 0x3);
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x306E, 0x0030, 0x3);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x1000,0);   
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0200,0);   
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0400,1);   
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0800, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3178, 0x0800, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3ED0, 0x0080, 0);  
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0400, 0x0002, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0404, 0x10,   MSM_CAMERA_I2C_WORD_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0342, 0x1196, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 0x563,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 0x55F,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3014, 0x846,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3010, 0x130 , MSM_CAMERA_I2C_WORD_DATA);
}

static void mt9e013_snap_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_snap_settings\n");
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0300, 0x4 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0302, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0304, 0x2 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0306, 0x3A,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0308, 0xA ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x030A, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	REG_DELAY(1);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0344, 0x0 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0348, 0xCCF,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0346, 0x0  ,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034A, 0x99F,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034C, 0xCD0,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034E, 0x9A0,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x01C0, 0x1);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x003F, 0x1);
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x306E, 0x0030, 0x0);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x1000,0);   
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0200,0);   
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0400,0);   
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0800, 0);   
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3178, 0x0800, 0);   
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3ED0, 0x0080, 0);   
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0400, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0404, 0x10  , MSM_CAMERA_I2C_WORD_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0342, 0x1258, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 0xA2F , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 0xA2F , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3014, 0x3F6 , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3010, 0x78  , MSM_CAMERA_I2C_WORD_DATA);
}

static void mt9e013_video_HD_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_video_HD_settings\n");
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0300, 0x4 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0302, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0304, 0x2 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0306, 0x3A,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0308, 0xA ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x030A, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	REG_DELAY(1);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0344, 0x0  ,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0348, 0xCD1,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0346, 0x136,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034A, 0x86B,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034C, 0x668,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034E, 0x39A,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x01C0, 0x3);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x003F, 0x3);
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x306E, 0x0030, 0x3);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x1000, 0);  
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0200, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0400, 1);  
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0800, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3178, 0x0800, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3ED0, 0x0080, 0);  
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0400, 0x0002, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0404, 0x10  , MSM_CAMERA_I2C_WORD_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0342, 0x16B4, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 0x42D , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 0x429 , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3014, 0x846 , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3010, 0x130 , MSM_CAMERA_I2C_WORD_DATA);
}

static void mt9e013_HS_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_HS_settings\n");
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0300, 0x4 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0302, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0304, 0x2 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0306, 0x3A,   MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0308, 0xA ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x030A, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	REG_DELAY(1);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0344, 0x0  ,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0348, 0xCD1,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0346, 0x0,    MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034A, 0x9A1,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034C, 0x668,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034E, 0x4D0,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x01C0, 0x3); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x003F, 0x3); 
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x306E, 0x0030, 0x3); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x1000, 1); 
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0200, 0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0400, 1); 
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0800, 0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3178, 0x0800, 0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3ED0, 0x0080, 0); 
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0400, 0x0002, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0404, 0x10  , MSM_CAMERA_I2C_WORD_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0342, 0x1196, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 0x563 , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 0x55F , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3014, 0x846 , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3010, 0x130 , MSM_CAMERA_I2C_WORD_DATA);
}

static void mt9e013_video_FHD_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_video_FHD_settings\n");
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0300, 0x4 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0302, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0304, 0x2 ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0306, 0x3A,   MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0308, 0xA ,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x030A, 0x1 ,   MSM_CAMERA_I2C_WORD_DATA);
	REG_DELAY(1);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0344, 0x0  ,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0348, 0xCCF,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0346, 0x136,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034A, 0x869,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034C, 0xCD0,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034E, 0x734,  MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x01C0, 0x1); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x003F, 0x1); 
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x306E, 0x0030, 0x0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x1000, 1); 
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0200, 0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0400, 0); 
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0800, 0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3178, 0x0800, 0); 
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3ED0, 0x0080, 0); 
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0400, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0404, 0x10  , MSM_CAMERA_I2C_WORD_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0342, 0x16CC, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 0x7C3 , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 0x1FC , MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3014, 0x1330, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3010, 0x78  , MSM_CAMERA_I2C_WORD_DATA);
}


static void mt9e013_video_60fps_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_video_60fps_settings\n");
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0300, 0x4,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0302, 0x1,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0304, 0x2,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0306, 0x3A,   MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0308, 0xA,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x030A, 0x1,    MSM_CAMERA_I2C_WORD_DATA);
	REG_DELAY(1);

	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0344, 0x8,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0348, 0xCC1,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0346, 0x8,    MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034A, 0x991,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034C, 0x330,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x034E, 0x264,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x01C0, 0x7);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x003F, 0x7);
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x306E, 0x0030, 0x0);
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x1000,1);   
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0200,0);   
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0400,1);   
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3040, 0x0800, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3178, 0x0800, 0);  
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3ED0, 0x0080, 0);  
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0400, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0404, 0x10,   MSM_CAMERA_I2C_WORD_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0342, 0x1000,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0340, 0x2F3,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, 0x2D5,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3014, 0x846,  MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3010, 0x130 , MSM_CAMERA_I2C_WORD_DATA);
}


static void mt9e013_recommend_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	pr_info("mt9e013_recommend_settings\n");
	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x3064, 0x0100, 0);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31AE, 0x0202, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31B0, 0x0083, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31B2, 0x004D, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31B4, 0x0E56, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31B6, 0x0E14, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31B8, 0x020E, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31BA, 0x0710, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31BC, 0x2A0D, MSM_CAMERA_I2C_WORD_DATA); 
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31BE, 0x2003, MSM_CAMERA_I2C_WORD_DATA); 
	REG_DELAY(5);

	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0112, 0x0A0A, MSM_CAMERA_I2C_WORD_DATA);  

	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30F0, 0x800D, MSM_CAMERA_I2C_WORD_DATA);  
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3044, 0x0590, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x306E, 0xFC80, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30B2, 0xC000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30D6, 0x0800, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x316C, 0xB42F, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x316E, 0x869A, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3170, 0x210E, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x317A, 0x010E, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31E0, 0x1BB9, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x31E6, 0x07FC, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C0, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C2, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C4, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C6, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E00, 0x0011, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E02, 0x8801, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E04, 0x2801, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E06, 0x8449, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E08, 0x6841, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E0A, 0x400C, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E0C, 0x1001, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E0E, 0x2603, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E10, 0x4B41, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E12, 0x4B24, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E14, 0xA3CF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E16, 0x8802, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E18, 0x8401, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E1A, 0x8601, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E1C, 0x8401, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E1E, 0x840A, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E20, 0xFF00, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E22, 0x8401, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E24, 0x00FF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E26, 0x0088, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E28, 0x2E8A, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E30, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E32, 0x8801, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E34, 0x4029, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E36, 0x00FF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E38, 0x8469, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E3A, 0x00FF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E3C, 0x2801, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E3E, 0x3E2A, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E40, 0x1C01, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E42, 0xFF84, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E44, 0x8401, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E46, 0x0C01, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E48, 0x8401, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E4A, 0x00FF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E4C, 0x8402, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E4E, 0x8984, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E50, 0x6628, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E52, 0x8340, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E54, 0x00FF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E56, 0x4A42, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E58, 0x2703, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E5A, 0x6752, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E5C, 0x3F2A, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E5E, 0x846A, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E60, 0x4C01, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E62, 0x8401, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E66, 0x3901, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E90, 0x2C01, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E98, 0x2B02, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E92, 0x2A04, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E94, 0x2509, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E96, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E9A, 0x2905, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3E9C, 0x00FF, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3ECC, 0x00EB, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3ED0, 0x1E24, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3ED4, 0xAFC4, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3ED6, 0x909B, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3EE0, 0x2424, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3EE2, 0x9797, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3EE4, 0xC100, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3EE6, 0x0540, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3174, 0x8000, MSM_CAMERA_I2C_WORD_DATA);
}


static int32_t mt9e013_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines;
	fl_lines =
		(s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider) / Q10;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain | 0x1000,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

static int32_t mt9e013_write_exp_snapshot_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines;
	fl_lines =
		(s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider) / Q10;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain | 0x1000,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x301A, (0x065C|0x2), MSM_CAMERA_I2C_WORD_DATA);

	return 0;
}


static int32_t mt9e013_sensor_write_res_settings(struct msm_sensor_ctrl_t *s_ctrl,
	int res)
{
	int32_t rc;

	pr_info("mt9e013_sensor_write_res_settings res = %d\n",res);
	mt9e013_recommend_settings(s_ctrl);

	switch(res){
	case MSM_SENSOR_RES_QTR:
		mt9e013_prev_settings(s_ctrl);
		break;
	case MSM_SENSOR_RES_FULL:
		mt9e013_snap_settings(s_ctrl);
		break;
	case MSM_SENSOR_RES_2:
		mt9e013_video_HD_settings(s_ctrl);
		break;
	case MSM_SENSOR_RES_3:
		mt9e013_HS_settings(s_ctrl);
		break;
	case MSM_SENSOR_RES_4:
		mt9e013_video_FHD_settings(s_ctrl);
		break;

	case MSM_SENSOR_RES_5:
		mt9e013_video_60fps_settings(s_ctrl);
		break;

	default:
		break;
	}
	rc = msm_sensor_write_output_settings(s_ctrl, res);
	return rc;
}


static void mt9e013_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{


	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x301A, 0x200, 1);	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x301A, 0x400, 1);	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x301A, 0x8, 1);	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x00,MSM_CAMERA_I2C_WORD_DATA);	
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x301A, 0x4, 1);	













}

static void mt9e013_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{


	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x301A, 0x4, 0);		
	msm_camera_set_bitfield(s_ctrl->sensor_i2c_client, 0x301A, 0x8, 0);		
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x01,MSM_CAMERA_I2C_WORD_DATA);









}

static const struct i2c_device_id mt9e013_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&mt9e013_s_ctrl},
	{ }
};

static struct i2c_driver mt9e013_i2c_driver = {
	.id_table = mt9e013_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client mt9e013_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


static struct msm_cam_clk_info mt9e013_cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_25HZ},
};

static int32_t mt9e013_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    struct msm_camera_sensor_info *data = s_ctrl->sensordata;

    CDBG("%s START\n", __func__);
    s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
                    * data->sensor_platform_info->num_vreg, GFP_KERNEL);
    if (!s_ctrl->reg_ptr) {
        pr_err("%s: could not allocate mem for regulators\n",
            __func__);
        return -ENOMEM;
    }

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM1_RST_N, GPIOF_OUT_INIT_HIGH);
    usleep_range(MT9E013_GPIOF_OUT_INIT_HIGH_DELAY, MT9E013_GPIOF_OUT_INIT_HIGH_DELAY + 1000);
    CDBG("CAM1_RST_N(%d) High\n", MT9E013_GPIO_CAM1_RST_N);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM2_RST_N, GPIOF_OUT_INIT_HIGH);
    usleep_range(MT9E013_GPIOF_OUT_INIT_HIGH_DELAY, MT9E013_GPIOF_OUT_INIT_HIGH_DELAY + 1000);
    CDBG("CAM2_RST_N(%d) High\n", MT9E013_GPIO_CAM2_RST_N);

    
    mdelay(2);
    CDBG("WAIT %d[ms]\n",2);

    
    if (cam_vio == NULL) {
        cam_vio = regulator_get(&s_ctrl->sensor_i2c_client->client->dev, "cam_vio");
        if (IS_ERR(cam_vio)) {
            pr_err("%s: VREG CAM VIO get failed\n", __func__);
            cam_vio = NULL;
            goto cam_vio_get_failed;
        }
        if (regulator_enable(cam_vio)) {
            pr_err("%s: VREG CAM VIO enable failed\n", __func__);
            goto cam_vio_enable_failed;
        }
        CDBG("LVS5_1P8(cam_vio) ON\n");
    }

    
    mdelay(2);
    CDBG("WAIT %d[ms]\n",2);

    
    if (cam_vana == NULL) {
        cam_vana = regulator_get(&s_ctrl->sensor_i2c_client->client->dev, "cam_vana");
        if (IS_ERR(cam_vana)) {
            pr_err("%s: VREG CAM VANA get failed\n", __func__);
            cam_vana = NULL;
            goto cam_vana_get_failed;
        }
        if (regulator_set_voltage(cam_vana, CAM_VANA_MINUV, CAM_VANA_MAXUV)) {
            pr_err("%s: VREG CAM VANA set voltage failed\n", __func__);
            goto cam_vana_set_voltage_failed;
        }
        if (regulator_set_optimum_mode(cam_vana, CAM_VANA_LOAD_UA) < 0) {
            pr_err("%s: VREG CAM VANA set optimum mode failed\n", __func__);
            goto cam_vana_set_optimum_mode_failed;
        }
        if (regulator_enable(cam_vana)) {
            pr_err("%s: VREG CAM VANA enable failed\n", __func__);
            goto cam_vana_enable_failed;
        }
        CDBG("LV11_2P8(cam_vana) ON\n");
    }

    
    mdelay(2);
    CDBG("WAIT %d[ms]\n",2);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM2_RST_N, GPIOF_OUT_INIT_LOW);
    usleep_range(MT9E013_GPIOF_OUT_INIT_LOW_DELAY, MT9E013_GPIOF_OUT_INIT_LOW_DELAY + 1000);
    CDBG("CAM2_RST_N(%d) Low\n", MT9E013_GPIO_CAM2_RST_N);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    rc = msm_camera_request_gpio_table(data, 1);
    if (rc < 0) {
        pr_err("%s: request gpio failed\n", __func__);
        goto request_gpio_failed;
    }
    CDBG("I2C3_CLK/DATA_CAM ENABLE %d\n",rc);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    if (s_ctrl->clk_rate != 0)
        mt9e013_cam_clk_info->clk_rate = s_ctrl->clk_rate;

    rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
                            mt9e013_cam_clk_info,
                            &s_ctrl->cam_clk,
                            ARRAY_SIZE(mt9e013_cam_clk_info), 1);
    if (rc < 0) {
        pr_err("%s: clk enable failed\n", __func__);
        goto enable_clk_failed;
    }
    CDBG("MCLK ENABLE %d\n",rc);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM1_RST_N, GPIOF_OUT_INIT_LOW);
    usleep_range(MT9E013_GPIOF_OUT_INIT_LOW_DELAY, MT9E013_GPIOF_OUT_INIT_LOW_DELAY + 1000);
    CDBG("CAM1_RST_N(%d) Low\n", MT9E013_GPIO_CAM1_RST_N);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    if (cam_vdig == NULL) {
        cam_vdig = regulator_get(&s_ctrl->sensor_i2c_client->client->dev, "cam_vdig");
        if (IS_ERR(cam_vdig)) {
            pr_err("%s: VREG CAM VDIG get failed\n", __func__);
            cam_vdig = NULL;
            goto cam_vdig_get_failed;
        }
        if (regulator_set_voltage(cam_vdig, CAM_VDIG_MINUV, CAM_VDIG_MAXUV)) {
            pr_err("%s: VREG CAM VDIG set voltage failed\n", __func__);
            goto cam_vdig_set_voltage_failed;
        }
        if (regulator_set_optimum_mode(cam_vdig, CAM_VDIG_LOAD_UA) < 0) {
            pr_err("%s: VREG CAM VDIG set optimum mode failed\n", __func__);
            goto cam_vdig_set_optimum_mode_failed;
        }
        if (regulator_enable(cam_vdig)) {
            pr_err("%s: VREG CAM VDIG enable failed\n", __func__);
            goto cam_vdig_enable_failed;
        }
        CDBG("LV17_2P8(cam_vdig) ON\n");
    }

    
    mdelay(50);
    CDBG("WAIT %d[ms]\n",50);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM2_RST_N, GPIOF_OUT_INIT_HIGH);
    usleep_range(MT9E013_GPIOF_OUT_INIT_HIGH_DELAY, MT9E013_GPIOF_OUT_INIT_HIGH_DELAY + 1000);
    CDBG("CAM2_RST_N(%d) High\n", MT9E013_GPIO_CAM2_RST_N);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM2_PD_N, GPIOF_OUT_INIT_HIGH);
    usleep_range(MT9E013_GPIOF_OUT_INIT_HIGH_DELAY, MT9E013_GPIOF_OUT_INIT_HIGH_DELAY + 1000);
    CDBG("CAM2_PD_N(%d) High\n", MT9E013_GPIO_CAM2_PD_N);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM1_RST_N, GPIOF_OUT_INIT_HIGH);
    usleep_range(MT9E013_GPIOF_OUT_INIT_HIGH_DELAY, MT9E013_GPIOF_OUT_INIT_HIGH_DELAY + 1000);
    CDBG("CAM1_RST_N(%d) High\n", MT9E013_GPIO_CAM1_RST_N);

    
    usleep(100);
    CDBG("WAIT %d[us]\n",100);

    
    CDBG("GPIO[%d]:%d\n", MT9E013_GPIO_CAM2_RST_N, gpio_get_value(MT9E013_GPIO_CAM2_RST_N));
    CDBG("GPIO[%d]:%d\n", MT9E013_GPIO_CAM2_PD_N,  gpio_get_value(MT9E013_GPIO_CAM2_PD_N) );
    CDBG("GPIO[%d]:%d\n", MT9E013_GPIO_CAM1_RST_N, gpio_get_value(MT9E013_GPIO_CAM1_RST_N));

    CDBG("%s END(%d)", __func__, rc);
    return rc;

cam_vdig_enable_failed:
    
    regulator_set_optimum_mode(cam_vdig, 0);

cam_vdig_set_optimum_mode_failed:
    
    regulator_set_voltage(cam_vdig, 0, CAM_VDIG_MAXUV);

cam_vdig_set_voltage_failed:
    
    regulator_put(cam_vdig);
    cam_vdig = NULL;

cam_vdig_get_failed:
    
    msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
                       mt9e013_cam_clk_info,
                       &s_ctrl->cam_clk,
                       ARRAY_SIZE(mt9e013_cam_clk_info), 0);

enable_clk_failed:
    
    msm_camera_request_gpio_table(data, 0);

request_gpio_failed:
    
    regulator_disable(cam_vana);

cam_vana_enable_failed:
    
    regulator_set_optimum_mode(cam_vana, 0);

cam_vana_set_optimum_mode_failed:
    
    regulator_set_voltage(cam_vana, 0, CAM_VANA_MAXUV);

cam_vana_set_voltage_failed:
    
    regulator_put(cam_vana);
    cam_vana = NULL;

cam_vana_get_failed:
    
    regulator_disable(cam_vio);

cam_vio_enable_failed:
    
    regulator_put(cam_vio);
    cam_vio = NULL;

cam_vio_get_failed:
    kfree(s_ctrl->reg_ptr);

    pr_err("%s: END(%d) failed\n", __func__, rc);
    return rc;
}

static int mt9e013_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
    struct msm_camera_sensor_info *data = s_ctrl->sensordata;

    CDBG("%s START\n", __func__);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM1_RST_N, GPIOF_OUT_INIT_LOW);
    usleep_range(MT9E013_GPIOF_OUT_INIT_LOW_DELAY, MT9E013_GPIOF_OUT_INIT_LOW_DELAY + 1000);
    CDBG("CAM1_RST_N(%d) Low\n", MT9E013_GPIO_CAM1_RST_N);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM2_PD_N, GPIOF_OUT_INIT_LOW);
    usleep_range(MT9E013_GPIOF_OUT_INIT_LOW_DELAY, MT9E013_GPIOF_OUT_INIT_LOW_DELAY + 1000);
    CDBG("CAM2_PD_N(%d) Low\n", MT9E013_GPIO_CAM2_PD_N);

    
    mdelay(1);
    CDBG("WAIT %d[ms]\n",1);

    
    gpio_set_value_cansleep(MT9E013_GPIO_CAM2_RST_N, GPIOF_OUT_INIT_LOW);
    usleep_range(MT9E013_GPIOF_OUT_INIT_LOW_DELAY, MT9E013_GPIOF_OUT_INIT_LOW_DELAY + 1000);
    CDBG("CAM2_RST_N(%d) Low\n", MT9E013_GPIO_CAM2_RST_N);

    mdelay(5);
    CDBG("WAIT %d[ms]\n",5);

    
    msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
                       mt9e013_cam_clk_info,
                       &s_ctrl->cam_clk,
                       ARRAY_SIZE(mt9e013_cam_clk_info), 0);
    CDBG("MCLK DISABLE\n");

    
    mdelay(10);
    CDBG("WAIT %d[ms]\n",10);

    
    msm_camera_request_gpio_table(data, 0);
    CDBG("I2C3_CLK/DATA_CAM DISABLE\n");

    
    if (cam_vdig) {
        regulator_set_voltage(cam_vdig, 0, CAM_VDIG_MAXUV);
        regulator_set_optimum_mode(cam_vdig, 0);
        regulator_disable(cam_vdig);
        regulator_put(cam_vdig);
        cam_vdig = NULL;
        CDBG("LV17_2P8(cam_vdig) OFF\n");
    }

    
    mdelay(10);
    CDBG("WAIT %d[ms]\n",10);

    
    if (cam_vana) {
        regulator_set_voltage(cam_vana, 0, CAM_VANA_MAXUV);
        regulator_set_optimum_mode(cam_vana, 0);
        regulator_disable(cam_vana);
        regulator_put(cam_vana);
        cam_vana = NULL;
        CDBG("LV11_2P8(cam_vana) OFF\n");
    }

    
    mdelay(10);
    CDBG("WAIT %d[ms]\n",10);

    
    if (cam_vio) {
        regulator_disable(cam_vio);
        regulator_put(cam_vio);
        cam_vio = NULL;
        CDBG("LVS5_1P8(cam_vio) OFF\n");
    }

    kfree(s_ctrl->reg_ptr);
    CDBG("%s END(0)", __func__);

    return 0;
}


static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&mt9e013_i2c_driver);
}

static struct v4l2_subdev_core_ops mt9e013_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops mt9e013_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops mt9e013_subdev_ops = {
	.core = &mt9e013_subdev_core_ops,
	.video  = &mt9e013_subdev_video_ops,
};

static struct msm_sensor_fn_t mt9e013_func_tbl = {
	.sensor_start_stream = mt9e013_start_stream,
	.sensor_stop_stream = mt9e013_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = mt9e013_write_exp_gain,
	.sensor_write_snapshot_exp_gain = mt9e013_write_exp_snapshot_gain,


	.sensor_setting = msm_sensor_setting,





	.sensor_write_res_settings = mt9e013_sensor_write_res_settings,

	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,

	.sensor_power_up = mt9e013_sensor_power_up,
	.sensor_power_down = mt9e013_sensor_power_down,





	.sensor_get_exif_param = mt9e013_get_exif_param,


	.sensor_get_device_id = mt9e013_get_device_id,


	.sensor_get_exposure_info = mt9e013_get_exposure_info,

	.sensor_get_csi_params = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t mt9e013_regs = {


	.default_data_type = MSM_CAMERA_I2C_WORD_DATA,
	.group_hold_on_conf = mt9e013_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(mt9e013_groupon_settings),
	.group_hold_off_conf = mt9e013_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(mt9e013_groupoff_settings),
	.output_settings = &mt9e013_dimensions[0],
	.num_conf = ARRAY_SIZE(mt9e013_dimensions),














};

static struct msm_sensor_ctrl_t mt9e013_s_ctrl = {
	.msm_sensor_reg = &mt9e013_regs,
	.sensor_i2c_client = &mt9e013_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &mt9e013_reg_addr,
	.sensor_id_info = &mt9e013_id_info,
	.sensor_exp_gain_info = &mt9e013_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,


	.csi_params = &mt9e013_csi_params_array[0],




	.msm_sensor_mutex = &mt9e013_mut,
	.sensor_i2c_driver = &mt9e013_i2c_driver,
	.sensor_v4l2_subdev_info = mt9e013_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(mt9e013_subdev_info),
	.sensor_v4l2_subdev_ops = &mt9e013_subdev_ops,
	.func_tbl = &mt9e013_func_tbl,


	.clk_rate = MSM_SENSOR_MCLK_25HZ,




};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Aptina 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");


