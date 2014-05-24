/*
 * linux/sound/wm2000.h -- Platform data for WM2000
 *
 * Copyright 2010 Wolfson Microelectronics. PLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/

#if !defined(__LINUX_SND_YDA160_H)
#define __LINUX_SND_YDA160_H

struct yda160_platform_data {
    
	int reset_gpio;
	int spk_pwr_en_gpio;
	int rcv_switch_gpio;

	int (*power_on)(int on);
	int (*dev_setup)(bool on);	
};


#endif
