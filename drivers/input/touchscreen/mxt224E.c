/*
 * Copyright (c) 2011 M7System Co., Ltd.
 *	
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt224E.h>
#include <asm/unaligned.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include <mach/msm_iomap.h>
#include <mach/gpio.h> 
#include <mach/irqs.h>
#include <linux/regulator/consumer.h> 
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mutex.h> 







#define OBJECT_TABLE_START_ADDRESS	7
#define OBJECT_TABLE_ELEMENT_SIZE	6

#define CMD_RESET_OFFSET		0
#define CMD_BACKUP_OFFSET		1
#define CMD_CAL_OFFSET			2


#define SUPPRESS_MSG_MASK		(1 << 1)
#define MXT_AMP 			(1 << 2)
#define MXT_VECTOR			(1 << 3)
#define MOVE_MSG_MASK		(1 << 4)
#define RELEASE_MSG_MASK	(1 << 5)
#define PRESS_MSG_MASK		(1 << 6)
#define DETECT_MSG_MASK 	(1 << 7)


#define ID_BLOCK_SIZE			7

#define SUPPRESS_MSG 			16
#define SUPPRESS_MSG_PRESS 		1



#define DEBUG_INFO	   1
#define DEBUG_VERBOSE  2
#define DEBUG_MESSAGES 5
#define DEBUG_RAW	   8
#define DEBUG_TRACE   10


#define MXT_FW_NAME 	"maxtouch.fw"

#define TOUCH_RESET_GPIO		50
#define MXT224E_TS_GPIO_IRQ		11
#define MXT224_MAX_MT_FINGERS 	5
#define MXT224E_TS_VDD 			17


#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)


#define NORMAL_CFG_F		"/system/etc/firmware/TSP_N.cfg"
#define GLOVEL_CFG_F		"/system/etc/firmware/TSP_C.cfg"
#define TOUCH_MODE_LOG_F		"/mnt/sdcard/tsmode.log"

#define TSP_LOG	0
#define MXT_I2C_RETRY_CNT	5			
#define MXT_I2C_POWER_ON_CNT	2		

#if !defined(MIN)
#define  MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

static int debug = DEBUG_INFO;
module_param(debug, int, 0644);

MODULE_PARM_DESC(debug, "Activate debugging output");


#define mxt_debug(level, ...) \
	do { \
		if (debug >= (level)) \
			pr_debug(__VA_ARGS__); \
	} while (0) 

static const u8 *object_type_name[] = {
	[0]  = "Reserved",
	[7]  = "GEN_POWERCONFIG_T7",
	[8]  = "GEN_ACQUISITIONCONFIG_T8",
	[9]  = "TOUCH_MULTITOUCHSCREEN_T9",
	[15] = "TOUCH_KEYARRAY_T15",
	[18] = "SPT_COMMSCONFIG_T18",
	[19] = "SPT_GPIOPWM_T19",
	[23] = "TOUCH_PROXIMITY_T23",
	[24] = "PROCI_ONETOUCHGESTUREPROCESSOR_T24",
	[25] = "SPT_SELFTEST_T25",
	[38] = "SPT_USERDATA_T38 ", 
	[40] = "PROCI_GRIPSUPPRESSION_T40",
	[42] = "PROCI_TOUCHSUPPRESSION_T42",
	[46] = "SPT_CTECONFIG_T46",
	[47] = "PROCI_STYLUS_T47",
	[48] = "PROCG_NOISESUPPRESSION_T48",
};

struct object_t {
	u8 object_type;
	u16 i2c_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;
} __packed;

struct finger_info {
	s16 x;
	s16 y;
	s16 z;
	s16 w;
	s16 status; 
};


struct report_id_map {
	u8  object;
	u8  instance;







	u8  first_rid;
};

enum Firm_Status_ID {
	NO_FIRM_UP = 0,
	UPDATE_FIRM_UP,
	SUCCESS_FIRM_UP,
	FAIL_FIRM_UP,
};



struct mxt_device_info {
	u8	 family_id;
	u8	 variant_id;
	u8	 major;
	u8	 minor;
	u8	 build;
	u8	 num_objs;
	u8	 x_size;
	u8	 y_size;
	char family_name[16];	 
	char variant_name[16];	  
	u16  num_nodes; 		  
};

struct mxt_data {
	struct i2c_client 		*client;
	struct input_dev 		*input_dev;
	struct early_suspend 	early_suspend;
	struct object_t 		*objects;
	u32	finger_mask;
	int gpio_read_done;
	u8 	objects_len;
	u8 	tsp_version;
	u8 	finger_type;
	u16 msg_proc;
	u16 cmd_proc;
	u16 msg_object_size;
	u32 x_dropbits:2;
	u32 y_dropbits:2;
	
	int num_fingers;
	int bytes_to_read;
	int read_fail_counter;
	u16 last_read_addr;
	u8 	check_auto_cal;
	int message_counter;
	u8 	message_size;
	u16	set_mode_for_ta;
	u16 msg_proc_addr;
	const u8 *power_cfg;
	u8  *last_message;
	bool new_msgs;
	void (*power_on)(void);
	void (*power_off)(void);
	
	spinlock_t           	lock;
	struct mutex touch_mutex; 
	wait_queue_head_t msg_queue;
	struct report_id_map *rid_map;
	struct semaphore  		msg_sem;
	struct delayed_work  	dwork;
	struct work_struct 		ta_work;
	struct work_struct 		tmr_work;
	struct timer_list 		ts_timeout_tmr;
	struct timer_list 		*p_ts_timeout_tmr;
	struct delayed_work  	config_dwork;
	struct delayed_work  	timer_dwork;
	
	struct delayed_work  	timer_get_delta;
	struct delayed_work  	firmup_dwork;
	struct delayed_work		checkchip_dwork;

    struct delayed_work		finger_dwork;
 
	struct delayed_work  	normal_dwork;
	struct delayed_work  	charger_dwork;
	struct delayed_work		wireless_dwork;
	struct delayed_work		glove_dwork;
	struct delayed_work		resume_dwork;
	struct delayed_work		charging_dwork;
	struct delayed_work     call_recovery_dwork;
	struct delayed_work     mxt_reset_dwork;
	struct delayed_work     calibrate_timer_dwork;
	struct delayed_work     suppress_timer_dwork;
	
	struct mxt_device_info	device_info;
	struct object_t  		*object_table;
	struct finger_info 		fingers[];
};

struct mxt_data *the_mxt224E;

static struct mxt_data *mxt224E_data;

static struct regulator *touch_io;

static int resume_flag = 0;
static int resume_change = 0;

static bool calibrate_timer_flag = false;
static bool calibrate_timer_wait = false;

static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				T7_IDLEACQINT, 
				T7_ACTVACQINT, 
				T7_ACTV2IDLETO};

static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				T8_CHRGTIME, 
				0,
				T8_ATCHDRIFT, 
				T8_DRIFTST, 
				T8_TCHAUTOCAL, 
				T8_SYNC, 
				T8_ATCHCALST, 
				T8_ATCHCALSTHR, 
				T8_ATCHFRCCALTHR,
				T8_ATCHFRCCALRATIO};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				T9_CTRL, 
				T9_XORIGIN,  
				T9_YORIGIN, 
				T9_XSIZE, 
				T9_YSIZE, 
				T9_AKSCFG, 
				T9_BLEN, 
				T9_TCHTHR, 
				T9_TCHDI, 
				T9_ORIENT, 
				T9_MRGTIMEOUT,	
				T9_MOVHYSTI, 
				T9_MOVHYSTN,
				T9_MOVFILTER, 
				T9_NUMTOUCH, 
				T9_MRGHYST, 
				T9_MRGTHR, 
				T9_AMPHYST, 
				T9_XRANGE_LOW,	
				T9_XRANGE_HIGH, 
				T9_YRANGE_LOW,  
				T9_YRANGE_HIGH, 			
				T9_XLOCLIP,
				T9_XHICLIP, 
				T9_YLOCLIP, 
				T9_YHICLIP, 
				T9_XEDGECTRL, 
				T9_XEDGEDIST, 
				T9_YEDGECTRL, 
				T9_YEDGEDIST, 
				T9_JUMPLIMIT,
				T9_TCHHYST,
				T9_XPITCH,
				T9_YPITCH,
				T9_NEXTTCHDI};

static u8 t15_config[] = {TOUCH_KEYARRAY_T15,
				T15_CTRL, 
				T15_XORIGIN_4KEY,
				T15_YORIGIN,
				T15_XSIZE_4KEY,
				T15_YSIZE,
				T15_AKSCFG,
				T15_BLEN,
				T15_TCHTHR,
				T15_TCHDI,
				T15_RESERVED_0,
				T15_RESERVED_1};
				

static u8 t18_config[] = {SPT_COMCONFIG_T18,
				T18_CTRL, 
				T18_COMMAND};

static u8 t19_config[] = {SPT_GPIOPWM_T19,
				T19_CTRL,  
				T19_REPORTMASK, 
				T19_DIR, 
				T19_INTPULLUP, 
				T19_OUT, 
				T19_WAKE, 
				T19_PWM, 
				T19_PERIOD, 
				T19_DUTY_0, 
				T19_DUTY_1, 
				T19_DUTY_2, 
				T19_DUTY_3, 
				T19_TRIGGER_0, 
				T19_TRIGGER_1, 
				T19_TRIGGER_2, 
				T19_TRIGGER_3}; 

static u8 t23_config[] = {TOUCH_PROXIMITY_T23,
				T23_CTRL,  
				T23_XORIGIN, 
				T23_YORIGIN, 
				T23_XSIZE, 
				T23_YSIZE, 
				T23_RESERVED, 
				T23_BLEN, 
				T23_FXDDTHR, 
				T23_FXDDTHR, 				
				T23_FXDDI, 
				T23_AVERAGE, 
				T23_MVNULLRATE, 
				T23_MVNULLRATE, 
				T23_MVDTHR, 
				T23_MVDTHR}; 



















static u8 t40_config[] = {PROCI_GRIPSUPPRESSION_T40,
				T40_CTRL,
				T40_XLOGRIP,
				T40_XHIGRIP,
				T40_YLOGRIP,
				T40_YHIGRIP};



static u8 t42_config[] = {PROCI_TOUCHSUPPRESSION_T42,
				T42_CTRL,
				T42_APPRTHR,
				T42_MAXAPPRAREA,
				T42_MAXTCHAREA,
				T42_SUPSTRENGTH,
				T42_SUPEXTTO,
				T42_MAXNUMTCHS,
				T42_SHAPESTRENGTH};


static u8 t46_config[] = {SPT_CTECONFIG_T46,
				T46_CTRL, 
				T46_MODE, 
				T46_IDLESYNCSPERX, 
				T46_ACTVSYNCSPERX, 
				T46_ADCSPERSYNC,
				T46_PULSESPERADC,
				T46_XSLEW,
				T46_SYNCDELAY,
				T46_SYNCDELAY};

static u8 t47_config[] = {PROCI_STYLUS_T47,
				T47_CTRL, 
				T47_CONTMIN, 
				T47_CONTMAX, 
				T47_STABILITY, 
				T47_MAXTCHAREA, 
				T47_AMPLTHR,
				T47_STYSHAPE,
				T47_HOVERSUP,
				T47_CONFTHR,
				T47_SYNCSPERX};


static u8 t48_config[] = {PROCG_NOISESUPPRESSION_T48,
				T48_CTRL_TA,  
				T48_CFG_TA,  
				T48_CALCFG_TA,	
				T48_BASEFREQ_TA,  
				T48_RESERVED0_TA,  
				T48_RESERVED1_TA,  
				T48_RESERVED2_TA,  
				T48_RESERVED3_TA,  
				T48_MFFREQ_2_TA,  
				T48_MFFREQ_3_TA,  
				T48_RESERVED4_TA,  
				T48_RESERVED5_TA,  
				T48_RESERVED6_TA,  
				T48_GCACTVINVLDADCS_TA, 
				T48_GCIDLEINVLDADCS_TA,  
				T48_RESERVED7_TA,  
				T48_RESERVED8_TA, 
				T48_GCMAXADCSPERX_TA,  
				T48_GCLIMITMIN_TA,	
				T48_GCLIMITMAX_TA,	
				T48_GCCOUNTMINTGT_TA,  
				T48_GCCOUNTMINTGT_TA, 
				T48_MFINVLDDIFFTHR_TA,	
				T48_MFINCADCSPXTHR_TA,	
				T48_MFINCADCSPXTHR_TA, 
				T48_MFERRORTHR_TA,	
				0,	
				T48_SELFREQMAX_TA,	
				T48_RESERVED9_TA,  
				T48_RESERVED10_TA,	
				T48_RESERVED11_TA,	
				T48_RESERVED12_TA,	
				T48_RESERVED13_TA,	
				T48_RESERVED14_TA, 
				T48_BLEN_TA,  
				T48_TCHTHR_TA,	
				T48_TCHDI_TA,  
				T48_MOVHYSTI_TA,  
				T48_MOVHYSTN_TA,  
				T48_MOVFILTER_TA,  
				T48_NUMTOUCH_TA,  
				T48_MRGHYST_TA,  
				T48_MRGTHR_TA,	
				T48_XLOCLIP_TA,  
				T48_XHICLIP_TA,  
				T48_YLOCLIP_TA,  
				T48_YHICLIP_TA,  
				T48_XEDGECTRL_TA,  
				T48_XEDGEDIST_TA,  
				T48_YEDGECTRL_TA, 
				T48_YEDGEDIST_TA,  
				T48_JUMPLIMIT_TA,  
				T48_TCHHYST_TA, 
				T48_NEXTTCHDI_TA }; 



static u8 t7_wireless_config[] = {GEN_POWERCONFIG_T7,
				T7_WIRELESS_IDLEACQINT, 
				T7_WIRELESS_ACTVACQINT, 
				T7_WIRELESS_ACTV2IDLETO};

static u8 t8_wireless_config[] = {GEN_ACQUISITIONCONFIG_T8,
				T8_WIRELESS_CHRGTIME, 
				0,
				T8_WIRELESS_ATCHDRIFT, 
				T8_WIRELESS_DRIFTST, 
				T8_WIRELESS_TCHAUTOCAL, 
				T8_WIRELESS_SYNC, 
				T8_WIRELESS_ATCHCALST, 
				T8_WIRELESS_ATCHCALSTHR, 
				T8_WIRELESS_ATCHFRCCALTHR,
				T8_WIRELESS_ATCHFRCCALRATIO};
static u8 t9_wireless_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				T9_WIRELESS_CTRL, 
				T9_WIRELESS_XORIGIN,  
				T9_WIRELESS_YORIGIN, 
				T9_WIRELESS_XSIZE, 
				T9_WIRELESS_YSIZE, 
				T9_WIRELESS_AKSCFG, 
				T9_WIRELESS_BLEN, 
				T9_WIRELESS_TCHTHR, 
				T9_WIRELESS_TCHDI, 
				T9_WIRELESS_ORIENT, 
				T9_WIRELESS_MRGTIMEOUT,	
				T9_WIRELESS_MOVHYSTI, 
				T9_WIRELESS_MOVHYSTN,
				T9_WIRELESS_MOVFILTER, 
				T9_WIRELESS_NUMTOUCH, 
				T9_WIRELESS_MRGHYST, 
				T9_WIRELESS_MRGTHR, 
				T9_WIRELESS_AMPHYST, 
				T9_WIRELESS_XRANGE_LOW,	
				T9_WIRELESS_XRANGE_HIGH, 
				T9_WIRELESS_YRANGE_LOW,  
				T9_WIRELESS_YRANGE_HIGH, 			
				T9_WIRELESS_XLOCLIP,
				T9_WIRELESS_XHICLIP, 
				T9_WIRELESS_YLOCLIP, 
				T9_WIRELESS_YHICLIP, 
				T9_WIRELESS_XEDGECTRL, 
				T9_WIRELESS_XEDGEDIST, 
				T9_WIRELESS_YEDGECTRL, 
				T9_WIRELESS_YEDGEDIST, 
				T9_WIRELESS_JUMPLIMIT,
				T9_WIRELESS_TCHHYST,
				T9_WIRELESS_XPITCH,
				T9_WIRELESS_YPITCH,
				T9_WIRELESS_NEXTTCHDI};

static u8 t15_wireless_config[] = {TOUCH_KEYARRAY_T15,
				T15_WIRELESS_CTRL, 
				T15_WIRELESS_XORIGIN_4KEY,
				T15_WIRELESS_YORIGIN,
				T15_WIRELESS_XSIZE_4KEY,
				T15_WIRELESS_YSIZE,
				T15_WIRELESS_AKSCFG,
				T15_WIRELESS_BLEN,
				T15_WIRELESS_TCHTHR,
				T15_WIRELESS_TCHDI,
				T15_WIRELESS_RESERVED_0,
				T15_WIRELESS_RESERVED_1};
				

static u8 t18_wireless_config[] = {SPT_COMCONFIG_T18,
				T18_WIRELESS_CTRL, 
				T18_WIRELESS_COMMAND};

static u8 t19_wireless_config[] = {SPT_GPIOPWM_T19,
				T19_WIRELESS_CTRL,  
				T19_WIRELESS_REPORTMASK, 
				T19_WIRELESS_DIR, 
				T19_WIRELESS_INTPULLUP, 
				T19_WIRELESS_OUT, 
				T19_WIRELESS_WAKE, 
				T19_WIRELESS_PWM, 
				T19_WIRELESS_PERIOD, 
				T19_WIRELESS_DUTY_0, 
				T19_WIRELESS_DUTY_1, 
				T19_WIRELESS_DUTY_2, 
				T19_WIRELESS_DUTY_3, 
				T19_WIRELESS_TRIGGER_0, 
				T19_WIRELESS_TRIGGER_1, 
				T19_WIRELESS_TRIGGER_2, 
				T19_WIRELESS_TRIGGER_3}; 

static u8 t23_wireless_config[] = {TOUCH_PROXIMITY_T23,
				T23_WIRELESS_CTRL,  
				T23_WIRELESS_XORIGIN, 
				T23_WIRELESS_YORIGIN, 
				T23_WIRELESS_XSIZE, 
				T23_WIRELESS_YSIZE, 
				T23_WIRELESS_RESERVED, 
				T23_WIRELESS_BLEN, 
				T23_WIRELESS_FXDDTHR, 
				T23_WIRELESS_FXDDTHR, 				
				T23_WIRELESS_FXDDI, 
				T23_WIRELESS_AVERAGE, 
				T23_WIRELESS_MVNULLRATE, 
				T23_WIRELESS_MVNULLRATE, 
				T23_WIRELESS_MVDTHR, 
				T23_WIRELESS_MVDTHR}; 



















static u8 t40_wireless_config[] = {PROCI_GRIPSUPPRESSION_T40,
				T40_WIRELESS_CTRL,
				T40_WIRELESS_XLOGRIP,
				T40_WIRELESS_XHIGRIP,
				T40_WIRELESS_YLOGRIP,
				T40_WIRELESS_YHIGRIP};



static u8 t42_wireless_config[] = {PROCI_TOUCHSUPPRESSION_T42,
				T42_WIRELESS_CTRL,
				T42_WIRELESS_APPRTHR,
				T42_WIRELESS_MAXAPPRAREA,
				T42_WIRELESS_MAXTCHAREA,
				T42_WIRELESS_SUPSTRENGTH,
				T42_WIRELESS_SUPEXTTO,
				T42_WIRELESS_MAXNUMTCHS,
				T42_WIRELESS_SHAPESTRENGTH};


static u8 t46_wireless_config[] = {SPT_CTECONFIG_T46,
				T46_WIRELESS_CTRL, 
				T46_WIRELESS_MODE, 
				T46_WIRELESS_IDLESYNCSPERX, 
				T46_WIRELESS_ACTVSYNCSPERX, 
				T46_WIRELESS_ADCSPERSYNC,
				T46_WIRELESS_PULSESPERADC,
				T46_WIRELESS_XSLEW,
				T46_WIRELESS_SYNCDELAY,
				T46_WIRELESS_SYNCDELAY};

static u8 t47_wireless_config[] = {PROCI_STYLUS_T47,
				T47_WIRELESS_CTRL, 
				T47_WIRELESS_CONTMIN, 
				T47_WIRELESS_CONTMAX, 
				T47_WIRELESS_STABILITY, 
				T47_WIRELESS_MAXTCHAREA, 
				T47_WIRELESS_AMPLTHR,
				T47_WIRELESS_STYSHAPE,
				T47_WIRELESS_HOVERSUP,
				T47_WIRELESS_CONFTHR,
				T47_WIRELESS_SYNCSPERX};


static u8 t48_wireless_config[] = {PROCG_NOISESUPPRESSION_T48,
				T48_WIRELESS_CTRL_TA,  
				T48_WIRELESS_CFG_TA,  
				T48_WIRELESS_CALCFG_TA,	
				T48_WIRELESS_BASEFREQ_TA,  
				T48_WIRELESS_RESERVED0_TA,  
				T48_WIRELESS_RESERVED1_TA,  
				T48_WIRELESS_RESERVED2_TA,  
				T48_WIRELESS_RESERVED3_TA,  
				T48_WIRELESS_MFFREQ_2_TA,  
				T48_WIRELESS_MFFREQ_3_TA,  
				T48_WIRELESS_RESERVED4_TA,  
				T48_WIRELESS_RESERVED5_TA,  
				T48_WIRELESS_RESERVED6_TA,  
				T48_WIRELESS_GCACTVINVLDADCS_TA, 
				T48_WIRELESS_GCIDLEINVLDADCS_TA,  
				T48_WIRELESS_RESERVED7_TA,  
				T48_WIRELESS_RESERVED8_TA, 
				T48_WIRELESS_GCMAXADCSPERX_TA,  
				T48_WIRELESS_GCLIMITMIN_TA,	
				T48_WIRELESS_GCLIMITMAX_TA,	
				T48_WIRELESS_GCCOUNTMINTGT_TA,  
				T48_WIRELESS_GCCOUNTMINTGT_TA, 
				T48_WIRELESS_MFINVLDDIFFTHR_TA,	
				T48_WIRELESS_MFINCADCSPXTHR_TA,	
				T48_WIRELESS_MFINCADCSPXTHR_TA, 
				T48_WIRELESS_MFERRORTHR_TA,	
				0,	
				T48_WIRELESS_SELFREQMAX_TA,	
				T48_WIRELESS_RESERVED9_TA,  
				T48_WIRELESS_RESERVED10_TA,	
				T48_WIRELESS_RESERVED11_TA,	
				T48_WIRELESS_RESERVED12_TA,	
				T48_WIRELESS_RESERVED13_TA,	
				T48_WIRELESS_RESERVED14_TA, 
				T48_WIRELESS_BLEN_TA,  
				T48_WIRELESS_TCHTHR_TA,	
				T48_WIRELESS_TCHDI_TA,  
				T48_WIRELESS_MOVHYSTI_TA,  
				T48_WIRELESS_MOVHYSTN_TA,  
				T48_WIRELESS_MOVFILTER_TA,  
				T48_WIRELESS_NUMTOUCH_TA,  
				T48_WIRELESS_MRGHYST_TA,  
				T48_WIRELESS_MRGTHR_TA,	
				T48_WIRELESS_XLOCLIP_TA,  
				T48_WIRELESS_XHICLIP_TA,  
				T48_WIRELESS_YLOCLIP_TA,  
				T48_WIRELESS_YHICLIP_TA,  
				T48_WIRELESS_XEDGECTRL_TA,  
				T48_WIRELESS_XEDGEDIST_TA,  
				T48_WIRELESS_YEDGECTRL_TA, 
				T48_WIRELESS_YEDGEDIST_TA,  
				T48_WIRELESS_JUMPLIMIT_TA,  
				T48_WIRELESS_TCHHYST_TA, 
				T48_WIRELESS_NEXTTCHDI_TA }; 



static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224E_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t15_config,
	t18_config,
	t19_config,
	t23_config,
	t40_config,
	t42_config,
	t46_config,
	t47_config,
	t48_config,
	end_config,
};

const u8 *mxt224E_wireless_config[] = {
	t7_wireless_config,
	t8_wireless_config,
	t9_wireless_config,
	t15_wireless_config,
	t18_wireless_config,
	t19_wireless_config,
	t23_wireless_config,
	t40_wireless_config,
	t42_wireless_config,
	t46_wireless_config,
	t47_wireless_config,
	t48_wireless_config,	
	end_config,
};


#define CONFIG_MAX 250
static u8 normal_cfg_config[CONFIG_MAX][3];
static u8 glove_cfg_config[CONFIG_MAX][3];
static u8 charger_cfg_config[CONFIG_MAX][3];
static u8 wireless_cfg_config[CONFIG_MAX][3];

static u8 normal_cfg_count;
static u8 glove_cfg_count;
static u8 charger_cfg_count;
static u8 wireless_cfg_count;
static u8 normal_cfg_max;
static u8 glove_cfg_max;
static u8 charger_cfg_max;
static u8 wireless_cfg_max;

#define ATCHFRCCALTHR_NOMAL 127
#define ATCHFRCCALRATIO_NORMAL 127
#define ATCHFRCCALTHR_CALIBRATE 16
#define ATCHFRCCALRATIO_CALIBRATE 1
struct wake_lock        touch_wake_lock;


static unsigned int proximity_status = 0;



u8 booting_initial_config[179][3]=
{
	{7,0,64},{7,1,255},{7,2,50},
	
	{8,0,24},{8,1,0},{8,2,5},{8,3,1},{8,4,0},{8,5,0},{8,6,0},{8,7,1},{8,8,16},{8,9,1},
	
	{9,0,139},{9,1,0},{9,2,0},{9,3,19},{9,4,11},{9,5,0},{9,6,16},{9,7,30},{9,8,2},
	{9,9,3},{9,10,0},{9,11,3},{9,12,1},{9,13,1},{9,14,5},{9,15,10},{9,16,20},{9,17,20},
	{9,18,255},{9,19,3},{9,20,255},{9,21,3},{9,22,0},{9,23,0},{9,24,0},{9,25,0},{9,26,33},
	{9,27,15},{9,28,36},{9,29,13},{9,30,20},{9,31,20},{9,32,0},{9,33,0},{9,34,1},
	
	{15,0,0},{15,1,0},{15,2,0},{15,3,0},{15,4,0},{15,5,0},{15,6,0},{15,7,0},{15,8,0},
	{15,9,0},{15,10,0},
	
	{18,0,0},{18,1,0},
	
	{19,0,0},{19,1,0},{19,2,0},{19,3,0},{19,4,0},{19,5,0},
	{19,6,0},{19,7,0},{19,8,0},{19,9,0},{19,10,0},{19,11,0},
	{19,12,0},{19,13,0},{19,14,0},{19,15,0},
	
	{23,0,0},{23,1,0},{23,2,0},{23,3,0},{23,4,0},{23,5,0},{23,6,0},{23,7,0},{23,8,0},
	{23,9,0},{23,10,0},{23,11,0},{23,12,0},{23,13,0},{23,14,0},
	
	{25,0,0},{25,1,0},{25,2,0},{25,3,0},{25,4,0},{25,5,0},{25,6,0},{25,7,0},
	
	{38,0,0},{38,1,0},{38,2,0},{38,3,0},{38,4,0},{38,5,0},{38,6,0},{38,7,0},
	
	{40,0,0},{40,1,0},{40,2,0},{40,3,0},{40,4,0},

	{42,0,3},{42,1,20},{42,2,40},{42,3,35},{42,4,224},{42,5,0},{42,6,8},{42,7,0},
	
	{46,0,0},{46,1,3},{46,2,16},{46,3,32},{46,4,0},{46,5,0},{46,6,1},{46,7,0},{46,8,0},
	
	{47,0,0},{47,1,0},{47,2,0},{47,3,0},{47,4,0},{47,5,0},{47,6,0},{47,7,0},{47,8,0},{47,9,0},
	
	{48,0,23},{48,1,0},{48,2,96},{48,3,0},{48,8,0},{48,9,0},{48,6,0},{48,7,0},{48,13,6},
	{48,14,6},{48,17,100},{48,18,6},{48,19,64},{48,20,10},{48,21,0},{48,22,20},{48,23,42},
	{48,24,0},{48,27,10},{48,34,0},{48,35,100},{48,36,2},{48,37,3},{48,38,1},{48,39,0},
	{48,40,5},{48,41,10},{48,42,20},{48,43,0},{48,44,0},{48,45,0},{48,46,0},{48,47,33},
	{48,48,15},{48,49,36},{48,50,13},{48,51,30},{48,52,20},{48,53,0},
};

static u8 normal_booting_config[178][3]=
{
	{7,0,64},{7,1,255},{7,2,50},
	
	{8,0,24},{8,1,0},{8,2,5},{8,3,1},{8,4,0},{8,5,0},{8,6,0},{8,7,1},{8,8,16},{8,9,1},
	
	{9,0,139},{9,1,0},{9,2,0},{9,3,19},{9,4,11},{9,5,0},{9,6,16},{9,7,30},{9,8,2},
	{9,9,3},{9,10,0},{9,11,3},{9,12,1},{9,13,1},{9,14,5},{9,15,10},{9,16,20},{9,17,20},
	{9,18,255},{9,19,3},{9,20,255},{9,21,3},{9,22,0},{9,23,0},{9,24,0},{9,25,0},{9,26,33},
	{9,27,15},{9,28,36},{9,29,13},{9,30,20},{9,31,20},{9,32,0},{9,33,0},{9,34,1},
	
	{15,0,0},{15,1,0},{15,2,0},{15,3,0},{15,4,0},{15,5,0},{15,6,0},{15,7,0},{15,8,0},
	{15,9,0},{15,10,0},
	
	{18,0,0},{18,1,0},
	
	{19,0,0},{19,1,0},{19,2,0},{19,3,0},{19,4,0},{19,5,0},
	{19,6,0},{19,7,0},{19,8,0},{19,9,0},{19,10,0},{19,11,0},
	{19,12,0},{19,13,0},{19,14,0},{19,15,0},
	
	{23,0,0},{23,1,0},{23,2,0},{23,3,0},{23,4,0},{23,5,0},{23,6,0},{23,7,0},{23,8,0},
	{23,9,0},{23,10,0},{23,11,0},{23,12,0},{23,13,0},{23,14,0},
	
	{40,0,0},{40,1,0},{40,2,0},{40,3,0},{40,4,0},

	{42,0,3},{42,1,20},{42,2,40},{42,3,35},{42,4,224},{42,5,0},{42,6,8},{42,7,0},
	
	{46,0,0},{46,1,3},{46,2,16},{46,3,32},{46,4,0},{46,5,0},{46,6,1},{46,7,0},{46,8,0},
	
	{47,0,0},{47,1,0},{47,2,0},{47,3,0},{47,4,0},{47,5,0},{47,6,0},{47,7,0},{47,8,0},{47,9,0},
	
	{48,0,23},{48,1,0},{48,2,96},{48,3,0},{48,4,0},{48,5,0},{48,6,0},{48,7,0},{48,8,0},{48,9,0},
	{48,10,0},{48,11,0},{48,12,0},{48,13,6},{48,14,6},{48,15,0},{48,16,0},{48,17,100},{48,18,6},
	{48,19,64},{48,20,10},{48,21,0},{48,22,20},{48,23,42},{48,24,0},{48,25,0},{48,26,0},{48,27,10},
	{48,28,0},{48,29,0},{48,30,0},{48,31,0},{48,32,0},{48,33,0},
	{48,34,0},{48,35,100},{48,36,2},{48,37,3},{48,38,1},{48,39,0},
	{48,40,5},{48,41,10},{48,42,20},{48,43,0},{48,44,0},{48,45,0},{48,46,0},{48,47,33},
	{48,48,15},{48,49,36},{48,50,13},{48,51,30},{48,52,20},{48,53,0},
};

static u8 glove_temp_config[178][3]=
{
	{7,0,64},{7,1,255},{7,2,50},
	
	{8,0,24},{8,1,0},{8,2,5},{8,3,1},{8,4,0},{8,5,0},{8,6,0},{8,7,1},{8,8,16},{8,9,1},
	
	{9,0,139},{9,1,0},{9,2,0},{9,3,19},{9,4,11},{9,5,0},{9,6,32},{9,7,25},{9,8,5},
	{9,9,3},{9,10,0},{9,11,3},{9,12,1},{9,13,1},{9,14,1},{9,15,10},{9,16,20},{9,17,20},
	{9,18,255},{9,19,3},{9,20,255},{9,21,3},{9,22,0},{9,23,0},{9,24,0},{9,25,0},{9,26,33},
	{9,27,15},{9,28,36},{9,29,13},{9,30,20},{9,31,20},{9,32,0},{9,33,0},{9,34,3},
	
	{15,0,0},{15,1,0},{15,2,0},{15,3,0},{15,4,0},{15,5,0},{15,6,0},{15,7,0},{15,8,0},
	{15,9,0},{15,10,0},
	
	{18,0,0},{18,1,0},
	
	{19,0,0},{19,1,0},{19,2,0},{19,3,0},{19,4,0},{19,5,0},
	{19,6,0},{19,7,0},{19,8,0},{19,9,0},{19,10,0},{19,11,0},
	{19,12,0},{19,13,0},{19,14,0},{19,15,0},
	
	{23,0,0},{23,1,0},{23,2,0},{23,3,0},{23,4,0},{23,5,0},{23,6,0},{23,7,0},{23,8,0},
	{23,9,0},{23,10,0},{23,11,0},{23,12,0},{23,13,0},{23,14,0},
	
	{40,0,0},{40,1,0},{40,2,0},{40,3,0},{40,4,0},

	{42,0,2},{42,1,20},{42,2,40},{42,3,35},{42,4,224},{42,5,0},{42,6,8},{42,7,0},
	
	{46,0,0},{46,1,3},{46,2,32},{46,3,63},{46,4,0},{46,5,0},{46,6,1},{46,7,0},{46,8,0},
	
	{47,0,1},{47,1,50},{47,2,255},{47,3,5},{47,4,4},{47,5,35},{47,6,40},{47,7,255},{47,8,0},{47,9,40},
	
	{48,0,23},{48,1,0},{48,2,96},{48,3,0},{48,4,0},{48,5,0},{48,6,0},{48,7,0},{48,8,0},{48,9,0},
	{48,10,0},{48,11,0},{48,12,0},{48,13,6},{48,14,6},{48,15,0},{48,16,0},{48,17,100},{48,18,6},
	{48,19,64},{48,20,10},{48,21,0},{48,22,20},{48,23,42},{48,24,0},{48,25,0},{48,26,0},{48,27,10},
	{48,28,0},{48,29,0},{48,30,0},{48,31,0},{48,32,0},{48,33,0},
	{48,34,0},{48,35,100},{48,36,2},{48,37,3},{48,38,1},{48,39,0},
	{48,40,5},{48,41,10},{48,42,20},{48,43,0},{48,44,0},{48,45,0},{48,46,0},{48,47,33},
	{48,48,15},{48,49,36},{48,50,13},{48,51,30},{48,52,20},{48,53,0},
};

static u8 charger_temp_config[178][3]=
{
	{7,0,255},{7,1,255},{7,2,50},
	
	{8,0,24},{8,1,0},{8,2,5},{8,3,1},{8,4,0},{8,5,0},{8,6,0},{8,7,1},{8,8,16},{8,9,1},
	
	{9,0,139},{9,1,0},{9,2,0},{9,3,19},{9,4,11},{9,5,0},{9,6,16},{9,7,50},{9,8,2},
	{9,9,3},{9,10,0},{9,11,3},{9,12,1},{9,13,1},{9,14,5},{9,15,10},{9,16,20},{9,17,20},
	{9,18,255},{9,19,3},{9,20,255},{9,21,3},{9,22,0},{9,23,0},{9,24,0},{9,25,0},{9,26,33},
	{9,27,15},{9,28,36},{9,29,13},{9,30,20},{9,31,20},{9,32,0},{9,33,0},{9,34,1},
	
	{15,0,0},{15,1,0},{15,2,0},{15,3,0},{15,4,0},{15,5,0},{15,6,0},{15,7,0},{15,8,0},
	{15,9,0},{15,10,0},
	
	{18,0,0},{18,1,0},
	
	{19,0,0},{19,1,0},{19,2,0},{19,3,0},{19,4,0},{19,5,0},
	{19,6,0},{19,7,0},{19,8,0},{19,9,0},{19,10,0},{19,11,0},
	{19,12,0},{19,13,0},{19,14,0},{19,15,0},
	
	{23,0,0},{23,1,0},{23,2,0},{23,3,0},{23,4,0},{23,5,0},{23,6,0},{23,7,0},{23,8,0},
	{23,9,0},{23,10,0},{23,11,0},{23,12,0},{23,13,0},{23,14,0},
	
	{40,0,0},{40,1,0},{40,2,0},{40,3,0},{40,4,0},

	{42,0,3},{42,1,20},{42,2,40},{42,3,35},{42,4,224},{42,5,0},{42,6,8},{42,7,0},
	
	{46,0,0},{46,1,3},{46,2,16},{46,3,32},{46,4,0},{46,5,0},{46,6,1},{46,7,0},{46,8,0},
	
	{47,0,0},{47,1,0},{47,2,0},{47,3,0},{47,4,0},{47,5,0},{47,6,0},{47,7,0},{47,8,0},{47,9,0},
	
	{48,0,23},{48,1,0},{48,2,112},{48,3,0},{48,4,0},{48,5,0},{48,6,0},{48,7,50},{48,8,0},{48,9,0},
	{48,10,0},{48,11,0},{48,12,0},{48,13,6},{48,14,6},{48,15,0},{48,16,0},{48,17,100},{48,18,6},
	{48,19,64},{48,20,10},{48,21,0},{48,22,20},{48,23,42},{48,24,0},{48,25,0},{48,26,0},{48,27,10},
	{48,28,0},{48,29,0},{48,30,0},{48,31,0},{48,32,0},{48,33,0},
	{48,34,0},{48,35,80},{48,36,3},{48,37,3},{48,38,1},{48,39,0},
	{48,40,5},{48,41,10},{48,42,20},{48,43,0},{48,44,0},{48,45,0},{48,46,0},{48,47,33},
	{48,48,15},{48,49,36},{48,50,13},{48,51,15},{48,52,20},{48,53,0},
};

static u8 wireless_charger_config[178][3]=
{
	{7,0,64},{7,1,255},{7,2,50},
	
	{8,0,24},{8,1,0},{8,2,5},{8,3,1},{8,4,0},{8,5,0},{8,6,0},{8,7,1},{8,8,127},{8,9,127},
	
	{9,0,139},{9,1,0},{9,2,0},{9,3,19},{9,4,11},{9,5,0},{9,6,16},{9,7,50},{9,8,2},
	{9,9,3},{9,10,0},{9,11,3},{9,12,1},{9,13,1},{9,14,5},{9,15,10},{9,16,20},{9,17,20},
	{9,18,255},{9,19,3},{9,20,255},{9,21,3},{9,22,0},{9,23,0},{9,24,0},{9,25,0},{9,26,33},
	{9,27,15},{9,28,36},{9,29,13},{9,30,20},{9,31,20},{9,32,0},{9,33,0},{9,34,1},
	
	{15,0,0},{15,1,0},{15,2,0},{15,3,0},{15,4,0},{15,5,0},{15,6,0},{15,7,0},{15,8,0},
	{15,9,0},{15,10,0},
	
	{18,0,0},{18,1,0},
	
	{19,0,0},{19,1,0},{19,2,0},{19,3,0},{19,4,0},{19,5,0},
	{19,6,0},{19,7,0},{19,8,0},{19,9,0},{19,10,0},{19,11,0},
	{19,12,0},{19,13,0},{19,14,0},{19,15,0},
	
	{23,0,0},{23,1,0},{23,2,0},{23,3,0},{23,4,0},{23,5,0},{23,6,0},{23,7,0},{23,8,0},
	{23,9,0},{23,10,0},{23,11,0},{23,12,0},{23,13,0},{23,14,0},
	
	{40,0,0},{40,1,0},{40,2,0},{40,3,0},{40,4,0},

	{42,0,3},{42,1,20},{42,2,40},{42,3,35},{42,4,224},{42,5,0},{42,6,8},{42,7,0},
	
	{46,0,0},{46,1,3},{46,2,32},{46,3,40},{46,4,0},{46,5,0},{46,6,2},{46,7,0},{46,8,0},
	
	{47,0,0},{47,1,0},{47,2,0},{47,3,0},{47,4,0},{47,5,0},{47,6,0},{47,7,0},{47,8,0},{47,9,0},
	
	{48,0,23},{48,1,0},{48,2,114},{48,3,4},{48,4,0},{48,5,0},{48,6,0},{48,7,0},{48,8,10},{48,9,20},
	{48,10,0},{48,11,0},{48,12,0},{48,13,6},{48,14,6},{48,15,0},{48,16,0},{48,17,100},{48,18,6},
	{48,19,64},{48,20,10},{48,21,0},{48,22,20},{48,23,42},{48,24,0},{48,25,0},{48,26,0},{48,27,5},
	{48,28,0},{48,29,0},{48,30,0},{48,31,0},{48,32,0},{48,33,0},
	{48,34,0},{48,35,56},{48,36,2},{48,37,3},{48,38,1},{48,39,1},
	{48,40,5},{48,41,10},{48,42,40},{48,43,0},{48,44,0},{48,45,0},{48,46,0},{48,47,33},
	{48,48,15},{48,49,36},{48,50,13},{48,51,30},{48,52,20},{48,53,0},
};

typedef struct { 
	uint8_t ctrl;                
	uint8_t cfg;                 
	uint8_t calcfg;              
	uint8_t basefreq;            
	uint8_t freq_0;              
	uint8_t freq_1;              
	uint8_t freq_2;              
	uint8_t freq_3;              
	uint8_t mffreq_2;            
	uint8_t mffreq_3;            
	uint8_t nlgain;              
	uint8_t nlthr;               
	uint8_t gclimit;             
	uint8_t gcactvinvldadcs;     
	uint8_t gcidleinvldadcs;     
	uint16_t gcinvalidthr;        
	uint8_t gcmaxadcsperx;       
	uint8_t gclimitmin ;
	uint8_t gclimitmax ;
	uint16_t gccountmintgt ;
	uint8_t mfinvlddiffthr ;
	uint16_t mfincadcspxthr ;
	uint16_t mferrorthr ;
	uint8_t selfreqmax ;
	uint8_t reserved9 ;
	uint8_t reserved10 ;
	uint8_t reserved11 ;
	uint8_t reserved12 ;
	uint8_t reserved13 ;
	uint8_t reserved14 ;
	uint8_t blen ;
	uint8_t tchthr ;
	uint8_t tchdi ;
	uint8_t movhysti ;
	uint8_t movhystn ;
	uint8_t movfilter ;
	uint8_t numtouch ;
	uint8_t mrghyst ;
	uint8_t mrgthr ;
	uint8_t xloclip ;
	uint8_t xhiclip ;
	uint8_t yloclip ;
	uint8_t yhiclip ;
	uint8_t xedgectrl ;
	uint8_t xedgedist ;
	uint8_t yedgectrl ;
	uint8_t yedgedist ;
	uint8_t jumplimit ;
	uint8_t tchhyst ;
	uint8_t nexttchdi ;
} __packed procg_noisesuppression_t48_config_t;

static procg_noisesuppression_t48_config_t		noisesuppression_t48_config = { 0 };

enum {
	DISABLE,
	ENABLE
};

static bool palm_check_timer_flag = false;
static bool palm_release_flag = true;

static bool cal_check_flag;
static u8 facesup_message_flag=0 ;
static u8 facesup_message_flag_T9 ;
static bool timer_flag = DISABLE;
static uint8_t timer_ticks = 0;
static unsigned int mxt_time_point;
static unsigned int time_after_autocal_enable = 0;

static u8 coin_check_count = 0;
static bool metal_suppression_chk_flag = true;
unsigned char not_yet_count = 0;
extern unsigned int ts_lcd_flag; 

static void check_chip_calibration(struct mxt_data *mxt);
static void cal_maybe_good(struct mxt_data *mxt);
static int calibrate_chip(struct mxt_data *mxt);
static void mxt_palm_recovery(struct work_struct *work);
static void check_chip_palm(struct mxt_data *mxt);

static struct workqueue_struct *ts_100s_tmr_workqueue;
static void ts_100ms_timeout_handler(unsigned long data);
static void ts_100ms_timer_start(struct mxt_data *mxt);
static void ts_100ms_timer_stop(struct mxt_data *mxt);
static void ts_100ms_timer_init(struct mxt_data *mxt);
static void ts_100ms_tmr_work(struct work_struct *work);

static void mxt_release_fingers(struct mxt_data *data);
static void mxt_release_all_keys(struct mxt_data *data);


static int finger_recovery(struct mxt_data *data);

static int mxt_call_recovery(struct mxt_data *data);

#define ABS(x,y)		( (x < y) ? (y - x) : (x - y))


enum{
    TOUCH_NOT_CHARGE = 0,
    TOUCH_WALL_CHARGE,
    TOUCH_USB_CHARGE,
    TOUCH_CRADLE_CHARGE,
    TOUCH_WIRELESS_CHARGE,
};

#define TOUCH_NORMAL 	0x0
#define TOUCH_GLOVE 	0x1
#define TOUCH_CHARGE 	0x2
#define TOUCH_WIRELESS 	0x4
#define SET_NOW			0x11


#define MAX_FINGER_RECOVERY_CNT 10



#define MXT_BASE_ADDR(object_type) \
get_object_address(mxt, object_type, 0, mxt->object_table, mxt->device_info.num_objs)


#define MXT_GET_SIZE(object_type) \
get_object_size(mxt, object_type, mxt->object_table, mxt->device_info.num_objs)


#define	REPORT_ID_TO_OBJECT(rid) \
(((rid) == 0xff) ? 0 : mxt->rid_map[rid].object)

#define	REPORT_ID_TO_OBJECT_NAME(rid) \
object_type_name[REPORT_ID_TO_OBJECT(rid)]


#define REPORT_MT(touch_number, x, y, amplitude, size)		\
	do {														\
	input_report_abs(mxt->input_dev, ABS_MT_TRACKING_ID, touch_number); \
	input_report_abs(mxt->input_dev, ABS_MT_POSITION_X, x);             \
	input_report_abs(mxt->input_dev, ABS_MT_POSITION_Y, y);             \
	input_report_abs(mxt->input_dev, ABS_MT_TOUCH_MAJOR, amplitude);    \
	input_report_abs(mxt->input_dev, ABS_MT_WIDTH_MAJOR, size);         \
	input_mt_sync(mxt->input_dev);                                      \
	} while (0)

int touch_mode=0;

int last_charge_mode=0;
int last_touch_mode=0;

int charge_mode=0;

int last_apply_mode = -1;

static int mxt_suspend_status = 0;
static int mxt_swreset_status = 0;
static int mxt_swreset_cancel = 0;
static int mxt_writecfg_status = 0;

static int calibration_complete_count=0;
static int finger_check=0;


static int ts_store_obj_status=0;

#define MAX_MSG_LOG_SIZE	512

int gg3_menukey_status = 0;

extern int pm_power_get_charger_mode(void);
extern int GET_WIRELESS_STATUS;
extern int pm8921_get_prop_battery_uvolts(void); 
enum{
	NOT_CHARGE = 0,
	WALL_CHARGE,
	USB_CHARGE,
	CRADLE_CHARGE,
	WIRELESS_CHARGE,
};


struct  {
	u8  id[MAX_MSG_LOG_SIZE];
	u8  status[MAX_MSG_LOG_SIZE];
	u16  xpos[MAX_MSG_LOG_SIZE];
	u16  ypos[MAX_MSG_LOG_SIZE];
	u8   area[MAX_MSG_LOG_SIZE];
	u8   amp[MAX_MSG_LOG_SIZE];
	u16 cnt;
}msg_log;

struct  tch_msg_t{
	u8   id;
	u8   status[10];
	u16  xpos[10];
	u16  ypos[10];
	u8   area[10];
	u8   amp[10];
};

struct tch_msg_t new_touch;
struct tch_msg_t old_touch;

struct  {
	s16 length[5];
	u8  angle[5];
	u8  cnt;
}tch_vct[10];

static int read_mem(struct mxt_data *data, u16 reg, u8 len, u8 *buf)
{
	int ret;
	int retry_cnt = 0;				
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.len = 2,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};







    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		ret = i2c_transfer(data->client->adapter, msg, 2);
		if (ret < 0){
			printk(KERN_ERR "%s: i2c_transfer error(%d) ret:%x\n",__func__, retry_cnt, ret);
		}
		else{
			break;
		}
		
	}

	if( retry_cnt >= MXT_I2C_RETRY_CNT )
	{
		printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x00\n");
		return ret;
	}


	return ret == 2 ? 0 : -EIO;
}

static int write_mem(struct mxt_data *data, u16 reg, u8 len, const u8 *buf)
{
	int ret;
	int retry_cnt = 0;				
	u8 tmp[len + 2];

	put_unaligned_le16(cpu_to_le16(reg), tmp);
	memcpy(tmp + 2, buf, len);








    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		ret = i2c_master_send(data->client, tmp, sizeof(tmp));
		if (ret < 0){
			printk(KERN_ERR "%s: i2c_master_send error(%d) ret:%x\n",__func__, retry_cnt, ret);
		}
		else{
			break;
		}
		
	}

	if( retry_cnt >= MXT_I2C_RETRY_CNT )
	{
		printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x01\n");
		return ret;
	}


	return ret == sizeof(tmp) ? 0 : -EIO;
}

static int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value)
{ 
	struct { 
		__le16 le_addr;
		u8 data;

	} i2c_byte_transfer;
	int retry_cnt = 0;				

	struct mxt_data *mxt;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	i2c_byte_transfer.le_addr = cpu_to_le16(addr);
	i2c_byte_transfer.data = value;









    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if  (i2c_master_send(client, (u8 *) &i2c_byte_transfer, 3) == 3)
			return 0;
		else{
			printk(KERN_ERR "%s: i2c_master_send error(%d) \n",__func__,retry_cnt);
		}
		
	}

	printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x02\n");
	return -EIO;


}

static int mxt_read_byte(struct i2c_client *client, u16 addr, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16 le_addr = cpu_to_le16(addr);
	struct mxt_data *mxt;
	int retry_cnt = 0;				

	mxt = i2c_get_clientdata(client);


	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len	 = 2;
	msg[0].buf	 = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len	 = 1;
	msg[1].buf	 = (u8 *) value;
















    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if	(i2c_transfer(adapter, msg, 2) == 2) {
			mxt->last_read_addr = addr;
			return 0;
		} else {
			



			 printk(KERN_ERR "%s: i2c_transfer error(%d) \n",__func__,retry_cnt);
		}
	}

	printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x03\n");
	mxt->last_read_addr = -1;
	return -EIO;


}

static int mxt_read_block(struct i2c_client *client,u16 addr,u16 length,u8 *value)
{ 
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	__le16	le_addr;
	struct mxt_data *mxt;
	int retry_cnt = 0;				

	mxt = i2c_get_clientdata(client);

	if (mxt != NULL) { 
		if ((mxt->last_read_addr == addr) &&
			(addr == mxt->msg_proc)) { 
			if  (i2c_master_recv(client, value, length) == length)
				return 0;
			else
				return -EIO;
		} else { 
			mxt->last_read_addr = addr;
		}
	}

	le_addr = cpu_to_le16(addr);
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &le_addr;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;








    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if  (i2c_transfer(adapter, msg, 2) == 2)
			return 0;
		else
			printk(KERN_ERR "%s: i2c_transfer error(%d) \n",__func__,retry_cnt);
	}

	return -EIO;


}

static u16 get_object_address(struct mxt_data *data, uint8_t object_type,uint8_t instance,struct object_t *object_table,int max_objs)
{
	uint16_t address = 0;
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			address = data->objects[i].i2c_address;
			return address;
		}
	}

	return address;
}


u16 get_object_size(struct mxt_data *data, uint8_t object_type, struct object_t *object_table, int max_objs)
{ 
	uint16_t size = 0;
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			size = data->objects[i].size + 1;
			return size;
		}
	}
	
	return size;
}

static int get_object_info(struct mxt_data *data, u8 object_type, u16 *size, u16 *address)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			*size = data->objects[i].size + 1;
			*address = data->objects[i].i2c_address;
			return 0;
		}
	}

	return -ENODEV;
}

static int __devinit mxt224E_reset(struct mxt_data *data)
{
	u8 buf = 0x1u;
	return write_mem(data, data->cmd_proc + CMD_RESET_OFFSET, 1, &buf);
}











static int mxt224E_backup(struct mxt_data *data)

{
	u8 buf = 0x55u;



	int result;

	result = write_mem(data, data->cmd_proc + CMD_BACKUP_OFFSET, 1, &buf);
	mdelay(25);
	return result;

}









static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	u8 buf[3];
	int retry_cnt = 0;				

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	buf[2] = val;








    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if (i2c_master_send(client, buf, 3) != 3) {
			dev_err(&client->dev, "%s: i2c send failed(%d)\n", __func__,retry_cnt);
		}
		else{
			break;
		}
		
	}

	if( retry_cnt >= MXT_I2C_RETRY_CNT )
	{
		printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x05\n");
		return -EIO;
	}

	return 0;
}































int mxt_write_block(struct i2c_client *client,u16 addr,u16 length,u8 *value)
{ 
	int i;
	int retry_cnt = 0;				
	struct {
		__le16	le_addr;
		u8	data[256];

	} i2c_block_transfer;

	struct mxt_data *mxt;

	if (length > 256)
		return -EINVAL;

	mxt = i2c_get_clientdata(client);
	if (mxt != NULL)
		mxt->last_read_addr = -1;

	for (i = 0; i < length; i++)
		i2c_block_transfer.data[i] = *value++;

	i2c_block_transfer.le_addr = cpu_to_le16(addr);











    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);

		if (i == (length + 2))
			return length;
		else
			printk(KERN_ERR "%s: i2c_master_send error(%d) i:%x\n",__func__, retry_cnt, i);
		
	}

	printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x06\n");
	return -EIO;

}

static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	int ret;
	u16 address;
	u16 size;

	ret = get_object_info(data, type, &size, &address);

	if (ret)
		return ret;

	return mxt_write_reg(data->client, address + offset, val);
}
















static void mxt_sw_reset(struct mxt_data *data)
{
	if(!mxt_suspend_status)
		mxt_write_object(data, 6, 0, 1);
}

static int mxt_power_on( bool on )
{
	int rc;

	if(on == true) {
		rc = regulator_enable(touch_io);
		if (rc) {
			pr_err("enable touch_io failed, rc=%d\n", rc);
			return -ENODEV;
		}
	}
	else {
		rc = regulator_disable(touch_io);
			if (rc) {
				pr_err("enable touch_io failed, rc=%d\n", rc);
				return -ENODEV;
		}
	}

	return 0;
}

static int write_config(struct mxt_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address;
	u16 size;

	ret = get_object_info(data, type, &size, &address);

	if (ret)
		return ret;

	return write_mem(data, address, size, cfg);
}

static void write_temp_config(struct mxt_data *mxt, u8 type, u8 offset, u8 val)
{
	u16 addr = 0;
	u16 size = 0;	
	u8 temp_val = 0;
	u8 count = 0;
	int ret;
	
	get_object_info(mxt, type, &size, &addr);

	do
	{
		ret = mxt_write_byte(mxt->client, addr+(u16)offset, (u8)val);
		ret |= mxt_read_byte(mxt->client,addr+(u16)offset,&temp_val);
		count++;
		if(ret && (count==3)) printk("%s failed\n",__FUNCTION__);
	}while(val != temp_val && count < 3);

	

	return;
}


static u32 __devinit crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16)byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32)data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}



static int calculate_infoblock_crc(struct mxt_data *data, u32 *crc_pointer)					

{
	u32 crc = 0;
	u8 mem[7 + data->objects_len * 6];
	int status;
	int i;

	status = read_mem(data, 0, sizeof(mem), mem);

	if (status)
		return status;

	for (i = 0; i < sizeof(mem) - 1; i += 2)
		crc = crc24(crc, mem[i], mem[i + 1]);

	*crc_pointer = crc24(crc, mem[i], 0) & 0x00FFFFFF;

	return 0;
}

int mxt_noisesuppression_t48_config(struct mxt_data *mxt)
{ 
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T48);

	noisesuppression_t48_config.ctrl  				= T48_CTRL ;
	noisesuppression_t48_config.cfg  				= T48_CFG ;
	noisesuppression_t48_config.calcfg  			= (T48_CALCFG | T48_CHGON_BIT) ;
	noisesuppression_t48_config.basefreq  			= T48_BASEFREQ ;
	noisesuppression_t48_config.freq_0  			= T48_RESERVED0 ;
	noisesuppression_t48_config.freq_1  			= T48_RESERVED1 ;
	noisesuppression_t48_config.freq_2  			= T48_RESERVED2 ;
	noisesuppression_t48_config.freq_3  			= T48_RESERVED3 ;
	noisesuppression_t48_config.mffreq_2  			= T48_MFFREQ_2 ;
	noisesuppression_t48_config.mffreq_3  			= T48_MFFREQ_3 ;
	noisesuppression_t48_config.nlgain  			= T48_RESERVED4 ;
	noisesuppression_t48_config.nlthr  				= T48_RESERVED5 ;
	noisesuppression_t48_config.gclimit  			= T48_RESERVED6 ;
	noisesuppression_t48_config.gcactvinvldadcs  	= T48_GCACTVINVLDADCS ;
	noisesuppression_t48_config.gcidleinvldadcs  	= T48_GCIDLEINVLDADCS ;
	noisesuppression_t48_config.gcinvalidthr  		= T48_RESERVED7 ;
	
	noisesuppression_t48_config.gcmaxadcsperx  		= T48_GCMAXADCSPERX ;
	noisesuppression_t48_config.gclimitmin  		= T48_GCLIMITMIN ;
	noisesuppression_t48_config.gclimitmax  		= T48_GCLIMITMAX ;
	noisesuppression_t48_config.gccountmintgt  		= T48_GCCOUNTMINTGT ;
	noisesuppression_t48_config.mfinvlddiffthr  	= T48_MFINVLDDIFFTHR ;
	noisesuppression_t48_config.mfincadcspxthr  	= T48_MFINCADCSPXTHR ;
	noisesuppression_t48_config.mferrorthr  		= T48_MFERRORTHR ;
	noisesuppression_t48_config.selfreqmax  		= T48_SELFREQMAX ;
	noisesuppression_t48_config.reserved9  			= T48_RESERVED9 ;
	noisesuppression_t48_config.reserved10  		= T48_RESERVED10 ;
	noisesuppression_t48_config.reserved11  		= T48_RESERVED11 ;
	noisesuppression_t48_config.reserved12  		= T48_RESERVED12 ;
	noisesuppression_t48_config.reserved13  		= T48_RESERVED13 ;
	noisesuppression_t48_config.reserved14 		 	= T48_RESERVED14 ;
	noisesuppression_t48_config.blen  				= T48_BLEN ;
	noisesuppression_t48_config.tchthr  			= T48_TCHTHR ;
	noisesuppression_t48_config.tchdi  				= T48_TCHDI ;
	noisesuppression_t48_config.movhysti  			= T48_MOVHYSTI ;
	noisesuppression_t48_config.movhystn  			= T48_MOVHYSTN ;
	noisesuppression_t48_config.movfilter  			= T48_MOVFILTER ;
	noisesuppression_t48_config.numtouch  			= T48_NUMTOUCH ;
	noisesuppression_t48_config.mrghyst  			= T48_MRGHYST ;
	noisesuppression_t48_config.mrgthr  			= T48_MRGTHR ;
	noisesuppression_t48_config.xloclip  			= T48_XLOCLIP ;
	noisesuppression_t48_config.xhiclip  			= T48_XHICLIP ;
	noisesuppression_t48_config.yloclip  			= T48_YLOCLIP ;
	noisesuppression_t48_config.yhiclip  			= T48_YHICLIP ;
	noisesuppression_t48_config.xedgectrl  			= T48_XEDGECTRL ;
	noisesuppression_t48_config.xedgedist  			= T48_XEDGEDIST ;
	noisesuppression_t48_config.yedgectrl  			= T48_YEDGECTRL ;
	noisesuppression_t48_config.yedgedist  			= T48_YEDGEDIST ;
	noisesuppression_t48_config.jumplimit  			= T48_JUMPLIMIT ;
	noisesuppression_t48_config.tchhyst  			= T48_TCHHYST ;
	noisesuppression_t48_config.nexttchdi  			= T48_NEXTTCHDI ;

	error = mxt_write_block(client, obj_addr, obj_size, (u8 *)&noisesuppression_t48_config);
	if (error < 0) { 
		dev_err(&client->dev, "[TSP] mxt_write_block failed! (%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}

int mxt_noisesuppression_t48_config_for_TA(struct mxt_data *mxt)
{ 
	struct i2c_client *client = mxt->client;
	u16 obj_addr, obj_size;
	int error;

	obj_addr = MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48);
	obj_size = MXT_GET_SIZE(MXT_PROCG_NOISESUPPRESSION_T48);

	noisesuppression_t48_config.ctrl  				= T48_CTRL_TA ;
	noisesuppression_t48_config.cfg  				= T48_CFG_TA;
	noisesuppression_t48_config.calcfg  			= T48_CALCFG_TA;
	noisesuppression_t48_config.basefreq  			= T48_BASEFREQ_TA;
	noisesuppression_t48_config.freq_0  			= T48_RESERVED0_TA;
	noisesuppression_t48_config.freq_1  			= T48_RESERVED1_TA;
	noisesuppression_t48_config.freq_2  			= T48_RESERVED2_TA;
	noisesuppression_t48_config.freq_3  			= T48_RESERVED3_TA;
	noisesuppression_t48_config.mffreq_2  			= T48_MFFREQ_2_TA;
	noisesuppression_t48_config.mffreq_3  			= T48_MFFREQ_3_TA;
	noisesuppression_t48_config.nlgain  			= T48_RESERVED4_TA;
	noisesuppression_t48_config.nlthr  				= T48_RESERVED5_TA;
	noisesuppression_t48_config.gclimit  			= T48_RESERVED6_TA;
	noisesuppression_t48_config.gcactvinvldadcs  	= T48_GCACTVINVLDADCS_TA;
	noisesuppression_t48_config.gcidleinvldadcs  	= T48_GCIDLEINVLDADCS_TA;
	noisesuppression_t48_config.gcinvalidthr  		= T48_RESERVED7_TA;
	
	noisesuppression_t48_config.gcmaxadcsperx  		= T48_GCMAXADCSPERX_TA;
	noisesuppression_t48_config.gclimitmin  		= T48_GCLIMITMIN_TA;
	noisesuppression_t48_config.gclimitmax  		= T48_GCLIMITMAX_TA;
	noisesuppression_t48_config.gccountmintgt  		= T48_GCCOUNTMINTGT_TA;
	noisesuppression_t48_config.mfinvlddiffthr  	= T48_MFINVLDDIFFTHR_TA;
	noisesuppression_t48_config.mfincadcspxthr  	= T48_MFINCADCSPXTHR_TA;
	noisesuppression_t48_config.mferrorthr  		= T48_MFERRORTHR_TA;
	noisesuppression_t48_config.selfreqmax  		= T48_SELFREQMAX_TA;
	noisesuppression_t48_config.reserved9  			= T48_RESERVED9_TA;
	noisesuppression_t48_config.reserved10  		= T48_RESERVED10_TA;
	noisesuppression_t48_config.reserved11  		= T48_RESERVED11_TA;
	noisesuppression_t48_config.reserved12  		= T48_RESERVED12_TA;
	noisesuppression_t48_config.reserved13  		= T48_RESERVED13_TA;
	noisesuppression_t48_config.reserved14 		 	= T48_RESERVED14_TA;
	noisesuppression_t48_config.blen  				= T48_BLEN_TA;
	noisesuppression_t48_config.tchthr  			= T48_TCHTHR_TA;
	noisesuppression_t48_config.tchdi  				= T48_TCHDI_TA;
	noisesuppression_t48_config.movhysti  			= T48_MOVHYSTI_TA;
	noisesuppression_t48_config.movhystn  			= T48_MOVHYSTN_TA;
	noisesuppression_t48_config.movfilter  			= T48_MOVFILTER_TA;
	noisesuppression_t48_config.numtouch  			= T48_NUMTOUCH_TA;
	noisesuppression_t48_config.mrghyst  			= T48_MRGHYST_TA;
	noisesuppression_t48_config.mrgthr  			= T48_MRGTHR_TA;
	noisesuppression_t48_config.xloclip  			= T48_XLOCLIP_TA;
	noisesuppression_t48_config.xhiclip  			= T48_XHICLIP_TA;
	noisesuppression_t48_config.yloclip  			= T48_YLOCLIP_TA;
	noisesuppression_t48_config.yhiclip  			= T48_YHICLIP_TA;
	noisesuppression_t48_config.xedgectrl  			= T48_XEDGECTRL_TA;
	noisesuppression_t48_config.xedgedist  			= T48_XEDGEDIST_TA;
	noisesuppression_t48_config.yedgectrl  			= T48_YEDGECTRL_TA;
	noisesuppression_t48_config.yedgedist  			= T48_YEDGEDIST_TA;
	noisesuppression_t48_config.jumplimit  			= T48_JUMPLIMIT_TA;
	noisesuppression_t48_config.tchhyst  			= T48_TCHHYST_TA;
	noisesuppression_t48_config.nexttchdi  			= T48_NEXTTCHDI_TA;

	error = mxt_write_block(client, obj_addr, obj_size, (u8 *)&noisesuppression_t48_config);
	if (error < 0) { 
		dev_err(&client->dev, "[TSP] mxt_write_block failed! (%s, %d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}



















































































































static int mxt224E_init_touch_driver(struct mxt_data *data)

{
	struct object_t *object_table;
	u32 read_crc = 0;
	u32 calc_crc;
	u16 crc_address;
	u16 dummy;
	int i;
	u8 id[ID_BLOCK_SIZE];
	int ret;
	u8 type_count = 0;
	u8 tmp;

	ret = read_mem(data, 0, sizeof(id), id);
	if (ret) {
		pr_err("[DVE068]  I2C read fail\n");
		return ret;
	}

	data->tsp_version = id[2];
	data->objects_len = id[6];

	data->device_info.family_id  = id[0];
	data->device_info.variant_id = id[1];
	data->device_info.major 	= ((id[2] >> 4) & 0x0F);
	data->device_info.minor 	 = (id[2] & 0x0F);
	data->device_info.build 	= id[3];
	data->device_info.x_size		= id[4];
	data->device_info.y_size		= id[5];
	data->device_info.num_objs	 = id[6];
	data->device_info.num_nodes  = data->device_info.x_size *
					  data->device_info.y_size;
	strcpy(data->device_info.family_name, "mXT224");

	pr_err("[DVE068] family = %d, variant = %d, version = %d, build = %d\n", id[0], id[1], id[2], id[3]);
	pr_err("[DVE068] matrix X size = %d\n", id[4]);
	pr_err("[DVE068] matrix Y size = %d\n", id[5]);

	object_table = kmalloc(data->objects_len * sizeof(*object_table),
				GFP_KERNEL);
	if (!object_table)
		return -ENOMEM;

	ret = read_mem(data, OBJECT_TABLE_START_ADDRESS,
			data->objects_len * sizeof(*object_table),
			(u8 *)object_table);
	if (ret)
		goto err;
	
	for (i = 0; i < data->objects_len; i++) {
		object_table[i].i2c_address =
				le16_to_cpu(object_table[i].i2c_address);
		tmp = 0;
		if (object_table[i].num_report_ids) {
			tmp = type_count + 1;
			type_count += object_table[i].num_report_ids *
						(object_table[i].instances + 1);
		}
		switch (object_table[i].object_type) {
		case TOUCH_MULTITOUCHSCREEN_T9:
			data->finger_type = tmp;
			pr_err("Finger type = %d\n",
						data->finger_type);
			break;
		case GEN_MESSAGEPROCESSOR_T5:
			data->msg_object_size = object_table[i].size + 1;
			pr_err("Message object size = "
						"%d\n", data->msg_object_size);
			break;
		}
	}

	data->objects = object_table;
	pr_err("[DVE068] Verify CRC\n");
	
	crc_address = OBJECT_TABLE_START_ADDRESS +
			data->objects_len * OBJECT_TABLE_ELEMENT_SIZE;




	ret = read_mem(data, crc_address, 3, (u8 *)&read_crc);
	if (ret)
		goto err;

	read_crc = le32_to_cpu(read_crc);

	ret = calculate_infoblock_crc(data, &calc_crc);
	if (ret)
		goto err;

	if (read_crc != calc_crc) {
		ret = -EFAULT;
		goto err;
	}

	ret = get_object_info(data, GEN_MESSAGEPROCESSOR_T5, &dummy,
					&data->msg_proc);
	if (ret)
		goto err;

	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &dummy,
					&data->cmd_proc);
	if (ret)
		goto err;

	return 0;

err:
	kfree(object_table);
	return ret;
}
















static int glove_menukey_mask=0;

static void report_input_data(struct mxt_data *data)
{
	int i;
	int finger_num = 0;
	
	for (i = 0; i < 5; i++) {
		if (!data->fingers[i].status)
			continue;
		
		
		
		if (data->fingers[i].status & (RELEASE_MSG_MASK | SUPPRESS_MSG_MASK) ) {
			data->fingers[i].status = 0;
			if(glove_menukey_mask){
				
				input_report_key(data->input_dev, KEY_BACK,0);
				input_sync(data->input_dev);
				glove_menukey_mask=0;
			}
		} else {

			if(touch_mode == TOUCH_GLOVE)
			{
					if(data->fingers[i].y > 920){
						
						input_report_key(data->input_dev, KEY_BACK,1);
						input_sync(data->input_dev);
						glove_menukey_mask=1;
						
					}





					else if((data->fingers[i].x < 35)
						||(data->fingers[i].x > 985)
						||(data->fingers[i].y < 42)) {
							continue;
						}
					else {
						input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
								data->fingers[i].z);  	
						input_report_abs(data->input_dev, ABS_MT_POSITION_X,
								data->fingers[i].x);
						input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
								data->fingers[i].y);
						input_report_abs(data->input_dev, ABS_MT_PRESSURE,
								data->fingers[i].w); 
						
						input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, i);
						
						input_mt_sync(data->input_dev); 	
						finger_num++;
					}

			}
			else {
				input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
						data->fingers[i].z);  	
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
						data->fingers[i].x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
						data->fingers[i].y);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE,
						data->fingers[i].w); 

			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, i);

				input_mt_sync(data->input_dev); 	
				finger_num++;
			}
		}
	}
	
	data->finger_mask = 0;
	
	input_report_key(data->input_dev, BTN_TOUCH, finger_num > 0);
	input_sync(data->input_dev);
}
















static void mxt_ta_worker(struct work_struct *work)
{ 
	struct mxt_data *mxt = container_of(work, struct mxt_data, ta_work);
	int error;

	if (debug >= DEBUG_INFO)
		pr_info("[TSP] %s\n", __func__);
	disable_irq(mxt->client->irq);

	if (mxt->set_mode_for_ta) { 
		
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48) + MXT_ADR_T48_CALCFG,
			T48_CALCFG_TA);

		
		mxt_noisesuppression_t48_config_for_TA(mxt);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48) + MXT_ADR_T48_CALCFG,
			T48_CALCFG_TA | T48_CHGON_BIT);
	} else { 
		
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48) + MXT_ADR_T48_CALCFG,
			T48_CALCFG);

		
		mxt_noisesuppression_t48_config(mxt);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCG_NOISESUPPRESSION_T48) + MXT_ADR_T48_CALCFG,
			T48_CALCFG | T48_CHGON_BIT);
	}
	if (error < 0) pr_err("[TSP] mxt TA/USB mxt_noise_suppression_config Error!!\n");
	else { 
		if (debug >= DEBUG_INFO) {
			if (mxt->set_mode_for_ta) { 
				pr_info("[TSP] mxt TA/USB mxt_noise_suppression_config Success!!\n");				
			} else {
				pr_info("[TSP] mxt BATTERY mxt_noise_suppression_config Success!!\n");
			}
		}
	}

	enable_irq(mxt->client->irq);
	return;
}


static void mxt_metal_suppression_off(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	mxt = container_of(work, struct mxt_data, timer_dwork.work);
	
	metal_suppression_chk_flag = false;
	printk("[TSP]%s, metal_suppression_chk_flag = %d \n", __func__, metal_suppression_chk_flag);
	disable_irq(mxt->client->irq);
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_AMPHYST, 30);
	enable_irq(mxt->client->irq);
	return;	
}

static int first_booting_status=1;

static void mxt_checkchip_cal_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	mxt = container_of(work, struct mxt_data, checkchip_dwork.work);
	mutex_lock(&mxt->touch_mutex); 
	cal_check_flag = 1;
	check_chip_calibration(mxt);

	if(first_booting_status){
		first_booting_status=0;
		enable_irq(mxt->client->irq);
	}
	mutex_unlock(&mxt->touch_mutex); 
	
	return;	
}


static void mxt_finger_recovery_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	mxt = container_of(work, struct mxt_data, finger_dwork.work);
	mxt_write_byte(mxt->client, get_object_address(mxt,MXT_GEN_ACQUIRECONFIG_T8,0,mxt->object_table,mxt->device_info.num_objs) + MXT_ADR_T8_ATCHCALST, 255);
    mxt_write_byte(mxt->client, get_object_address(mxt,MXT_GEN_ACQUIRECONFIG_T8,0,mxt->object_table,mxt->device_info.num_objs) + MXT_ADR_T8_ATCHCALSTHR, 1);
	
	return;	
}


static void ts_100ms_timeout_handler(unsigned long data)
{
	struct mxt_data *mxt = (struct mxt_data*)data;
	mxt->p_ts_timeout_tmr=NULL;	
	queue_work(ts_100s_tmr_workqueue, &mxt->tmr_work);
}

static void ts_100ms_timer_start(struct mxt_data *mxt)
{	
	if(mxt->p_ts_timeout_tmr != NULL)	del_timer(mxt->p_ts_timeout_tmr);
	mxt->p_ts_timeout_tmr = NULL;
		
	mxt->ts_timeout_tmr.expires = jiffies + HZ/10;	
	mxt->p_ts_timeout_tmr = &mxt->ts_timeout_tmr;
	add_timer(&mxt->ts_timeout_tmr);
}

static void ts_100ms_timer_stop(struct mxt_data *mxt)
{
	if(mxt->p_ts_timeout_tmr) del_timer(mxt->p_ts_timeout_tmr);
		mxt->p_ts_timeout_tmr = NULL;
}

static void ts_100ms_timer_init(struct mxt_data *mxt)
{
	init_timer(&(mxt->ts_timeout_tmr));
	mxt->ts_timeout_tmr.data = (unsigned long)(mxt);
   	mxt->ts_timeout_tmr.function = ts_100ms_timeout_handler;		
	mxt->p_ts_timeout_tmr=NULL;
}

static void mxt_check_wakeup_work(struct mxt_data *mxt);
static void ts_100ms_tmr_work(struct work_struct *work)
{
	struct mxt_data *mxt = container_of(work, struct mxt_data, tmr_work);

	timer_ticks++;

	if(TSP_LOG) printk("[TSP] 100ms T %d\n", timer_ticks);

	disable_irq(mxt->client->irq);
	
	if(facesup_message_flag ){
	 	printk("[TSP] facesup_message_flag = %d\n", facesup_message_flag);
		mxt_check_wakeup_work(mxt);
	 	

		if(facesup_message_flag != 4 )
		{
			mxt_release_all_keys(mxt);
			mxt_release_fingers(mxt);
			calibrate_chip(mxt);
		}
		else
		{			
			mxt_release_all_keys(mxt);
			mxt_release_fingers(mxt);
			cal_check_flag = 0;
		}

		facesup_message_flag = 0;
	}

	timer_ticks = 10;

	if ((timer_flag == ENABLE) && (timer_ticks<10)) 
	
	{
		ts_100ms_timer_start(mxt);
		palm_check_timer_flag = false;
	} 
	else 
	{
		if (palm_check_timer_flag && ((facesup_message_flag == 1) || (facesup_message_flag == 2)) && (palm_release_flag == false)) 
		{
			printk("[TSP] calibrate_chip\n");
			calibrate_chip(mxt);	
			palm_check_timer_flag = false;
		}
		timer_flag = DISABLE;
		timer_ticks = 0;
	}

	facesup_message_flag = 0;
	
	enable_irq(mxt->client->irq);
}
static void calibrate_timer_stop(struct mxt_data *mxt)
{
	printk("[TSP] %s call\n", __func__ );
	if(calibrate_timer_flag){
		printk("[TSP] %s\n", __func__);
		cancel_delayed_work(&mxt->calibrate_timer_dwork);
	}
	calibrate_timer_flag = false;

}

static void calibrate_timer_start(struct mxt_data *mxt)
{
	printk("[TSP] %s call\n", __func__ );
	if(!calibrate_timer_flag){
		printk("[TSP] %s\n", __func__);
		schedule_delayed_work(&mxt->calibrate_timer_dwork, msecs_to_jiffies(500));
	}
	calibrate_timer_flag = true;
}

static void calibrate_timer_delayed_work(struct work_struct *work)
{
	struct	mxt_data *mxt;
	mxt = container_of(work, struct mxt_data, calibrate_timer_dwork.work);
	printk("[TSP] %s\n", __func__);
	cal_maybe_good(mxt);
	cal_check_flag=0;
	calibrate_timer_flag = false;
}

static void suppress_timer_delayed_work(struct work_struct *work)
{
	struct	mxt_data *mxt;
	printk("%s\n",__func__);
	mxt = container_of(work, struct mxt_data, suppress_timer_dwork.work);

	calibrate_chip(mxt);
}

static void cal_maybe_good(struct mxt_data *mxt)
{
	int ret;
	printk("[TSP] %s\n", __func__);
	
	
	if (mxt_time_point != 0) {
		
		if ((jiffies_to_msecs(jiffies) - mxt_time_point) >= 300) {
			pr_info("[TSP] time from calibration start = %d\n", (jiffies_to_msecs(jiffies) - mxt_time_point));
			
			mxt_time_point = 0;
			cal_check_flag = 0;
			
			timer_flag = DISABLE;
			timer_ticks = 0;
			ts_100ms_timer_stop(mxt);


















			
			if(0) ret = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_TCHTHR, T9_TCHTHR);

			mutex_lock(&mxt->touch_mutex);
			write_temp_config(mxt,8,8,ATCHFRCCALTHR_NOMAL);
			write_temp_config(mxt,8,9,ATCHFRCCALRATIO_NORMAL);
			mutex_unlock(&mxt->touch_mutex);
			printk("[TSP] Calibration success!! \n");

			if (metal_suppression_chk_flag == true) {
				
				cancel_delayed_work(&mxt->timer_dwork);
				schedule_delayed_work(&mxt->timer_dwork, 2000);
			}
		}
		else { 
			cal_check_flag = 1u;
		}
	}
	else { 
		cal_check_flag = 1u;
	}
}


static int calibrate_chip(struct mxt_data *mxt)
{
	u8 data = 1u;
	int ret ;
	printk("[TSP] %s\n", __func__);
	if (debug >= DEBUG_INFO)
		pr_info("[TSP] %s\n", __func__);

	mxt_time_point = jiffies_to_msecs(jiffies);

	facesup_message_flag  = 0;
	not_yet_count = 0;


















	
	if(0) ret = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_TOUCH_MULTITOUCHSCREEN_T9) + MXT_ADR_T9_TCHTHR, T9_TCHTHR);


	
	
	
	
	
	ret = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_CALIBRATE, data);
	if ( ret < 0 ) {
		printk("[TSP][ERROR] line : %d\n", __LINE__);
		return -1;
	} else {
		
		
		cal_check_flag = 1u;
	}

	msleep(10);
 
	return ret;
}

static void mxt_palm_recovery(struct work_struct *work)
{
	struct	mxt_data *mxt;
	int error;
	mxt = container_of(work, struct mxt_data, config_dwork.work);
	
	printk("[TSP] %s \n", __func__);

	if( mxt->check_auto_cal == 1)
	{
		mxt->check_auto_cal = 2;
		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHCALST,
			T8_ATCHCALST);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHCALSTHR,
			T8_ATCHCALSTHR);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHFRCCALTHR,
			T8_ATCHFRCCALTHR);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
		
		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHFRCCALRATIO,
			T8_ATCHFRCCALRATIO);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42) + MXT_ADR_T42_MAXNUMTCHS,
			T42_MAXNUMTCHS);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
		
	} else if(mxt->check_auto_cal == 2) { 
		mxt->check_auto_cal = 1;
		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHCALST,
			T8_ATCHCALST);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHCALSTHR,
			T8_ATCHCALSTHR);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHFRCCALTHR,
			0);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
		
		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_ATCHFRCCALRATIO,
			0);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

		
		error = mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_PROCI_TOUCHSUPPRESSION_T42) + MXT_ADR_T42_MAXNUMTCHS,
			0);
		if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);
			
	
	} else if(mxt->check_auto_cal == 3) {
		mxt->check_auto_cal = 5;

	
	} else if(mxt->check_auto_cal == 4) {
	        mxt->check_auto_cal = 0;
		facesup_message_flag  = 0;
	}
}

static void mxt_check_wakeup_work(struct mxt_data *mxt)
{ 
	int ret, loop_cnt=0, diag_address;
	uint8_t data_buffer[90] = { 0, 0 };
	uint8_t try_ctr = 0;
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t i;
	uint8_t j;
	uint8_t x_line_limit;
	uint8_t check_mask;
	int finger_cnt=0;

	if(TSP_LOG) printk("%s <<<< start mxt_check_wakeup_work >>>>\n",__FUNCTION__);
		
	memset( data_buffer , 0xFF, sizeof( data_buffer ) );
	diag_address = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);

	ret = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_DIAGNOSTICS, 0xf3);

	for(loop_cnt=0; loop_cnt<3; loop_cnt++)
	{
		
		while(!((data_buffer[0] == 0xf3) && (data_buffer[1] == loop_cnt))) {
			if(try_ctr > 20) {
				printk("[TSP] Diagnostic Data did not update !!--------\n");
				break;
			}
			msleep(2); 
			try_ctr++; 
			
			mxt_read_block(mxt->client, diag_address, 2, data_buffer);
		}
		
		
		try_ctr = 0;

		mxt_read_block(mxt->client, diag_address, 90, data_buffer);

		
		
		x_line_limit = 16 + T46_MODE;
		if(x_line_limit > 22) { 
			
			x_line_limit = 22;
		}
	
		
		x_line_limit = x_line_limit << 1;
	
		
		for(i = 0; i < x_line_limit; i+=2) {  
		
			
			
	
			
			for(j = 0; j < 8; j++) { 
				
				check_mask = 1 << j;
	
				
				if(data_buffer[2+i] & check_mask) { 
					tch_ch++;
				}
				if(data_buffer[3+i] & check_mask) { 
					tch_ch++;
				}
	
				
				if(data_buffer[46+i] & check_mask) { 
					atch_ch++;
				}
				if(data_buffer[47+i] & check_mask) { 
					atch_ch++;
				}
			}
		}

		
		ret = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_DIAGNOSTICS, 0x01);

		data_buffer[0] = 0;
		
	}

	for (i = 0 ; i < MXT_MAX_NUM_TOUCHES ; ++i) { 
		if ( mxt->fingers[i].w == -1 )
			continue;
		finger_cnt++;
	}

	printk("<<<< node_cnt tch_ch[%d] atch_ch[%d] finger_cnt[%d]>>>> \n",tch_ch,atch_ch,finger_cnt);

	if ((tch_ch > 0 ) && ( atch_ch	> 0)) {
		facesup_message_flag = 1;			
	} else if ((tch_ch > 0 ) && ( atch_ch  == 0)) {
		facesup_message_flag = 2;
	} else if ((tch_ch == 0 ) && ( atch_ch > 0)) {
		facesup_message_flag = 3;
	}else if((tch_ch == 0 ) && ( atch_ch == 0) && (finger_cnt==0)){ 
		facesup_message_flag = 4;
	}else{
		facesup_message_flag = 5;
	}

}

static void check_chip_palm(struct mxt_data *mxt)
{
	uint8_t data_buffer[100] = { 0 };
	uint8_t try_ctr = 0;
	uint8_t data_byte = 0xF3; 
	uint16_t diag_address;
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t check_mask;
	uint8_t i;
	uint8_t j;
	uint8_t x_line_limit;
	uint8_t max_touch_ch;
	struct i2c_client *client;

	client = mxt->client;
	
	printk("[TSP] %s\n", __func__);

	



	
	
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_DIAGNOSTICS, 0xf3);
	
	
	diag_address = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	
	msleep(5);

	
	memset( data_buffer , 0xFF, sizeof( data_buffer ) );

	
	while(!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))) {
		
		if(try_ctr > 50) {
			
			printk("[TSP] Diagnostic Data did not update!!\n");
			break;
		}
		msleep(2); 
		try_ctr++; 
		mxt_read_block(client, diag_address, 2, data_buffer);
		
		
	}

	if(try_ctr > 50){
		printk("[TSP] %s, Diagnostic Data did not update over 50, force reset!! %d\n", __func__, try_ctr);
		
		
		
		mxt_sw_reset(mxt);
		msleep(150); 
		return;
	}

	
	mxt_read_block(client, diag_address, 90, data_buffer);

	

	
	if((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)) {

		
		
		x_line_limit = 16 + T46_MODE;
		if(x_line_limit > 22) { 
			
			x_line_limit = 22;
		}

		
		x_line_limit = x_line_limit << 1;

		
		for(i = 0; i < x_line_limit; i+=2) {  
		
			
			

			
			for(j = 0; j < 8; j++) { 
				
				check_mask = 1 << j;

				
				if(data_buffer[2+i] & check_mask) { 
					tch_ch++;
				}
				if(data_buffer[3+i] & check_mask) { 
					tch_ch++;
				}

				
				if(data_buffer[46+i] & check_mask) { 
					atch_ch++;
				}
				if(data_buffer[47+i] & check_mask) { 
					atch_ch++;
				}
			}
		}

		
		printk("[TSP] Flags Counted channels: t:%d a:%d \n", tch_ch, atch_ch);

		

		data_byte = 0x01;
		
		mxt_write_byte(mxt->client,MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_DIAGNOSTICS, data_byte);

		     
		mxt_read_byte(mxt->client,MXT_BASE_ADDR(MXT_USER_INFO_T38)+ MXT_ADR_T38_USER1, &max_touch_ch);

		if ((tch_ch > 0 ) && ( atch_ch  > 0)) {
			facesup_message_flag = 1;			
		} else if ((tch_ch > 0 ) && ( atch_ch  == 0)) {
			facesup_message_flag = 2;
		} else if ((tch_ch == 0 ) && ( atch_ch > 0)) {
			facesup_message_flag = 3;
		}else {
			facesup_message_flag = 4;
		}

		if ((tch_ch < 70) || ((tch_ch >= 70) && ((tch_ch - atch_ch) > 25))) palm_check_timer_flag = true;

		printk("[TSP] Touch suppression State: %d \n", facesup_message_flag);
	}
}






































static void check_chip_calibration(struct mxt_data *mxt)
{
	uint8_t data_buffer[100] = { 0 };
	uint8_t try_ctr = 0;
	uint8_t data_byte = 0xF3; 
	uint16_t diag_address;
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t check_mask;
	uint8_t i;
	uint8_t j;
	uint8_t x_line_limit;
	
	
	struct i2c_client *client;
	uint8_t finger_cnt = 0;

	client = mxt->client;
	
	

	if (cal_check_flag != 1) {
		printk("[TSP] check_chip_calibration just return!! finger_cnt = %d\n", finger_cnt);
		return;
	}

	
	
	
	
	



	
	
	mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_DIAGNOSTICS, 0xf3);
	
	
	diag_address = MXT_BASE_ADDR(MXT_DEBUG_DIAGNOSTICS_T37);
	
	
	msleep(5);

	
	memset( data_buffer , 0xFF, sizeof( data_buffer ) );

	
	while(!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))) {
		
		if(try_ctr > 50) {
			
			
				break;
		}
		msleep(2); 
		try_ctr++; 
		
		mxt_read_block(client, diag_address, 2, data_buffer);
		
		
	}

	if(try_ctr > 50){
		
		
		mxt_release_fingers(mxt);
		mxt_release_all_keys(mxt);
		mxt_sw_reset(mxt);
		msleep(150); 
		
		return;
	}

	
	
	mxt_read_block(client, diag_address, 90, data_buffer);

	

	
	if((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)) {

		
		x_line_limit = 16 + T46_MODE;
		if(x_line_limit > 22) { 
			
			x_line_limit = 22;
		}

		
		x_line_limit = x_line_limit << 1;

		
		for(i = 0; i < x_line_limit; i+=2) {  
			
			for(j = 0; j < 8; j++) { 
				
				check_mask = 1 << j;

				
				if(data_buffer[2+i] & check_mask) { 
					tch_ch++;
				}
				if(data_buffer[3+i] & check_mask) { 
					tch_ch++;
				}

				
				if(data_buffer[46+i] & check_mask) { 
					atch_ch++;
				}
				if(data_buffer[47+i] & check_mask) { 
					atch_ch++;
				}
			}
		}

		
		

		

		data_byte = 0x01;
		
		mxt_write_byte(mxt->client,
			MXT_BASE_ADDR(MXT_GEN_COMMANDPROCESSOR_T6) + MXT_ADR_T6_DIAGNOSTICS, data_byte);
		
		for (i = 0 ; i < MXT_MAX_NUM_TOUCHES ; ++i) { 
			if ( mxt->fingers[i].w == -1 )
				continue;
			finger_cnt++;
		}
		printk("[TSP] check_chip_calibration - tch_ch = %d atch_ch = %d finger_cnt = %d\n", tch_ch, atch_ch, finger_cnt);

		if (cal_check_flag != 1) {
			printk("[TSP] check_chip_calibration just return!! finger_cnt = %d\n", finger_cnt);
			
			return;
		}
		
		 
		
		
		if((tch_ch && (tch_ch < 10 ) && (atch_ch == 0)) && (finger_cnt < 3)) {
			
			if ((finger_cnt >= 2) && (tch_ch <= 3)) {
				
				calibrate_chip(mxt);
				
				timer_flag = DISABLE;
				timer_ticks = 0;
				ts_100ms_timer_stop(mxt);
				cal_check_flag=1;
			} else {

				not_yet_count = 0;

				calibrate_timer_wait = true;
				printk("[TSP] calibrate_timer_wait\n");
			}
			
			calibration_complete_count=1;
			
			
			

		} else if (atch_ch > ((finger_cnt * 13) + 2)) { 
			
			calibrate_chip(mxt);
			
			timer_flag = DISABLE;
			timer_ticks = 0;
			ts_100ms_timer_stop(mxt);
		} else if((tch_ch + 7  ) <= atch_ch) { 
			
			
			calibrate_chip(mxt);
			
			timer_flag = DISABLE;
			timer_ticks = 0;
			ts_100ms_timer_stop(mxt);

		}else { 
			cal_check_flag = 1u;
			if (timer_flag == DISABLE) {
				
				
				timer_flag = ENABLE;
				timer_ticks = 0;
				ts_100ms_timer_start(mxt);
			}
			not_yet_count++;
			
			if((tch_ch == 0) && (atch_ch == 0)) {
				
				not_yet_count = 0;
			} else if(not_yet_count >= 30) {
				
				not_yet_count =0;
				mxt_release_all_keys(mxt);
				mxt_release_fingers(mxt);
				
				
				
				timer_flag = DISABLE;
				timer_ticks = 0;
				ts_100ms_timer_stop(mxt);
			}
		}
	}
}












static void mxt_release_fingers(struct mxt_data *mxt)
{
	int id;
	
	for (id = 0 ; id < MXT_MAX_NUM_TOUCHES ; ++id) { 
		if ( mxt->fingers[id].w == -1 )
			continue;

		
		mxt->fingers[id].w = 0;
		mxt->fingers[id].status = 0;
		
		REPORT_MT(id, mxt->fingers[id].x, mxt->fingers[id].y, mxt->fingers[id].w, mxt->fingers[id].z);

		if (mxt->fingers[id].w == 0)
			mxt->fingers[id].w = -1;
	}

	input_sync(mxt->input_dev);
}

static void mxt_release_all_keys(struct mxt_data *data)
{
	input_report_key(data->input_dev, KEY_MENU, 0);
	input_report_key(data->input_dev, KEY_HOME, 0);
	input_report_key(data->input_dev, KEY_BACK, 0);
	
	input_report_key(data->input_dev, KEY_APP_SWITCH, 0);
	
}

static void equalize_coordinate2(bool detect, u8 id, u16 *px, u16 *py)
{ 
	static u32 pre_x[MXT_MAX_NUM_TOUCHES] = {  0, };
	static u32 pre_y[MXT_MAX_NUM_TOUCHES] = {  0, };
	static u32 pre_raw_x[MXT_MAX_NUM_TOUCHES] = {  0, };
	static u32 pre_raw_y[MXT_MAX_NUM_TOUCHES] = {  0, };

	static int tcount[MXT_MAX_NUM_TOUCHES] = {  0, };
	static int pre_raw_distance_x[MXT_MAX_NUM_TOUCHES] = {  0, };
	static int pre_raw_distance_y[MXT_MAX_NUM_TOUCHES] = {  0, };
	static int pre_raw_distance[MXT_MAX_NUM_TOUCHES] = {  0, };
	static int drag_fast_stop[MXT_MAX_NUM_TOUCHES] = {  0, };
	
	int distance = 0;

	int raw_distance = 0;
	int raw_distance_x = 0;
	int raw_distance_y = 0;
	int coff0 = 0;
	int coff1 = 0;	
	int coff2 = 0;	
	u16 x_data = 0;
	u16 y_data = 0;
	int raw_slop_error =0;
	int dx =0;
	int dy =0;
	int pre_dx =0;
	int pre_dy =0;

	if (detect) { 
		tcount[id] = 0;
		raw_distance_x =0;
		raw_distance_y =0;	
		raw_distance = 0;
		drag_fast_stop[id]=0;
	}else { 
		raw_distance_x = *px - pre_raw_x[id];
		raw_distance_y = *py - pre_raw_y[id];
		raw_distance = abs(raw_distance_x) + abs(raw_distance_y);
	}

	raw_slop_error=0;

	if ((tcount[id] > 1) && (raw_distance >50)&& (pre_raw_distance[id]>0)) {

		dx = (raw_distance_x*100)/raw_distance;
		pre_dx = (pre_raw_distance_x[id]*100)/pre_raw_distance[id];
		dy = (raw_distance_y*100)/raw_distance;
		pre_dy = (pre_raw_distance_y[id]*100)/pre_raw_distance[id];

		raw_slop_error = abs(dx -pre_dx)+abs(dy - pre_dy);
	}	
	

	x_data = *px;
	y_data = *py;	      

	if (tcount[id] > 3){ 

		coff0 = min(50, raw_distance/2);
		coff0 = max(0, coff0);
		coff1 = max(13, 40-raw_distance/2);
		coff2 = coff0+coff1;
		*px = (u16)((x_data*coff0 + pre_x[id]*coff1)/coff2);
		*py = (u16)((y_data*coff0 + pre_y[id]*coff1)/coff2);

	}else if(tcount[id] > 0){
	
		if (raw_distance < 15) {
			*px = (u16)((x_data + pre_x[id]*60)/61);
			*py = (u16)((y_data + pre_y[id]*60)/61);
		}else{	
			*px = (u16)((x_data + pre_raw_x[id])/2);
			*py = (u16)((y_data + pre_raw_y[id])/2);
		}
	}


	if(( raw_distance>30 )&& (raw_slop_error < 81)){
		drag_fast_stop[id]=1;
	}
	
	if(drag_fast_stop[id]==1){
		distance = abs(x_data-*px) + abs(y_data-*py);
		if (distance <9 ){
			drag_fast_stop[id]=0;
		} 
	} 

	if (raw_slop_error >80){
		*px = (u16)((x_data + pre_x[id]*60)/61);
		*py = (u16)((y_data + pre_y[id]*60)/61);
	}else if ( (distance >8) && (raw_distance<9) && (drag_fast_stop[id]==1) ){
		coff0 = 25 - raw_distance * 2;
		coff1 = 75- coff0;
		*px = (u16)((x_data*coff0 + pre_x[id]*coff1)/75);
		*py = (u16)((y_data*coff0 + pre_y[id]*coff1)/75);
	}

	tcount[id]++;
	pre_raw_x[id] = x_data;
	pre_raw_y[id] = y_data;	  	
	pre_x[id] = *px;
	pre_y[id] = *py;	
	pre_raw_distance_x[id] = raw_distance_x;
	pre_raw_distance_y[id] = raw_distance_y;	
	pre_raw_distance[id] = raw_distance;	

}

static void equalize_coordinate(bool detect, u8 id, u16 *px, u16 *py)
{ 
	static int tcount[MXT_MAX_NUM_TOUCHES] = {  0, };
	static u16 pre_x[MXT_MAX_NUM_TOUCHES][4] = { { 0}, };
	static u16 pre_y[MXT_MAX_NUM_TOUCHES][4] = { { 0}, };
	int coff[4] = { 0,};
	int distance = 0;

	if (detect) { 
		tcount[id] = 0;
	}

	pre_x[id][tcount[id]%4] = *px;
	pre_y[id][tcount[id]%4] = *py;

	if (tcount[id] > 3)	{ 
		distance = abs(pre_x[id][(tcount[id]-1)%4] - *px) + abs(pre_y[id][(tcount[id]-1)%4] - *py);

		coff[0] = (u8)(4 + distance/5);
		if (coff[0] < 8) { 
			coff[0] = max(4, coff[0]);
			coff[1] = min((10 - coff[0]), (coff[0]>>1)+1);
			coff[2] = min((10 - coff[0] - coff[1]), (coff[1]>>1)+1);
			coff[3] = 10 - coff[0] - coff[1] - coff[2];

			pr_debug("[TSP] %d, %d, %d, %d \n",
				coff[0], coff[1], coff[2], coff[3]);
			*px = (u16)((*px*(coff[0])
				+ pre_x[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_x[id][(tcount[id]-2)%4]*(coff[2])
				+ pre_x[id][(tcount[id]-3)%4]*(coff[3]))/10);
			*py = (u16)((*py*(coff[0])
				+ pre_y[id][(tcount[id]-1)%4]*(coff[1])
				+ pre_y[id][(tcount[id]-2)%4]*(coff[2])
				+ pre_y[id][(tcount[id]-3)%4]*(coff[3]))/10);
		} else { 
			*px = (u16)((*px*4 + pre_x[id][(tcount[id]-1)%4])/5);
			*py = (u16)((*py*4 + pre_y[id][(tcount[id]-1)%4])/5);
		}
	}
	tcount[id]++;
}












void process_T9_message(u8 *message, struct mxt_data *mxt)
{ 
	struct	input_dev *input;
	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8 report_id;
	u8 touch_id;  
	u8 pressed_or_released = 0;
	static int prev_touch_id = -1;
	int i, error;
	u16 chkpress = 0;
	u8 touch_message_flag = 0;

	input = mxt->input_dev;
	status = message[MXT_MSG_T9_STATUS];
	report_id = message[0];
	touch_id = report_id - 2;

	if (touch_id >= MXT_MAX_NUM_TOUCHES) { 
		pr_err("[TSP] Invalid touch_id (toud_id=%d)", touch_id);
		return;
	}

	
	xpos = message[2];
	xpos = xpos << 4;
	xpos = xpos | (message[4] >> 4);
	xpos >>= 2;

	ypos = message[3];
	ypos = ypos << 4;
	ypos = ypos | (message[4] & 0x0F);
	ypos >>= 2;

	


	if ((coin_check_count <= 2) && (cal_check_flag == 0) && metal_suppression_chk_flag) {
		new_touch.id		= report_id;
		new_touch.status[new_touch.id]	= status;
		new_touch.xpos[new_touch.id]	= xpos;
		new_touch.ypos[new_touch.id]	= ypos;
		new_touch.area[new_touch.id]	= message[MXT_MSG_T9_TCHAREA];
		new_touch.amp[new_touch.id] 	= message[MXT_MSG_T9_TCHAMPLITUDE];










		tch_vct[new_touch.id].length[tch_vct[new_touch.id].cnt] = ABS(old_touch.xpos[new_touch.id],  new_touch.xpos[new_touch.id]);
		tch_vct[new_touch.id].length[tch_vct[new_touch.id].cnt] += ABS(old_touch.ypos[new_touch.id],new_touch.ypos[new_touch.id]);

		if( new_touch.area[new_touch.id] != 0) {
			tch_vct[new_touch.id].angle[tch_vct[new_touch.id].cnt] = new_touch.amp[new_touch.id] / new_touch.area[new_touch.id];
		} else {
			tch_vct[new_touch.id].angle[tch_vct[new_touch.id].cnt] = 255;
		}
		
		

















		 if (((new_touch.area[new_touch.id] > 4) && (new_touch.area[new_touch.id] < 15))
		 	&& ((new_touch.amp[new_touch.id] > 10) && (new_touch.amp[new_touch.id] < 45))
		 	&& ( tch_vct[new_touch.id].length[tch_vct[new_touch.id].cnt] < 20 )
		 	&& ( tch_vct[new_touch.id].angle[tch_vct[new_touch.id].cnt] < 12 ))
		 {
		 	tch_vct[new_touch.id].cnt ++;
		 } else {
			tch_vct[new_touch.id].cnt = 0;
		 }

		if (debug >= DEBUG_MESSAGES) {
			pr_info("[TSP] TCH_MSG :  %4d, 0x%2x, %4d, %4d , area=%d,  amp=%d, \n",
				report_id,
				status,
				xpos,
				ypos,
				message[MXT_MSG_T9_TCHAREA], 
				message[MXT_MSG_T9_TCHAMPLITUDE]);
			
			pr_info("[TSP] TCH_VCT :  %4d, length=%d, angle=%d, cnt=%d  \n",
				new_touch.id, 
				tch_vct[new_touch.id].length[tch_vct[new_touch.id].cnt],
				tch_vct[new_touch.id].angle[tch_vct[new_touch.id].cnt],
				tch_vct[new_touch.id].cnt);
		}

		if((tch_vct[new_touch.id].cnt >= 4) && (time_after_autocal_enable == 0)){






			for(i=0;i < 10; i++) {
				tch_vct[i].cnt = 0;
			}

			
			if (debug >= DEBUG_MESSAGES)
				pr_info("[TSP] Floating metal Suppressed!! Autocal = 3\n");
			error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_TCHAUTOCAL, 3);
			if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

			time_after_autocal_enable = jiffies_to_msecs(jiffies);
		}

		if (time_after_autocal_enable != 0) {
			if ((jiffies_to_msecs(jiffies) - time_after_autocal_enable) > 1500) {
				coin_check_count++;
				if (debug >= DEBUG_MESSAGES)
					pr_info("[TSP] Floating metal Suppressed time out!! Autocal = 0, (%d), coin_check_count = %d \n", 
						(jiffies_to_msecs(jiffies) - time_after_autocal_enable), 
						coin_check_count);
				
				
				error = mxt_write_byte(mxt->client, MXT_BASE_ADDR(MXT_GEN_ACQUIRECONFIG_T8) + MXT_ADR_T8_TCHAUTOCAL, 0);
				if (error < 0) pr_err("[TSP] %s, %d Error!!\n", __func__, __LINE__);

				time_after_autocal_enable = 0;

				for(i=0;i < 10; i++) {
					tch_vct[i].cnt = 0;
				}
			}
		}




		old_touch.id	=  new_touch.id ;
		old_touch.status[new_touch.id]	=  new_touch.status[new_touch.id] ;
		old_touch.xpos[new_touch.id]	=  new_touch.xpos[new_touch.id] ;
		old_touch.ypos[new_touch.id]	=  new_touch.ypos[new_touch.id] ;
		old_touch.area[new_touch.id]	=  new_touch.area[new_touch.id] ;
		old_touch.amp[new_touch.id]	=  new_touch.amp[new_touch.id] ;	  
	}
	  




	if (status & MXT_MSGB_T9_DETECT) {   
		
		touch_message_flag = 1;
						
		mxt->fingers[touch_id].w = message[MXT_MSG_T9_TCHAMPLITUDE];  
		mxt->fingers[touch_id].x = (int16_t)xpos;
		mxt->fingers[touch_id].y = (int16_t)ypos;

		if (status & MXT_MSGB_T9_PRESS) { 
			pressed_or_released = 1;  
			equalize_coordinate(1, touch_id, &mxt->fingers[touch_id].x, &mxt->fingers[touch_id].y);
		} else if (status & MXT_MSGB_T9_MOVE) { 
			equalize_coordinate(0, touch_id, &mxt->fingers[touch_id].x, &mxt->fingers[touch_id].y);
		}
	} else if (status & MXT_MSGB_T9_RELEASE) {   
		pressed_or_released = 1;
		mxt->fingers[touch_id].w = 0;
	} else if (status & MXT_MSGB_T9_SUPPRESS) {   
	     





		if (debug >= DEBUG_MESSAGES)
			pr_info("[TSP] Palm(T9) Suppressed !!! \n");
		facesup_message_flag_T9 = 1;
		pressed_or_released = 1;
		mxt->fingers[touch_id].w = 0;
	} else { 
		pr_err("[TSP] Unknown status (0x%x)", status);
		
		if(facesup_message_flag_T9 == 1) { 
			facesup_message_flag_T9 = 0;
			if (debug >= DEBUG_MESSAGES)
				pr_info("[TSP] Palm(T92) Suppressed !!! \n");
		}
		
	}

	
	mxt->fingers[touch_id].z = message[MXT_MSG_T9_TCHAREA];

	if (prev_touch_id >= touch_id || pressed_or_released) { 
		for (i = 0; i < MXT_MAX_NUM_TOUCHES; ++i) { 
			if (mxt->fingers[i].w == -1)
				continue;
			
			
			




			if (mxt->fingers[i].w == 0) { 	
				mxt->fingers[i].w = -1;
			} else { 
				chkpress ++;
			}

		}
		input_sync(input);  
	}
	prev_touch_id = touch_id;
	
















































	if(cal_check_flag == 1) 
	{

		

		if(touch_message_flag) {
			if(timer_flag == DISABLE) {
				timer_flag = ENABLE;
				timer_ticks = 0u;
				ts_100ms_timer_start(mxt);
			}
			if (mxt_time_point == 0) 
				mxt_time_point = jiffies_to_msecs(jiffies);
			check_chip_calibration(mxt);
		}
	}









	return;
}

void process_T42_message(u8 *message, struct mxt_data *mxt)
{ 
	struct	input_dev *input;
	u8  status = false;

	input = mxt->input_dev;
	status = message[MXT_MSG_T42_STATUS];

	
	

	if (message[MXT_MSG_T42_STATUS] == MXT_MSGB_T42_TCHSUP) { 
		palm_release_flag = false;
		
		printk("[TSP] Palm(T42) Suppressed !!! \n");
		
		if (facesup_message_flag && timer_flag) 
			return;
		
		check_chip_palm(mxt);

		if(facesup_message_flag)
		{
				
			timer_flag = ENABLE;
			timer_ticks = 0;
			ts_100ms_timer_start(mxt);
			printk("[TSP] Palm(T42) Timer start !!! \n");
		}
	} else {
		printk("[TSP] Palm(T42) Released !!! \n");
		
		palm_release_flag = true;
		facesup_message_flag = 0;
		
		timer_flag = DISABLE;
		timer_ticks = 0;
		ts_100ms_timer_stop(mxt);		
	}
	return;
}

int process_message(u8 *message, u8 object, struct mxt_data *mxt)
{ 
	struct i2c_client *client;

	u8  status, state;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  event;
	u8  length;
	u8  report_id;

	client = mxt->client;
	length = mxt->message_size;
	report_id = message[0];

	printk("process_message 0: (0x%x) 1:(0x%x) object:(%d)", message[0], message[1], object);

	switch (object) { 
	case MXT_PROCG_NOISESUPPRESSION_T48:
		state = message[4];
		if (state == 0x05) {	
			printk("[TSP] NOISESUPPRESSION_T48 error\n");
		}
		break;
	case MXT_GEN_COMMANDPROCESSOR_T6:
		status = message[1];
		if (status & MXT_MSGB_T6_COMSERR) { 
			printk("[TSP] maXTouch checksum error\n");
		}
		if (status & MXT_MSGB_T6_CFGERR) { 
			printk("[TSP] maXTouch configuration error\n");
			mxt_sw_reset(mxt);
			msleep(250);
			
			
			
			
			
			
			
		}
		if (status & MXT_MSGB_T6_CAL) { 
			dev_info(&client->dev, "[TSP] maXTouch calibration in progress(cal_check_flag = %d)\n",cal_check_flag);

			if(cal_check_flag == 0){
				
				mxt->check_auto_cal = 4;
				
				cancel_delayed_work(&mxt->config_dwork);
				schedule_delayed_work(&mxt->config_dwork, 0);
				cal_check_flag = 1;
			}
 		}
		if (status & MXT_MSGB_T6_SIGERR) { 
			dev_err(&client->dev,
				"[TSP] maXTouch acquisition error\n");





		}
		if (status & MXT_MSGB_T6_OFL) { 
			dev_err(&client->dev, "[TSP] maXTouch cycle overflow\n");
		}
		if (status & MXT_MSGB_T6_RESET) { 
			if (debug >= DEBUG_MESSAGES) 
				dev_info(&client->dev, "[TSP] maXTouch chip reset\n");
		}
		if (status == MXT_MSG_T6_STATUS_NORMAL) { 
			if (debug >= DEBUG_MESSAGES) 
				dev_info(&client->dev, "[TSP] maXTouch status normal\n");
		}
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T9:
		process_T9_message(message, mxt);
		break;

	case MXT_TOUCH_KEYARRAY_T15:
		
		break;

	case MXT_PROCI_TOUCHSUPPRESSION_T42:
		process_T42_message(message, mxt);
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev, "[TSP] Receiving Touch suppression msg\n");
		break;

	case MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		if (debug >= DEBUG_TRACE)
			dev_info(&client->dev,
			"[TSP] Receiving one-touch gesture msg\n");

		event = message[MXT_MSG_T24_STATUS] & 0x0F;
		xpos = message[MXT_MSG_T24_XPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 4) & 0x0F);
		ypos = message[MXT_MSG_T24_YPOSMSB] * 16 +
			((message[MXT_MSG_T24_XYPOSLSB] >> 0) & 0x0F);
		xpos >>= 2;
		ypos >>= 2;

		switch (event) { 
		case	MT_GESTURE_RESERVED:
			break;
		case	MT_GESTURE_PRESS:
			break;
		case	MT_GESTURE_RELEASE:
			break;
		case	MT_GESTURE_TAP:
			break;
		case	MT_GESTURE_DOUBLE_TAP:
			break;
		case	MT_GESTURE_FLICK:
			break;
		case	MT_GESTURE_DRAG:
			break;
		case	MT_GESTURE_SHORT_PRESS:
			break;
		case	MT_GESTURE_LONG_PRESS:
			break;
		case	MT_GESTURE_REPEAT_PRESS:
			break;
		case	MT_GESTURE_TAP_AND_PRESS:
			break;
		case	MT_GESTURE_THROW:
			break;
		default:
			break;
		}
		break;

		case MXT_SPT_SELFTEST_T25:
			if (debug >= DEBUG_TRACE) { 
				dev_info(&client->dev,"[TSP] Receiving Self-Test msg\n");
			}

			if (message[MXT_MSG_T25_STATUS] == MXT_MSGR_T25_OK) { 
				if (debug >= DEBUG_TRACE)
					dev_info(&client->dev,
					"[TSP] maXTouch: Self-Test OK\n");

			} else  { 
				dev_err(&client->dev,
					"[TSP] maXTouch: Self-Test Failed [%02x]:"
					"{ %02x,%02x,%02x,%02x,%02x}\n",
					message[MXT_MSG_T25_STATUS],
					message[MXT_MSG_T25_STATUS + 0],
					message[MXT_MSG_T25_STATUS + 1],
					message[MXT_MSG_T25_STATUS + 2],
					message[MXT_MSG_T25_STATUS + 3],
					message[MXT_MSG_T25_STATUS + 4]
					);
			}
			break;

		case MXT_SPT_CTECONFIG_T46:
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,
				"[TSP] Receiving CTE message...\n");
			status = message[MXT_MSG_T46_STATUS];
			if (status & MXT_MSGB_T46_CHKERR)
				dev_err(&client->dev,
				"[TSP] maXTouch: Power-Up CRC failure\n");

			break;
		default:
			if (debug >= DEBUG_TRACE)
				dev_info(&client->dev,"[TSP] maXTouch: Unknown message!\n");

			break;
	}

	return 0;
}

static int mxt_need_reset=0,mxt_reset_flag=0;

u8 touch_reset_flag = 0;

static void mxt_reset_delayed_work(struct work_struct *work)
{ 
    
	struct mxt_data *mxt;
	printk("%s %d, [%d][tsp]\n", __func__, __LINE__, touch_reset_flag);

	mxt = container_of(work, struct mxt_data, mxt_reset_dwork.work);

	disable_irq(mxt->client->irq);
	
	if(touch_reset_flag)
	{
		mxt_sw_reset(mxt);
		msleep(60);
	}
	touch_reset_flag = 0;
	
	facesup_message_flag = 1;

	check_chip_calibration(mxt);
	enable_irq(mxt->client->irq);

	return;	
}

static irqreturn_t mxt224E_irq_thread(int irq, void *ptr)
{
	struct mxt_data *data = ptr;
	u8 report_id;
	u8 touch_message_flag = 0;
	u8 msg[data->msg_object_size];

	static int report_bfr_num = 0;
	int report_num = 0;
	int i;









	
	do {
		if(read_mem(data, data->msg_proc, sizeof(msg), msg) < 0)
		{
			goto error;
		}




		if(msg[0] == SUPPRESS_MSG){
			if(msg[1] == SUPPRESS_MSG_PRESS){
				printk("%s suppress\n", __func__);
				schedule_delayed_work(&data->suppress_timer_dwork, msecs_to_jiffies(2000));
			}
			if(cal_check_flag == 1){
				process_T42_message(msg, data);
			}
		}

		report_id = msg[0] - 2;












		
		if (report_id < 0 || report_id >= 5) {
			continue;
		}
		
		if (data->finger_mask & (1U << report_id)) {
			report_input_data(data);
			mxt_need_reset=0;
		}

		if ((msg[1] & RELEASE_MSG_MASK) || (msg[1] & SUPPRESS_MSG_MASK)) {
			if(msg[1] & RELEASE_MSG_MASK){
				cancel_delayed_work(&data->suppress_timer_dwork);
			}
			data->fingers[report_id].z = msg[5]; 
			data->fingers[report_id].w = msg[6]; 			
			data->finger_mask |= 1U << report_id;
			data->fingers[report_id].status = RELEASE_MSG_MASK;
			mxt_need_reset=0;

			if(mxt_writecfg_status){
				if((data->fingers[0].status == RELEASE_MSG_MASK) ||
						(data->fingers[1].status == RELEASE_MSG_MASK) ||
						(data->fingers[2].status == RELEASE_MSG_MASK) ||
						(data->fingers[3].status == RELEASE_MSG_MASK) ||
						(data->fingers[4].status == RELEASE_MSG_MASK))
				{
					mxt_release_all_keys(data);
					mxt_release_fingers(data);
					mxt_writecfg_status=0;
				}
			}

			if(resume_flag && !cal_check_flag)
			{
				if((data->fingers[0].status == RELEASE_MSG_MASK) ||
					(data->fingers[1].status == RELEASE_MSG_MASK) ||
					(data->fingers[2].status == RELEASE_MSG_MASK) ||
					(data->fingers[3].status == RELEASE_MSG_MASK) ||
					(data->fingers[4].status == RELEASE_MSG_MASK))
				{
					if(resume_flag++ > 1)
					{
						resume_flag = 0;
						schedule_delayed_work(&data->call_recovery_dwork, msecs_to_jiffies(0));
					}
				}
			}
			
		    if(touch_mode == TOUCH_GLOVE)
			{
				if((calibration_complete_count >= 1) && (calibration_complete_count <= 10))
		 		{
              		if((data->fingers[0].status == RELEASE_MSG_MASK) ||
						(data->fingers[1].status == RELEASE_MSG_MASK) ||
						(data->fingers[2].status == RELEASE_MSG_MASK) ||
						(data->fingers[3].status == RELEASE_MSG_MASK) ||
						(data->fingers[4].status == RELEASE_MSG_MASK))
			    	{
                 		if(finger_check==report_id)  {
				 			if(0) finger_recovery(data); 
							calibration_complete_count++; 
							mxt_swreset_cancel = 0;
			        	}
			    	}
			 	}
			}
			else
			{
				if((calibration_complete_count >= 1) && (calibration_complete_count <= MAX_FINGER_RECOVERY_CNT))
		 		{
              		if((data->fingers[0].status == RELEASE_MSG_MASK) ||
						(data->fingers[1].status == RELEASE_MSG_MASK) ||
						(data->fingers[2].status == RELEASE_MSG_MASK) ||
						(data->fingers[3].status == RELEASE_MSG_MASK) ||
						(data->fingers[4].status == RELEASE_MSG_MASK))
			    	{
                 		if(finger_check==report_id)
						{
				 			if(0) finger_recovery(data); 
							calibration_complete_count++; 
							mxt_swreset_cancel = 0;
			        	}				
			    	}
			 	}
			}
		 	
			
			if (data->finger_mask)	report_input_data(data);
		} else if ((msg[1] & DETECT_MSG_MASK) && (msg[1] &
				(PRESS_MSG_MASK | MOVE_MSG_MASK))) {
			cancel_delayed_work(&data->suppress_timer_dwork);
			data->fingers[report_id].z = msg[5]; 
			data->fingers[report_id].w = msg[6]; 			
			data->fingers[report_id].x = ((msg[2] << 4) | (msg[4] >> 4)) >>
							data->x_dropbits;
			data->fingers[report_id].y = ((msg[3] << 4) |
					(msg[4] & 0xF)) >> data->y_dropbits;
			data->finger_mask |= 1U << report_id;
			data->fingers[report_id].status = (PRESS_MSG_MASK | MOVE_MSG_MASK);
			
			if(0){
				int mask=0;
				if(msg[1]&PRESS_MSG_MASK)
					mask = 1;
				else if(msg[1]&MOVE_MSG_MASK)
					mask = 0;
				else
					printk("%s %d, --------- error [tsp]\n", __func__, __LINE__);
				
				equalize_coordinate2(mask, report_id, &data->fingers[report_id].x, &data->fingers[report_id].y);

				
			}
			
			mxt_need_reset=0;
			touch_message_flag = 1;
			
            if(touch_mode == TOUCH_GLOVE)
            {
            	if((calibration_complete_count >= 1) && (calibration_complete_count <= 10))
				{
					finger_check=report_id;
            	}
			}
			else
			{
               if((calibration_complete_count>=1) && (calibration_complete_count <= MAX_FINGER_RECOVERY_CNT))
	     	    {
					finger_check=report_id;
			    }
			}
			

			if (data->finger_mask)	report_input_data(data);
		}

















		else {
			if(0)
			{
				if(msg[0]==0x10 && msg[1]==0x0)
				{
					mxt_need_reset++;
					if(!mxt_reset_flag) goto error;
				}
				else if(msg[0]==0x10 && msg[1]==0x1 && msg[2]==0 && msg[3]==0)
				{
					mxt_need_reset++;
					if(!mxt_reset_flag) goto error;
				}
				else if(msg[0]==0x1 && msg[1]==0x10 && msg[2]==0x7d && msg[3]==0x9a)
				{
					mxt_need_reset++;
					if(!mxt_reset_flag) goto error;
				}
				else if(msg[0]==0x1 && msg[1]==0 && msg[2]==0x7d && msg[3]==0x9a)
				{
					mxt_need_reset++;
					if(!mxt_reset_flag) goto error;
				}
			}
		}
	} while(!gpio_get_value(data->gpio_read_done));
	
error:









	if(cal_check_flag)
	{
		for (i = 0; i < data->num_fingers; i++) {
			if (!data->fingers[i].status)
				continue;
			if ( data->fingers[i].status & RELEASE_MSG_MASK ) {
				
				
			} else {
				report_num++;
			}
		}

		printk("[TSP] *** %s report_bfr_num = %d -> report_num = %d calibrate_timer_wait = %d\n",
			__func__, report_bfr_num,report_num, calibrate_timer_wait);




		if( report_bfr_num && report_num == 0 && calibrate_timer_wait)
		{
			calibrate_timer_start(data);
			calibrate_timer_wait = false;
		}
		else if( report_bfr_num == 0 && report_num >= 1 )
		{
			calibrate_timer_stop(data);
			calibrate_timer_wait = false;
			facesup_message_flag = 0;
		} else if( calibrate_timer_wait && (report_num > 1 ) ){
			printk("[TSP] %s finger detect calibrate_timer_wait cancel calibrate_timer_wait = %d  facesup_message_flag = %d\n",
			__func__, calibrate_timer_wait, facesup_message_flag);
			calibrate_timer_wait = false;
			facesup_message_flag = 0;
		}
		report_bfr_num = report_num;
	}
	else
	{
		report_bfr_num = 0;
	}


	
	if(0){
		printk("[TOUCH] mxt_reset.\n");
		mxt_reset_flag=1;
		mxt_need_reset=0;
		
		
		mxt_sw_reset(data);
		msleep(150);
		mxt_reset_flag=0;
	}







	if(touch_message_flag && (cal_check_flag) && !ts_store_obj_status && !facesup_message_flag)
	{
		if(timer_flag == DISABLE)
		{
			
			cancel_delayed_work(&mxt224E_data->mxt_reset_dwork);
			schedule_delayed_work(&mxt224E_data->mxt_reset_dwork, msecs_to_jiffies(0));
		}
	}
	
	return IRQ_HANDLED;
}

int mxt_write_ap(struct mxt_data *mxt, u16 ap)
{
	struct i2c_client *client;
	__le16	le_ap = cpu_to_le16(ap);
	int retry_cnt = 0;				

	client = mxt->client;
	if (mxt != NULL)
		mxt->last_read_addr = -1;











    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if (i2c_master_send(client, (u8 *) &le_ap, 2) == 2) {
			mxt_debug(DEBUG_TRACE, "Address pointer set to %d\n", ap);
			return 0;
		} else {
			mxt_debug(DEBUG_INFO, "Error writing address pointer!(%d)\n",retry_cnt);
		}
	}

	printk(KERN_ERR "[T][ARM]Event:0x68 Info:0x07\n");
	return -EIO;


}

static int chk_obj(u8 type)
{
	switch (type) {
	case	GEN_POWERCONFIG_T7:
	case	GEN_ACQUISITIONCONFIG_T8:
	case	TOUCH_MULTITOUCHSCREEN_T9:
	case	TOUCH_KEYARRAY_T15:
	case	SPT_COMCONFIG_T18:
	case	SPT_GPIOPWM_T19:
	case	TOUCH_PROXIMITY_T23:
	case	PROCI_GRIPSUPPRESSION_T40:
	case	PROCI_TOUCHSUPPRESSION_T42:
	case	SPT_CTECONFIG_T46:
	case	PROCI_STYLUS_T47:
	case	PROCG_NOISESUPPRESSION_T48:
		return 0;
	default:
		return -1;
	}
}


int write_wireless_config(struct mxt_data *mxt)
{
	int ret;
	int i;
	
	for (i = 0; mxt224E_wireless_config[i][0] != RESERVED_T255; i++) {
		ret = write_config(mxt, mxt224E_wireless_config[i][0],
							mxt224E_wireless_config[i] + 1);
		if (ret)
			goto err_config;
	}

	return 0;
err_config:
	printk("TOUCH write config error\n");
	return -1;
}


static ssize_t set_nvbackup(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	int ret;
	struct mxt_data *mxt;

	mxt = dev_get_drvdata(dev);

	ret = mxt224E_backup(mxt);
	if (ret)
		return 0;

	mdelay(25);		
	
	ret = mxt224E_reset(mxt);
	if (ret)
		return 0;

	msleep(60);

	return count;
}

static ssize_t get_nvbackup(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{

	return (ssize_t) 0;
}

static ssize_t store_ts_reset(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{
	struct mxt_data *mxt;

	mxt = dev_get_drvdata(dev);

	mxt_sw_reset(mxt);

	mdelay(100);

	return count;
}

static ssize_t show_ts_reset(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct mxt_data *mxt;

	mxt = dev_get_drvdata(dev);

	mxt_sw_reset(mxt);

	mdelay(100);
	
	return (ssize_t) 0;
}

static ssize_t show_virtual_poweroff(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct mxt_data *mxt;

	mxt = dev_get_drvdata(dev);

	mxt->power_off();
	return (ssize_t) 0;
}

static ssize_t show_cradle_status(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct mxt_data *mxt;
	int cradle_chg_int,count = 0;

	
	mxt = dev_get_drvdata(dev);
	
	cradle_chg_int = gpio_get_value(GPIO_CRADLE_CHG_INT);
	count = sprintf(buf, "%d", !cradle_chg_int);
	printk(KERN_INFO "##### cradle_chg_int = %d\n",buf[0]);
	return (ssize_t) count;
}
static ssize_t show_wireless_status(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct mxt_data *mxt;
	int wireless_chg_int,count = 0;

	
	mxt = dev_get_drvdata(dev);

	wireless_chg_int = gpio_get_value(GPIO_WIRELESS_CHG_INT);
	count = sprintf(buf, "%d", !wireless_chg_int);
	printk(KERN_INFO "##### wireless_chg_int = %d\n",buf[0]);
	return (ssize_t) count;
}

static ssize_t show_batt_mvolts(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct mxt_data *mxt;
	int battery_mvolts,count = 0;
	
	mxt = dev_get_drvdata(dev);
	
	battery_mvolts = pm8921_get_prop_battery_uvolts()/1000;
	count = sprintf(buf, "%d", battery_mvolts);
	printk(KERN_INFO "##### battery_uvolts = %d\n",buf[0]);
	return (ssize_t) count;
}

static ssize_t set_ap(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf,
			  size_t count)
{

	int i;
	struct i2c_client *client;
	struct mxt_data *mxt;
	u16 ap;

	client = to_i2c_client(dev);
	mxt = i2c_get_clientdata(client);

	if (count < 3) {
		
		printk(KERN_INFO "set_ap needs to arguments: address pointer "
			   "and data size");
		return -EIO;
	}

	ap = (u16) *((u16 *)buf);
	i = mxt_write_ap(mxt, ap);
	mxt->bytes_to_read = (u16) *(buf + 2);
	return count;

}

static ssize_t ts_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	
	buf[0] = mxt->device_info.major;
	return (ssize_t)count;
}

static ssize_t diag_normal_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_NORMAL, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_NORMAL, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_glove_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_GLOVE, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_GLOVE, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_hardkey_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_HARDKEY, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_HARDKEY, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_softkey_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_SOFTKEY, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_SOFTKEY,0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_keyled_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_KEYLED, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_KEYLED, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_backlight_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_POWER, 1);
	input_sync(mxt->input_dev);
	msleep(200);
	input_report_key(mxt->input_dev, KEY_POWER, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_white_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_WHITE, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_WHITE, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_rainbow_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_RAINBOW, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_RAINBOW, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_frame_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_FRAME, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_FRAME, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_softbl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_SOFTBL, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_SOFTBL, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_normal2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_NORMAL2, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_NORMAL2, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}
static ssize_t diag_glove2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_GLOVE2, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_GLOVE2, 0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}


static ssize_t diag_nfcmenu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	int count = 0;
	printk("[DIAG] %s\n",__func__);
	mxt = dev_get_drvdata(dev);
	input_report_key(mxt->input_dev, KEY_DIAG_NFCMENU, 1);
	input_sync(mxt->input_dev);
	msleep(500);
	input_report_key(mxt->input_dev, KEY_DIAG_NFCMENU,0);
	input_sync(mxt->input_dev);
	return (ssize_t)count;
}



static ssize_t diag_key_emulation(struct device *dev, 
                                      struct device_attribute *attr, const char *buf, size_t count)
{
  	struct mxt_data *mxt;
  	u8 press;
  	u16 emul_key;
  	printk("\n[DIAG/KeyEmulation] %s\n",__func__);

  	press = (u8) *((u8 *)(buf));
  	emul_key = (u16) *((u16 *)(buf+1));
  	if (debug >= DEBUG_INFO)
    	printk("[DIAG/KeyEmulation] %d, emul_key: %d, 0x%X\n", press, emul_key, emul_key);

  	mxt = dev_get_drvdata(dev);

  	if(press == 1) {
    	input_report_key(mxt->input_dev, emul_key, 1);
    	input_sync(mxt->input_dev);
  	}
  	else {
    	input_report_key(mxt->input_dev, emul_key, 0);
    	input_sync(mxt->input_dev);
  	}
  
	return (ssize_t)count;
}


void mxt_normal_config_write(void)
{ 
	int ret,i=0;

	
	
	
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_normal_config_write [%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt224E_data);
			cal_check_flag = 0;
		}

		schedule_delayed_work(&mxt224E_data->normal_dwork, msecs_to_jiffies(50));
		return;
	}

	touch_mode = TOUCH_NORMAL;
	

	if(mxt_suspend_status==0)
	{
		printk("\n[TOUCH MODE] ####### NORMAL TOUCH1 ######\n");
		printk("\n[TOUCH MODE] ####### NORMAL TOUCH ######\n");
		printk("\n[TOUCH MODE] ####### NORMAL TOUCH ######\n");
		last_apply_mode = TOUCH_NORMAL;
		mutex_lock(&mxt224E_data->touch_mutex); 
		ts_store_obj_status=1;
		for(i=0;i<normal_cfg_max;i++)
		{
			write_temp_config(mxt224E_data,normal_cfg_config[i][0],normal_cfg_config[i][1],normal_cfg_config[i][2]);
		}
		if(resume_flag)
			mxt_call_recovery(mxt224E_data);
		ret = mxt224E_backup(mxt224E_data);
		if (ret)
		{
			printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
		}
		
		ts_store_obj_status=0;
		mxt_writecfg_status = 1;

		mutex_unlock(&mxt224E_data->touch_mutex); 
	}
	else
	{
		schedule_delayed_work(&mxt224E_data->normal_dwork, msecs_to_jiffies(5000));
	}
	
	return;	
}
void mxt_glove_config_write(void)
{ 
	int ret,i=0;

	
	
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_glove_config_write [%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt224E_data);
			cal_check_flag = 0;
		}

		schedule_delayed_work(&mxt224E_data->glove_dwork, msecs_to_jiffies(50));
		return;
	}

	touch_mode = TOUCH_GLOVE;
	
	if(mxt_suspend_status==0)
	{
		printk("\n[TOUCH MODE] ####### GLOVE TOUCH1 ######\n");
		printk("\n[TOUCH MODE] ####### GLOVE TOUCH ######\n");
		printk("\n[TOUCH MODE] ####### GLOVE TOUCH ######\n");
		last_apply_mode = TOUCH_GLOVE;
	
		mutex_lock(&mxt224E_data->touch_mutex); 
		ts_store_obj_status=1;
		for(i=0;i<glove_cfg_max;i++)
		{
			write_temp_config(mxt224E_data,glove_cfg_config[i][0],glove_cfg_config[i][1],glove_cfg_config[i][2]);
		}
		if(resume_flag)
			mxt_call_recovery(mxt224E_data);
		ret = mxt224E_backup(mxt224E_data);
		if (ret)
		{
			printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
		}
		
		ts_store_obj_status=0;
		mxt_writecfg_status = 1;

		mutex_unlock(&mxt224E_data->touch_mutex); 
	}
	else
	{
		schedule_delayed_work(&mxt224E_data->glove_dwork, msecs_to_jiffies(5000));
	}
	
	return;	
}

void mxt_charger_config_write(void)
{ 
	int ret,i=0;

	
	
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_charger_config_write [%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt224E_data);
			cal_check_flag = 0;
		}

		schedule_delayed_work(&mxt224E_data->charger_dwork, msecs_to_jiffies(50));
		return;
	}

	if(mxt_suspend_status==0)
	{
		printk("\n[TOUCH MODE] ####### CHARGER TOUCH1 ######\n");
		printk("\n[TOUCH MODE] ####### CHARGER TOUCH ######\n");
		printk("\n[TOUCH MODE] ####### CHARGER TOUCH ######\n");
		last_apply_mode = TOUCH_CHARGE;
		mutex_lock(&mxt224E_data->touch_mutex); 
		ts_store_obj_status=1;
		for(i=0;i<charger_cfg_max;i++)
		{
			write_temp_config(mxt224E_data,charger_cfg_config[i][0],charger_cfg_config[i][1],charger_cfg_config[i][2]);			
		}
		if(resume_flag)
			mxt_call_recovery(mxt224E_data);
		ret = mxt224E_backup(mxt224E_data);
		if (ret){
			printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
		}
		
		ts_store_obj_status=0;
		mxt_writecfg_status = 1;

		mutex_unlock(&mxt224E_data->touch_mutex); 
	}
	else{
		schedule_delayed_work(&mxt224E_data->charger_dwork, msecs_to_jiffies(5000));
	}
	
	return;	
}

void mxt_wireless_config_write(void)
{ 
	int ret,i=0;

	
	
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_wireless_config_write [%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt224E_data);
			cal_check_flag = 0;
		}

		schedule_delayed_work(&mxt224E_data->wireless_dwork, msecs_to_jiffies(50));
		return;
	}

	if(mxt_suspend_status==0){
		printk("\n[TOUCH MODE] ####### WIRELESS TOUCH1 ######\n");
		printk("\n[TOUCH MODE] ####### WIRELESS TOUCH ######\n");
		printk("\n[TOUCH MODE] ####### WIRELESS TOUCH ######\n");
		last_apply_mode = TOUCH_WIRELESS;
		mutex_lock(&mxt224E_data->touch_mutex); 
		ts_store_obj_status=1;
		for(i=0;i<wireless_cfg_max;i++)
		{
			write_temp_config(mxt224E_data,wireless_cfg_config[i][0],wireless_cfg_config[i][1],wireless_cfg_config[i][2]);
		}
		if(resume_flag)
			mxt_call_recovery(mxt224E_data);

		ret = mxt224E_backup(mxt224E_data);
		if (ret){
			printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
		}
		
		ts_store_obj_status=0;
		mxt_writecfg_status = 1;

		mutex_unlock(&mxt224E_data->touch_mutex); 
	}
	else{
		schedule_delayed_work(&mxt224E_data->wireless_dwork, msecs_to_jiffies(3000));
	}
	
	return;	
}


static void mxt_normal_config_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	int ret,i=0;
	mxt = container_of(work, struct mxt_data, normal_dwork.work);

	cancel_delayed_work(&mxt->charger_dwork);
	cancel_delayed_work(&mxt->wireless_dwork);
	cancel_delayed_work(&mxt->glove_dwork);
	cancel_delayed_work(&mxt->normal_dwork);
	cancel_delayed_work(&mxt->mxt_reset_dwork);
	cancel_delayed_work(&mxt->call_recovery_dwork);
	cancel_delayed_work(&mxt->calibrate_timer_dwork);
	cancel_delayed_work(&mxt->suppress_timer_dwork);
	calibrate_timer_stop(mxt);
	
	if(last_apply_mode == TOUCH_NORMAL)
		return;
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{	
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_normal_config_delayed_work return[%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt);
			cal_check_flag = 0;
		}
		schedule_delayed_work(&mxt->normal_dwork, msecs_to_jiffies(50));
		return;
	}

	touch_mode = TOUCH_NORMAL;
	
	printk("\n[TOUCH MODE] ####### NORMAL TOUCH2 ######\n\n");

	cal_check_flag = 1;
	
	
	last_apply_mode = TOUCH_NORMAL;
	mutex_lock(&mxt->touch_mutex); 
	ts_store_obj_status=1;
	for(i=0;i<normal_cfg_max;i++)
	{
		write_temp_config(mxt,normal_cfg_config[i][0],normal_cfg_config[i][1],normal_cfg_config[i][2]);
	}
	
	if(resume_flag)
		mxt_call_recovery(mxt224E_data);
	
	ret = mxt224E_backup(mxt);
	if (ret){
		printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
	}
	
	
	ts_store_obj_status=0;
	mxt_writecfg_status = 1;

	mutex_unlock(&mxt->touch_mutex); 
	
	return;	
}

static void mxt_charger_config_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	int ret,i=0;
	mxt = container_of(work, struct mxt_data, charger_dwork.work);

	cancel_delayed_work(&mxt->charger_dwork);
	cancel_delayed_work(&mxt->wireless_dwork);
	cancel_delayed_work(&mxt->glove_dwork);
	cancel_delayed_work(&mxt->normal_dwork);
	cancel_delayed_work(&mxt->mxt_reset_dwork);
	cancel_delayed_work(&mxt->call_recovery_dwork);
	cancel_delayed_work(&mxt->calibrate_timer_dwork);
	cancel_delayed_work(&mxt->suppress_timer_dwork);
	calibrate_timer_stop(mxt);

	if(last_apply_mode == TOUCH_CHARGE)
		return;
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_charger_config_delayed_work return[%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt);
			cal_check_flag = 0;
		}
		schedule_delayed_work(&mxt->charger_dwork, msecs_to_jiffies(50));
		return;
	}

	printk("\n[TOUCH MODE] ####### CHARGER TOUCH2 ######\n\n");
	
	cal_check_flag = 1;
	
	
	last_apply_mode = TOUCH_CHARGE;
	mutex_lock(&mxt->touch_mutex); 
	ts_store_obj_status=1;
	for(i=0;i<charger_cfg_max;i++)
	{
		write_temp_config(mxt,charger_cfg_config[i][0],charger_cfg_config[i][1],charger_cfg_config[i][2]);
	}
	
	if(resume_flag)
		mxt_call_recovery(mxt224E_data);
	
	ret = mxt224E_backup(mxt);
	if (ret){
		printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
	}
	
	
	ts_store_obj_status=0;
	mxt_writecfg_status = 1;

	mutex_unlock(&mxt->touch_mutex); 
	
	return;	
}

static void mxt_wireless_config_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	int ret,i=0;
	mxt = container_of(work, struct mxt_data, wireless_dwork.work);

	cancel_delayed_work(&mxt->charger_dwork);
	cancel_delayed_work(&mxt->wireless_dwork);
	cancel_delayed_work(&mxt->glove_dwork);
	cancel_delayed_work(&mxt->normal_dwork);
	cancel_delayed_work(&mxt->mxt_reset_dwork);
	cancel_delayed_work(&mxt->call_recovery_dwork);
	cancel_delayed_work(&mxt->calibrate_timer_dwork);
	cancel_delayed_work(&mxt->suppress_timer_dwork);
	calibrate_timer_stop(mxt);
	
	if(last_apply_mode == TOUCH_WIRELESS)
		return;
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_wireless_config_delayed_work [%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt);
			cal_check_flag = 0;
		}

		schedule_delayed_work(&mxt->wireless_dwork, msecs_to_jiffies(50));
		return;
	}

	printk("\n[TOUCH MODE] ####### WIRELESS TOUCH2 ######\n\n");

	cal_check_flag = 1;
	
	
	last_apply_mode = TOUCH_WIRELESS;
	mutex_lock(&mxt->touch_mutex); 
	ts_store_obj_status=1;
	for(i=0;i<wireless_cfg_max;i++)
	{
		write_temp_config(mxt,wireless_cfg_config[i][0],wireless_cfg_config[i][1],wireless_cfg_config[i][2]);
	}
	
	if(resume_flag)
		mxt_call_recovery(mxt224E_data);

	ret = mxt224E_backup(mxt);
	if (ret){
		printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
	}
	
	
	ts_store_obj_status=0;
	mxt_writecfg_status = 1;

	mutex_unlock(&mxt->touch_mutex); 
	
	return;	
}

static void mxt_glove_config_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	int ret,i=0;
	mxt = container_of(work, struct mxt_data, glove_dwork.work);

	cancel_delayed_work(&mxt->charger_dwork);
	cancel_delayed_work(&mxt->wireless_dwork);
	cancel_delayed_work(&mxt->glove_dwork);
	cancel_delayed_work(&mxt->normal_dwork);
	cancel_delayed_work(&mxt->mxt_reset_dwork);
	cancel_delayed_work(&mxt->call_recovery_dwork);
	cancel_delayed_work(&mxt->calibrate_timer_dwork);
	cancel_delayed_work(&mxt->suppress_timer_dwork);
	calibrate_timer_stop(mxt);

	if(last_apply_mode == TOUCH_GLOVE)
		return;
	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_glove_config_delayed_work [%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt);
			cal_check_flag = 0;
		}

		schedule_delayed_work(&mxt->glove_dwork, msecs_to_jiffies(50));
		return;
	}

	touch_mode = TOUCH_GLOVE;
	
	printk("\n[TOUCH MODE] ####### GLOVE TOUCH2 ######\n\n");

	cal_check_flag = 1;
	

	last_apply_mode = TOUCH_GLOVE;
	mutex_lock(&mxt->touch_mutex); 
	ts_store_obj_status=1;
	for(i=0;i<glove_cfg_max;i++)
	{
		write_temp_config(mxt,glove_cfg_config[i][0],glove_cfg_config[i][1],glove_cfg_config[i][2]);
	}
	
	if(resume_flag)
		mxt_call_recovery(mxt224E_data);
	
	ret = mxt224E_backup(mxt);
	if (ret){
		printk("[TOUCH] %s -> config RAM save fail !!!\n",__func__);
	}
	
	
	ts_store_obj_status=0;
	mxt_writecfg_status = 1;

	mutex_unlock(&mxt->touch_mutex); 
}

static void mxt_resume_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	
	mxt = container_of(work, struct mxt_data, resume_dwork.work);
	mutex_lock(&mxt->touch_mutex); 

	cancel_delayed_work(&mxt->charger_dwork);
    cancel_delayed_work(&mxt->wireless_dwork);
    cancel_delayed_work(&mxt->glove_dwork);
    cancel_delayed_work(&mxt->normal_dwork);

	cal_check_flag = 1;
	resume_change = 0;

	if(!ts_lcd_flag)
    {

		mutex_unlock(&mxt->touch_mutex);

        printk("do not write i2c cuz LDC29 is off line[%d]\n", __LINE__);
        return;
    }

	charge_mode = pm_power_get_charger_mode();

    if(last_charge_mode != charge_mode || last_touch_mode != touch_mode)
    {
		resume_change = 1;
        if(charge_mode==TOUCH_WALL_CHARGE || charge_mode==TOUCH_USB_CHARGE || charge_mode == TOUCH_CRADLE_CHARGE){
			
            schedule_delayed_work(&mxt->charger_dwork, msecs_to_jiffies(30));
        }
        else if(charge_mode==TOUCH_WIRELESS_CHARGE){
            schedule_delayed_work(&mxt->wireless_dwork, msecs_to_jiffies(30));
			
        }
        else if(charge_mode==TOUCH_NOT_CHARGE){
            if(touch_mode == TOUCH_GLOVE )
			{
				
                schedule_delayed_work(&mxt->glove_dwork, msecs_to_jiffies(30));
			}
            else
			{
                schedule_delayed_work(&mxt->normal_dwork, msecs_to_jiffies(30));
				
			}
        }
    }

    last_charge_mode = charge_mode;
    last_touch_mode = touch_mode;





	
	mutex_unlock(&mxt->touch_mutex); 
	
	resume_flag = 1;
	return;	
}

static void mxt_call_recovery_delayed_work(struct work_struct *work)
{ 
    
	struct mxt_data *mxt;

	mxt = container_of(work, struct mxt_data, call_recovery_dwork.work);

	if(ts_store_obj_status || !ts_lcd_flag || mxt_suspend_status || facesup_message_flag)
	{	
		if(TSP_LOG) printk("\n[TOUCH MODE] ####### mxt_call_recovery_delayed_work return[%d][%d][%d]######\n", ts_store_obj_status, mxt_suspend_status, facesup_message_flag);
		if(facesup_message_flag){
			timer_flag = DISABLE;
			timer_ticks = 10;
			facesup_message_flag = 0;
			ts_100ms_timer_stop(mxt);
			cal_check_flag = 0;
		}
		schedule_delayed_work(&mxt->call_recovery_dwork, msecs_to_jiffies(50));
		return;
	}

	cal_check_flag = 0;

	if(touch_mode == TOUCH_GLOVE)
	{
		write_temp_config(mxt,46,2,32);
		write_temp_config(mxt,46,3,63);
	}

	write_temp_config(mxt,42,0,3);
	write_temp_config(mxt,8,6,0);
	write_temp_config(mxt,8,7,1);




	
    printk("[tsp]call_recovery_delayed__\n");
	return;	
}


static void mxt_charging_delayed_work(struct work_struct *work)
{ 
	struct mxt_data *mxt;
	mxt = container_of(work, struct mxt_data, charging_dwork.work);

	cancel_delayed_work(&mxt->charger_dwork);
	cancel_delayed_work(&mxt->wireless_dwork);
	cancel_delayed_work(&mxt->glove_dwork);
	cancel_delayed_work(&mxt->normal_dwork);
	cancel_delayed_work(&mxt->resume_dwork);
	
	charge_mode = pm_power_get_charger_mode();

	

	if(!ts_lcd_flag)
    {
        printk("do not write i2c cuz LDC29 is off line return [%d]\n", __LINE__);
		
        return;
    }

	if(last_charge_mode != charge_mode || last_touch_mode != touch_mode)
	{
		printk("%s charge_mode = %d \n",__FUNCTION__,charge_mode);
		
		if(charge_mode==TOUCH_WALL_CHARGE || charge_mode==TOUCH_USB_CHARGE || charge_mode == TOUCH_CRADLE_CHARGE){
			schedule_delayed_work(&mxt->charger_dwork, msecs_to_jiffies(50)); 
		}
		else if(charge_mode==TOUCH_WIRELESS_CHARGE){
			schedule_delayed_work(&mxt->wireless_dwork, msecs_to_jiffies(50));
		}
		else if(charge_mode==TOUCH_NOT_CHARGE){
			if(touch_mode == TOUCH_GLOVE )
				schedule_delayed_work(&mxt->glove_dwork, msecs_to_jiffies(50));
			else
				schedule_delayed_work(&mxt->normal_dwork, msecs_to_jiffies(50));
		}
	}

	mutex_lock(&mxt->touch_mutex); 
	last_charge_mode = charge_mode;
	last_touch_mode = touch_mode;
	

	
	
	
	mxt_swreset_status = 0;
	mutex_unlock(&mxt->touch_mutex); 
	schedule_delayed_work(&mxt->charging_dwork, msecs_to_jiffies(300));

	return;	
}

static ssize_t show_object(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mxt_data *mxt;
	struct object_t *object_table;

	int count = 0;
	int i, j;
	u8 val;
	u16 size = 0; 
	u16 address = 0; 
	
	mxt = dev_get_drvdata(dev);
	object_table = mxt->objects;

	for (i = 0; i < mxt->device_info.num_objs; i++) {
		u8 obj_type = object_table[i].object_type;

		if (chk_obj(obj_type))
			continue;
		
		count += sprintf(buf + count, "%s: %d bytes\n", 
				object_type_name[obj_type], object_table[i].size);
		get_object_info(mxt, obj_type, &size, &address);
		for (j = 0; j < size; j++) {
			mxt_read_byte(mxt->client, address +(u16)j, &val);
			count += sprintf(buf + count,
					"  Byte %2d: 0x%02x (%d)\n", j, val, val);
		}

		count += sprintf(buf + count, "\n");
	}

	return count;
}

static ssize_t store_object(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *mxt;

	unsigned int type, offset, val;
	unsigned int cfg_type;

	mxt = dev_get_drvdata(dev);
	
	if ( (sscanf(buf, "%u %u %u %u", &cfg_type, &type, &offset, &val) != 4) || (type >= MXT_MAX_OBJECT_TYPES)) {
		printk("Invalid values \n");
		return -EINVAL;
	}
	


























	if(cfg_type == TOUCH_NORMAL){
		if(type == 1 && val == TOUCH_NORMAL){
			memset(normal_cfg_config,0,sizeof(normal_cfg_config));
			normal_cfg_count = 0;
		}
		else if(type == 0 && val == TOUCH_NORMAL){
			normal_cfg_max = normal_cfg_count;
			normal_cfg_count = 0;
		}
		else if(normal_cfg_count < CONFIG_MAX){
			normal_cfg_config[normal_cfg_count][0] = type;
			normal_cfg_config[normal_cfg_count][1] = offset;
			normal_cfg_config[normal_cfg_count][2] = val;
			normal_cfg_count++;
		}
		else{
			pr_err("[TSP] %s normal_config size over \n", __func__);
		}
	}
	else if(cfg_type == TOUCH_GLOVE){
		if(type == 1 && val == TOUCH_GLOVE){
			memset(glove_cfg_config,0,sizeof(glove_cfg_config));
			glove_cfg_count = 0;
		}
		else if(type == 0 && val == TOUCH_GLOVE){
			glove_cfg_max = glove_cfg_count;
			glove_cfg_count = 0;
		}
		else if(normal_cfg_count < CONFIG_MAX){
			glove_cfg_config[glove_cfg_count][0] = type;
			glove_cfg_config[glove_cfg_count][1] = offset;
			glove_cfg_config[glove_cfg_count][2] = val;
			glove_cfg_count++;
		}
		else{
			pr_err("[TSP] %s glove_config size over \n", __func__);
		}
	}
	else if(cfg_type == TOUCH_CHARGE){
		if(type == 1 && val == TOUCH_CHARGE){
			memset(charger_cfg_config,0,sizeof(charger_cfg_config));
			charger_cfg_count = 0;
		}
		else if(type == 0 && val == TOUCH_CHARGE){
			charger_cfg_max = charger_cfg_count;
			charger_cfg_count = 0;
		}
		else if(normal_cfg_count < CONFIG_MAX){
			charger_cfg_config[charger_cfg_count][0] = type;
			charger_cfg_config[charger_cfg_count][1] = offset;
			charger_cfg_config[charger_cfg_count][2] = val;
			charger_cfg_count++;
		}
		else{
			pr_err("[TSP] %s charger_config size over \n", __func__);
		}

	}
	else if(cfg_type == TOUCH_WIRELESS){
		if(type == 1 && val == TOUCH_WIRELESS){
			memset(wireless_cfg_config,0,sizeof(wireless_cfg_config));
			wireless_cfg_count = 0;
		}
		else if(type == 0 && val == TOUCH_WIRELESS){
			wireless_cfg_max = wireless_cfg_count;
			wireless_cfg_count = 0;
		}
		else if(normal_cfg_count < CONFIG_MAX){
			wireless_cfg_config[wireless_cfg_count][0] = type;
			wireless_cfg_config[wireless_cfg_count][1] = offset;
			wireless_cfg_config[wireless_cfg_count][2] = val;
			wireless_cfg_count++;
		}
		else{
			pr_err("[TSP] %s wireless_config size over \n", __func__);
		}
	}
	else if(cfg_type == SET_NOW){
		charge_mode = pm_power_get_charger_mode();
		
	    if(charge_mode != TOUCH_NOT_CHARGE ){
			if(charge_mode == TOUCH_WIRELESS_CHARGE)
				touch_mode = TOUCH_WIRELESS;
			else
				touch_mode = TOUCH_CHARGE;
	    }

    	printk("\n ###%s : touch_mode = %d charge_mode = %d \n\n",__FUNCTION__,touch_mode,charge_mode);

	    switch(touch_mode)
	    {
	        case TOUCH_NORMAL:
	            mxt_normal_config_write();
    	        break;
	        case TOUCH_GLOVE:
	            mxt_glove_config_write();
    	        break;
	        case TOUCH_WIRELESS:
	            mxt_wireless_config_write();
    	        break;
	        case TOUCH_CHARGE:
	            mxt_charger_config_write();
    	        break;
	        default:
    	        break;
	    }
	}


	return count;
}



































static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];
	int retry_cnt = 0;				

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;







    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if (i2c_master_send(client, buf, 2) != 2) {
			dev_err(&client->dev, "%s: i2c send failed(%d)\n", __func__,retry_cnt);
		}
		else{
			break;
		}
		
	}

	if( retry_cnt >= MXT_I2C_RETRY_CNT )
	{
		return -EIO;
	}


	return 0;
}

static int mxt_fw_write(struct i2c_client *client,const u8 *data, unsigned int frame_size)
{
	int retry_cnt = 0;				







    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		if (i2c_master_send(client, data, frame_size) != frame_size) {
			dev_err(&client->dev, "%s: i2c send failed(%d)\n", __func__, retry_cnt);
		}
		else{
			break;
		}
		
	}

	if( retry_cnt >= MXT_I2C_RETRY_CNT )
	{
		return -EIO;
	}


	return 0;
}

static int mxt_make_highchg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int count = 10;
	int error;
	u8 msg[data->msg_object_size];

	
	do {
		error = read_mem(data, data->msg_proc, sizeof(msg), msg);
		if (error)
			return error;
	} while (msg[0] != 0xff && --count);

	if (!count) {
		dev_err(dev, "CHG pin isn't cleared\n");
		return -EBUSY;
	}

	return 0;
}

static int mxt_boot_read_mem(struct i2c_client *client, unsigned char *mem)
{
	struct i2c_msg rmsg;
	int ret;
	int retry_cnt = 0;				

	rmsg.addr = client->addr;
	rmsg.flags = I2C_M_RD;
	rmsg.len = 1;
	rmsg.buf = mem;




    for( retry_cnt = 0; retry_cnt < MXT_I2C_RETRY_CNT; retry_cnt++ )
    {
		ret = i2c_transfer(client->adapter, &rmsg, 1);
		if (ret < 0){
			printk(KERN_ERR "%s: i2c_transfer error(%d) ret:%x\n",__func__, retry_cnt, ret);
		}
		else{
			break;
		}
		
	}


	return ret;
}

static int touch_firm_up_status = 0;

static int mxt_load_fw(struct mxt_data *data, const char *fn, size_t count)
{
	struct i2c_client *client = data->client;
	
	unsigned int frame_size = 0;
	unsigned int pos = 0;
	int ret = 0;

	unsigned char boot_status;
	unsigned char boot_ver;
	unsigned int  crc_error_count;
	unsigned int  next_frame;
	unsigned int  j, read_status_flag;
	
	unsigned char  *firmware_data;
	unsigned long int fw_size = 0;
	
	firmware_data = (unsigned char *)fn;
	fw_size = (unsigned long int)count;

	
	mxt_write_object(data, MXT_GEN_COMMANDPROCESSOR_T6,MXT_ADR_T6_RESET, MXT_BOOT_VALUE);
	msleep(MXT_RESET_TIME);

	
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	crc_error_count = 0;
	next_frame = 0;

	if ((mxt_boot_read_mem(client, &boot_status) == 1) &&
			(boot_status & MXT_WAITING_BOOTLOAD_CMD) == MXT_WAITING_BOOTLOAD_CMD) {
			
		boot_ver = boot_status & MXT_BOOT_STATUS_MASK;
		crc_error_count = 0;
		next_frame = 0;

		while (pos < fw_size) {
			for (j = 0; j < 5; j++) {
				if (mxt_boot_read_mem(client, &boot_status) == 1)	{
					read_status_flag = 1;
					break;
				} 
				else {
					mdelay(60);
					read_status_flag = 0;
				}
			}

			if (read_status_flag == 1) {
				if ((boot_status & MXT_WAITING_BOOTLOAD_CMD) == MXT_WAITING_BOOTLOAD_CMD) {
					if (mxt_unlock_bootloader(client) == 0) {
						mdelay(10);
						printk(KERN_INFO"Unlock OK\n");
					} 
					else {
						printk(KERN_INFO"Unlock fail\n");
					}
				} 
				else if ((boot_status & MXT_WAITING_BOOTLOAD_CMD) == MXT_WAITING_FRAME_DATA){
					frame_size = ((*(firmware_data + pos) << 8) | *(firmware_data + pos + 1));

					


					frame_size += 2;

					
					if (0 == frame_size) {
						printk(KERN_INFO"0 == frame_size\n");
						ret = 0;
						goto out;
					}
					next_frame = 1;
					
					mxt_fw_write(client, firmware_data + pos, frame_size);
					mdelay(10);
				} 
				else if (boot_status == MXT_FRAME_CRC_CHECK) {
					printk(KERN_INFO"CRC Check\n");
				} 
				else if (boot_status == MXT_FRAME_CRC_PASS) {
					if (next_frame == 1) {
						printk(KERN_INFO"CRC Ok\n");
						pos += frame_size;
						next_frame = 0;
						printk(KERN_INFO"Updated %d bytes / %lu bytes\n", pos, fw_size);
					} 
					else
						printk(KERN_INFO"next_frame != 1\n");
				} 
				else if (boot_status	== MXT_FRAME_CRC_FAIL) {
					printk(KERN_INFO"CRC Fail\n");
					crc_error_count++;
				}
				if (crc_error_count > 10) {
					ret = 1;
					goto out;
				}
			} 
			else {
				ret = 1;
				goto out;
			}
		}
	}
	else {
		printk(KERN_INFO"[TSP] read_boot_state() or (boot_status & 0xC0) == 0xC0) is fail!!!\n");
		ret = 1;
	}
out:
	
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;
	
	return ret;
}

static ssize_t update_fw_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int error;

	touch_firm_up_status = 1;
	
	disable_irq(data->client->irq);

	error = mxt_load_fw(data, buf, count);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		count = error;
	} 
	else {
		dev_dbg(dev, "The firmware update succeeded\n");

		
		msleep(MXT_FWRESET_TIME);

		kfree(data->objects);
		data->objects = NULL;

		mxt224E_init_touch_driver(data);
	}

	enable_irq(data->client->irq);

	error = mxt_make_highchg(data);

	touch_firm_up_status = 0;
	
	if (error)
		return error;

	return count;
}

static ssize_t show_touch_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct mxt_data *mxt;

    mxt = dev_get_drvdata(dev);
	
	printk("%s\n",__FUNCTION__);
	sprintf(buf,"%d",touch_mode);
	

    return (ssize_t) sizeof(touch_mode);
}

static ssize_t store_touch_mode(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	struct mxt_data *mxt;
	int temp_touch_mode;

	wake_lock(&touch_wake_lock);

	mxt = dev_get_drvdata(dev);

	sscanf(buf,"%d",&temp_touch_mode);
	
	touch_mode = temp_touch_mode;
	

	if(!ts_lcd_flag)
	{

		wake_unlock(&touch_wake_lock);

		printk("do not write i2c cuz LDC29 is off line[%d]\n", __LINE__);
		return 0;
	}

	if(last_apply_mode == -1)
	{

		wake_unlock(&touch_wake_lock);

		printk("[TSP] INIT NOT YET[%d]\n", __LINE__);
		return 0;		
	}
	printk("\n ###%s : touch_mode = %d charge_mode = %d \n\n",__FUNCTION__,touch_mode,charge_mode);
	
	if(charge_mode == TOUCH_NOT_CHARGE )
	{	
		switch(touch_mode)
		{
		case TOUCH_NORMAL:
			schedule_delayed_work(&mxt->normal_dwork, msecs_to_jiffies(0));
			
			break;
		case TOUCH_GLOVE:
			schedule_delayed_work(&mxt->glove_dwork, msecs_to_jiffies(0));
			
			break;
		default:
			break;
		}

	}
	else{
		if(touch_mode == TOUCH_GLOVE)
			schedule_delayed_work(&mxt->glove_dwork, msecs_to_jiffies(0));
			
		else if(charge_mode==TOUCH_WALL_CHARGE || charge_mode==TOUCH_USB_CHARGE || charge_mode == TOUCH_CRADLE_CHARGE){
			schedule_delayed_work(&mxt->charger_dwork, msecs_to_jiffies(0));
			
		}
		else if(charge_mode==TOUCH_WIRELESS_CHARGE){
			schedule_delayed_work(&mxt->wireless_dwork, msecs_to_jiffies(0));
			
		}
	}
	
	
	wake_unlock(&touch_wake_lock);

	return (ssize_t)count;
}



static int finger_recovery(struct mxt_data *data)
{
	mxt_write_byte(data->client, get_object_address(data,MXT_GEN_ACQUIRECONFIG_T8,0,data->object_table,data->device_info.num_objs) + MXT_ADR_T8_ATCHCALST, 0);
    mxt_write_byte(data->client, get_object_address(data,MXT_GEN_ACQUIRECONFIG_T8,0,data->object_table,data->device_info.num_objs) + MXT_ADR_T8_ATCHCALSTHR, 20);
   	schedule_delayed_work(&data->finger_dwork, msecs_to_jiffies(200));
   	return 0;  
}

static int mxt_call_recovery(struct mxt_data *data)
{
    printk("[tsp]call_recovery [%d]\n",touch_mode);

	if(touch_mode == TOUCH_GLOVE)
	{
		write_temp_config(data,46,2,28);
		write_temp_config(data,46,3,28);
	}

	write_temp_config(data,42,0,0);
	write_temp_config(data,8,6,0);
	write_temp_config(data,8,7,5);















	if(resume_flag==0)
		mxt224E_backup(data);
   	return 0;  
}

static ssize_t show_proximity_status(struct device *dev, struct device_attribute *attr, char *buf){
	int count = 0;
	
	printk("show_proximity_status start\n");

	wake_lock(&touch_wake_lock);
	count = sprintf(buf, "%d", proximity_status);
	printk(KERN_INFO "##### proximity_status = %d\n",buf[0]);
	printk("proximity_status:%d\n",proximity_status);
	wake_unlock(&touch_wake_lock);
	
	printk("show_proximity_status end\n");
	return (ssize_t)count;
}

static ssize_t store_proximity_status(struct device *dev,struct device_attribute *attr,const char *buf, size_t count){

	unsigned int work_proximity_status;
	struct mxt_data *mxt;
	mxt = dev_get_drvdata(dev);

	printk("store_proximity_status start\n");
	
	wake_lock(&touch_wake_lock);
	sscanf(buf,"%d",&work_proximity_status);
	
	printk("Old proximity_status:%d New proximity_status:%d\n",proximity_status,work_proximity_status);
	
	proximity_status = work_proximity_status;
























	if(proximity_status == 0){
		cal_check_flag = 1;
		mutex_lock(&mxt->touch_mutex);
		write_temp_config(mxt,8,8,ATCHFRCCALTHR_CALIBRATE);
		write_temp_config(mxt,8,9,ATCHFRCCALRATIO_CALIBRATE);
		mutex_unlock(&mxt->touch_mutex);
		calibrate_chip(mxt);
	}

	wake_unlock(&touch_wake_lock);
	
	printk("store_proximity_status end\n");
	return (ssize_t)count;
}

static DEVICE_ATTR(nvbackup, 0664, get_nvbackup, set_nvbackup);
static DEVICE_ATTR(object, 0664, show_object, store_object);
static DEVICE_ATTR(ap, S_IWUSR, NULL, set_ap);

static DEVICE_ATTR(update_fw, 0664, NULL, update_fw_store);
static DEVICE_ATTR(ts_version, 0664, ts_version_show, NULL);
static DEVICE_ATTR(ts_reset, 0664, show_ts_reset, store_ts_reset);

static DEVICE_ATTR(diag_normal, 0664, diag_normal_show, NULL);
static DEVICE_ATTR(diag_glove, 0664, diag_glove_show, NULL);
static DEVICE_ATTR(diag_hardkey, 0664, diag_hardkey_show, NULL);
static DEVICE_ATTR(diag_softkey, 0664, diag_softkey_show, NULL);
static DEVICE_ATTR(diag_keyled, 0664, diag_keyled_show, NULL);
static DEVICE_ATTR(diag_backlight, 0664, diag_backlight_show, NULL);
static DEVICE_ATTR(diag_white, 0664, diag_white_show, NULL);
static DEVICE_ATTR(diag_rainbow, 0664, diag_rainbow_show, NULL);
static DEVICE_ATTR(diag_frame, 0664, diag_frame_show, NULL);
static DEVICE_ATTR(diag_softbl, 0664, diag_softbl_show, NULL);
static DEVICE_ATTR(diag_normal2, 0664, diag_normal2_show, NULL);
static DEVICE_ATTR(diag_glove2, 0664, diag_glove2_show, NULL);

static DEVICE_ATTR(diag_nfcmenu, 0664, diag_nfcmenu_show, NULL);
static DEVICE_ATTR(poweroff, 0664, show_virtual_poweroff, NULL);
static DEVICE_ATTR(diag_keyemul, 0664, NULL, diag_key_emulation);
static DEVICE_ATTR(cradle_status, 0664, show_cradle_status, NULL);
static DEVICE_ATTR(wireless_status, 0664, show_wireless_status, NULL);
static DEVICE_ATTR(batt_mvolts, 0664,show_batt_mvolts, NULL); 

static DEVICE_ATTR(change_mode, 0664, show_touch_mode, store_touch_mode); 


static DEVICE_ATTR(proximity_status, 0664, show_proximity_status, store_proximity_status);


static struct attribute *maxTouch_attributes[] = {
	&dev_attr_ap.attr,
	&dev_attr_nvbackup.attr,
	&dev_attr_object.attr,
	
	&dev_attr_update_fw.attr,
	&dev_attr_ts_version.attr,
	&dev_attr_ts_reset.attr,
	&dev_attr_diag_normal.attr,
	&dev_attr_diag_glove.attr,
	&dev_attr_diag_hardkey.attr,
	&dev_attr_diag_softkey.attr,
	&dev_attr_diag_keyled.attr,
	&dev_attr_diag_backlight.attr,
	&dev_attr_diag_white.attr,
	&dev_attr_diag_rainbow.attr,
	&dev_attr_diag_frame.attr,
	&dev_attr_diag_softbl.attr,
	&dev_attr_diag_normal2.attr,
	&dev_attr_diag_glove2.attr,
	&dev_attr_diag_nfcmenu.attr,
	&dev_attr_poweroff.attr,
	&dev_attr_diag_keyemul.attr,
	&dev_attr_change_mode.attr,
	&dev_attr_cradle_status.attr, 
	&dev_attr_wireless_status.attr, 
	&dev_attr_batt_mvolts.attr,
	

	&dev_attr_proximity_status.attr,

	NULL,
};

static struct attribute_group maxtouch_attr_group = {
	.attrs = maxTouch_attributes,
};

static int mxt224E_internal_suspend(struct mxt_data *data)
{
	int i;

	cal_check_flag = 0;
	facesup_message_flag = 0;
	timer_flag = DISABLE;
	timer_ticks = 0;
	facesup_message_flag = 0;
	ts_100ms_timer_stop(data);
	
	last_charge_mode = pm_power_get_charger_mode();

	printk("[TOUCH] touch mode=%d, last touch mode=%d, last charge mode = %d\n",touch_mode, last_touch_mode, last_charge_mode);
	
	cancel_delayed_work(&data->checkchip_dwork);
	cancel_delayed_work(&data->charger_dwork);
	cancel_delayed_work(&data->wireless_dwork);
	cancel_delayed_work(&data->glove_dwork);
	cancel_delayed_work(&data->normal_dwork);
	cancel_delayed_work(&data->resume_dwork);
	cancel_delayed_work(&data->charging_dwork);
	cancel_delayed_work(&data->call_recovery_dwork);
	cancel_delayed_work(&data->mxt_reset_dwork);
	cancel_delayed_work(&data->calibrate_timer_dwork);
	cancel_delayed_work(&data->suppress_timer_dwork);
	
	calibration_complete_count=0; 
    
	resume_change = 0;
	calibrate_timer_stop(data);

	mxt_call_recovery(data);
	mdelay(30);
	
	if(!touch_firm_up_status)
	{	
		for (i = 0; i < 5; i++) {
			if (data->fingers[i].w == -1)
				continue;
			data->fingers[i].w = 0;
		}
		report_input_data(data);

		mxt_release_all_keys(data);
		mxt_release_fingers(data);

		
	
		data->power_off();
	}
	

	return 0;
}

static int mxt224E_internal_resume(struct mxt_data *data)
{
	int ret=0;
	printk("[touch] %s start\n",__func__);
	
	
	enable_irq(data->client->irq);





	calibrate_chip(data);

	schedule_delayed_work(&data->resume_dwork, msecs_to_jiffies(200));
	schedule_delayed_work(&data->charging_dwork, msecs_to_jiffies(2000));
	
	return ret;
}


#define mxt224E_suspend NULL
#define mxt224E_resume	NULL

static void mxt224E_early_suspend(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,early_suspend);
	mutex_lock(&data->touch_mutex); 
	wake_lock(&touch_wake_lock);

	disable_irq(data->client->irq);
	msleep(50);
	mxt224E_internal_suspend(data);
	input_mt_sync(data->input_dev);
	input_sync(data->input_dev);

	mxt_suspend_status = 1;
	mxt_swreset_status = 1;
	mxt_swreset_cancel = 1;
	
	mutex_unlock(&data->touch_mutex); 
	wake_unlock(&touch_wake_lock);
}

static void mxt224E_late_resume(struct early_suspend *h)
{
	struct mxt_data *data = container_of(h, struct mxt_data,early_suspend);
	mutex_lock(&data->touch_mutex); 
	wake_lock(&touch_wake_lock);
	mxt224E_internal_resume(data);
	mutex_unlock(&data->touch_mutex); 
	wake_unlock(&touch_wake_lock);
	mxt_suspend_status = 0;
}


















static int __devinit mxt224E_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct mxt224E_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	int ret;
	int i;
	int error;
	int power_on_cnt = MXT_I2C_POWER_ON_CNT;		
	int rc;	







	if (!pdata) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	if (pdata->max_finger_touches <= 0)
		return -EINVAL;

	data = kzalloc(sizeof(*data) + pdata->max_finger_touches *
					sizeof(*data->fingers), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	mutex_init(&data->touch_mutex);

	data->read_fail_counter = 0;
	data->message_counter   = 0;
	
	data->num_fingers = pdata->max_finger_touches;
	data->power_on = pdata->power_on;
	data->power_off = pdata->power_off;
	data->client = client;
	i2c_set_clientdata(client, data);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		goto err_alloc_dev;
	}
	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "mxt224E_ts_input";

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_2, input_dev->keybit);
	
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->min_x,pdata->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->min_y,pdata->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, pdata->min_z,pdata->max_z, 0, 0);
	
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, pdata->min_w,pdata->max_w, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 4, 0, 0);

	
	input_set_capability(input_dev, EV_KEY, KEY_BACK);
	input_set_capability(input_dev, EV_KEY, KEY_HOME);
	input_set_capability(input_dev, EV_KEY, KEY_MENU);
	
	input_set_capability(input_dev, EV_KEY, KEY_APP_SWITCH);
	

	input_set_capability(input_dev, EV_KEY, KEY_DIAG_HARDKEY);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_SOFTKEY);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_NORMAL);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_GLOVE);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_KEYLED);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_BACKLIGHT);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_WHITE);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_RAINBOW);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_FRAME);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_SOFTBL);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_NORMAL2);
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_GLOVE2);
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	
	input_set_capability(input_dev, EV_KEY, KEY_DIAG_NFCMENU);
	
	
	input_set_capability(input_dev, EV_KEY, KEY_1);
	input_set_capability(input_dev, EV_KEY, KEY_2);
	input_set_capability(input_dev, EV_KEY, KEY_3);
	input_set_capability(input_dev, EV_KEY, KEY_4);
	input_set_capability(input_dev, EV_KEY, KEY_5);
	input_set_capability(input_dev, EV_KEY, KEY_6);
	input_set_capability(input_dev, EV_KEY, KEY_7);
	input_set_capability(input_dev, EV_KEY, KEY_8);
	input_set_capability(input_dev, EV_KEY, KEY_9);
	input_set_capability(input_dev, EV_KEY, KEY_0);
	input_set_capability(input_dev, EV_KEY, KEY_KPASTERISK);
	input_set_capability(input_dev, EV_KEY, KEY_PHONE);
	input_set_capability(input_dev, EV_KEY, KEY_BACKSPACE);
    input_set_capability(input_dev, EV_KEY, KEY_UP);
    input_set_capability(input_dev, EV_KEY, KEY_LEFT);
    input_set_capability(input_dev, EV_KEY, KEY_RIGHT);
    input_set_capability(input_dev, EV_KEY, KEY_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input_dev, EV_KEY, KEY_VOLUMEUP);
    input_set_capability(input_dev, EV_KEY, KEY_TACTILE);
	input_set_capability(input_dev, EV_KEY, KEY_NUMERIC_STAR);
	input_set_capability(input_dev, EV_KEY, KEY_NUMERIC_POUND);
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_ENTER);
	input_set_capability(input_dev, EV_KEY, KEY_SEARCH);    
	input_set_capability(input_dev, EV_KEY, KEY_EXIT);    
	

	ret = input_register_device(input_dev);
	if (ret) {
		input_free_device(input_dev);
		goto err_reg_dev;
	}

	data->gpio_read_done = pdata->gpio_read_done;

	touch_io = regulator_get(NULL,"8921_l29");
	
	if (IS_ERR(touch_io)) {
		pr_err("could not get 8921_l29, rc = %ld\n",
			PTR_ERR(touch_io));
		return -ENODEV;
	}

	rc = regulator_set_voltage(touch_io, 1800000, 1800000);
	if (rc) {
		pr_err("set_voltage 8921_l29 failed, rc=%d\n", rc);
		return -EINVAL;
	}
		
	mxt_power_on(true);
	msleep(10);
	data->power_on();

	msleep(300);
	
	
























































	while(power_on_cnt)
	{

		ret = mxt224E_init_touch_driver(data);
		if (ret) {
			pr_err("[mxt224E] : chip initialization failed\n");
			goto power_on_err;
		}

		pdata->config = mxt224E_config;


		for (i = 0; pdata->config[i][0] != RESERVED_T255; i++) {











			if (pdata->config[i][0] == GEN_POWERCONFIG_T7)
				data->power_cfg = pdata->config[i] + 1;

			if (pdata->config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
				
				if (pdata->config[i][10] & 0x1) {
					data->x_dropbits =
						(!(pdata->config[i][22] & 0xC)) << 1;
					data->y_dropbits =
						(!(pdata->config[i][20] & 0xC)) << 1;
				} else {
					data->x_dropbits =
						(!(pdata->config[i][20] & 0xC)) << 1;
					data->y_dropbits =
						(!(pdata->config[i][22] & 0xC)) << 1;
				}
			}
		}

		ret = mxt224E_backup(data);
		if (ret)
		{
			pr_err("[mxt224E] : mxt224E_backup failed\n");
			goto power_on_err;
		}

		mdelay(25);		

		
		ret = mxt224E_reset(data);
		if (ret)
		{
			pr_err("[mxt224E] : mxt224E_reset failed\n");
			goto power_on_err;
		}

		break;	
power_on_err:
		if( data->objects )
			kfree(data->objects);
		data->objects = NULL;




		
		gpio_set_value(94,0);
		gpio_set_value(18,1);
		mdelay(1);
		gpio_set_value(94,1);

		power_on_cnt--;
	}

	if( power_on_cnt == 0 )
		goto err_power;


	msleep(60);

	for (i = 0; i < 5; i++)
		data->fingers[i].z = -1;

	ret = request_threaded_irq(client->irq, NULL, mxt224E_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt224E_ts", data);
	if (ret < 0)
		goto err_irq;

	ret = sysfs_create_group(&client->dev.kobj, &maxtouch_attr_group);
	if (ret) {
		printk("sysfs_create_group error\n");
		goto  err_irq;
	}
	

	
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING - 1;
	data->early_suspend.suspend = mxt224E_early_suspend;
	data->early_suspend.resume = mxt224E_late_resume;
	register_early_suspend(&data->early_suspend);


	INIT_WORK(&data->ta_work, mxt_ta_worker);
	INIT_DELAYED_WORK(&data->config_dwork, mxt_palm_recovery);
	INIT_DELAYED_WORK(&data->timer_dwork, mxt_metal_suppression_off); 
	INIT_DELAYED_WORK(&data->checkchip_dwork, mxt_checkchip_cal_delayed_work); 
  	
  	INIT_DELAYED_WORK(&data->finger_dwork, mxt_finger_recovery_delayed_work);
  	
	INIT_DELAYED_WORK(&data->normal_dwork, mxt_normal_config_delayed_work);
	INIT_DELAYED_WORK(&data->charger_dwork, mxt_charger_config_delayed_work);
	INIT_DELAYED_WORK(&data->wireless_dwork, mxt_wireless_config_delayed_work);
	INIT_DELAYED_WORK(&data->glove_dwork, mxt_glove_config_delayed_work);
	INIT_DELAYED_WORK(&data->resume_dwork, mxt_resume_delayed_work);
	INIT_DELAYED_WORK(&data->charging_dwork, mxt_charging_delayed_work);
	INIT_DELAYED_WORK(&data->call_recovery_dwork, mxt_call_recovery_delayed_work);
	INIT_DELAYED_WORK(&data->mxt_reset_dwork, mxt_reset_delayed_work);
	INIT_DELAYED_WORK(&data->calibrate_timer_dwork, calibrate_timer_delayed_work);	
	INIT_DELAYED_WORK(&data->suppress_timer_dwork, suppress_timer_delayed_work);	
	









	INIT_WORK(&data->tmr_work, ts_100ms_tmr_work);
	init_waitqueue_head(&data->msg_queue);
	
	spin_lock_init(&data->lock);

	mxt224E_data = data;
	
	for (i = 0; i < MXT_MAX_NUM_TOUCHES ; i++)	
		data->fingers[i].w = -1;
	
	ts_100s_tmr_workqueue = create_singlethread_workqueue("ts_100_tmr_workqueue");
	if (!ts_100s_tmr_workqueue)
	{
		printk(KERN_ERR "unabled to create touch tmr work queue \r\n");
		error = -1;
		goto err_alloc_dev;
	}
	ts_100ms_timer_init(data);

	cal_check_flag = 1;
	check_chip_calibration(data);

	disable_irq(data->client->irq);

	msleep(500);
















	schedule_delayed_work(&data->checkchip_dwork, msecs_to_jiffies(20000));
	schedule_delayed_work(&data->charging_dwork, msecs_to_jiffies(25000));
	
	
	
	
	wake_lock_init(&touch_wake_lock, WAKE_LOCK_SUSPEND, "mxt22E-lock");
	normal_cfg_max = MIN(CONFIG_MAX, sizeof(normal_booting_config) / 3);
	glove_cfg_max = MIN(CONFIG_MAX, sizeof(glove_temp_config) / 3);
	charger_cfg_max = MIN(CONFIG_MAX, sizeof(charger_temp_config) / 3);
	wireless_cfg_max = MIN(CONFIG_MAX, sizeof(wireless_charger_config) / 3);
	memcpy(normal_cfg_config,normal_booting_config,normal_cfg_max * 3);
	memcpy(glove_cfg_config,glove_temp_config,glove_cfg_max * 3);
	memcpy(charger_cfg_config,charger_temp_config,charger_cfg_max * 3);
	memcpy(wireless_cfg_config,wireless_charger_config,wireless_cfg_max * 3);
	normal_cfg_count = 0;
	glove_cfg_count = 0;
	charger_cfg_count = 0;
	wireless_cfg_count = 0;
	
	
	return 0;

err_irq:










err_power:
	if( data->objects )
		kfree(data->objects);
	data->objects = NULL;

	input_unregister_device(input_dev);
err_reg_dev:
err_alloc_dev:
	kfree(data);
	return ret;
}

static int __devexit mxt224E_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &maxtouch_attr_group);

	unregister_early_suspend(&data->early_suspend);

	free_irq(client->irq, data);
	kfree(data->objects);

	input_unregister_device(data->input_dev);
	kfree(data);

	return 0;
}

static struct i2c_device_id mxt224E_idtable[] = {
	{MXT224E_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mxt224E_idtable);

static const struct dev_pm_ops mxt224E_pm_ops = {
	.suspend = mxt224E_suspend,
	.resume = mxt224E_resume,
};

static struct i2c_driver mxt224E_i2c_driver = {
	.id_table = mxt224E_idtable,
	.probe = mxt224E_probe,
	.remove = __devexit_p(mxt224E_remove),
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MXT224E_DEV_NAME,

		

	},
};

static int __init mxt224E_init(void)
{
	return i2c_add_driver(&mxt224E_i2c_driver);
}

static void __exit mxt224E_exit(void)
{
	i2c_del_driver(&mxt224E_i2c_driver);
}

module_init(mxt224E_init);
module_exit(mxt224E_exit);

MODULE_DESCRIPTION("Atmel MaXTouch 224E driver");
MODULE_AUTHOR("M7-BL-Jinkue Bang");
MODULE_LICENSE("GPL");

