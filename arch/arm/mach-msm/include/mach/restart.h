/* Copyright (c) 2011, The Linux Foundation. All rights reserved.
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

#ifndef _ASM_ARCH_MSM_RESTART_H_
#define _ASM_ARCH_MSM_RESTART_H_

#define RESTART_NORMAL 0x0
#define RESTART_DLOAD  0x1


#define PANIC_MAGIC_NUM        0x2A03F014
#define PANIC_MAGIC_ADDR       0x18 

#define KERNEL_PANIC_LOG_ADDR	0x88a3e000

#define PANIC_REASON_MAGIC_NUM 0x29A90000


#define PANIC_REASON_MODEM     0x1000 
#define PANIC_REASON_DSPS	   0x2000 
#define PANIC_REASON_LPSS	   0x3000 
#define PANIC_REASON_RPM	   0x4000 
#define PANIC_REASON_WCNSS	   0x5000 
#define PANIC_REASON_KERNEL    0x6000 
#define PANIC_REASON_HW_RESET  0xAA00 


#define ERR_UNABLE_RESTART_THREAD		0x0001
#define ERR_FAILED_DURING_POWER_UP		0x0002
#define ERR_FAILED_DURING_POWER_DOWN	0x0003
#define ERR_RESET_SOC					0x0004
#define ERR_UNKNOWN						0x0005



#if defined(CONFIG_MSM_NATIVE_RESTART)
void msm_set_restart_mode(int mode);
void msm_restart(char mode, const char *cmd);
#elif defined(CONFIG_ARCH_FSM9XXX)
void fsm_restart(char mode, const char *cmd);
#else
#define msm_set_restart_mode(mode)
#endif

extern int pmic_reset_irq;

#endif

