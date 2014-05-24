/**********************************************************************
* File Name: arch/arm/mach-msm/include/mach/board_gg3.h
* 
* (C) NEC CASIO Mobile Communications, Ltd. 2013
**********************************************************************/
#if !defined(__BOARD_DVE068_H)
#define __BOARD_DVE068_H


#define ANROID_RAM_CONSOLE_SIZE (124 * SZ_1K * 2)



#define CRASH_LOG_SIZE (4*SZ_1K)



enum {
	M7SYSTEM_REV_NULL = 0,
	M7SYSTEM_REV_V1A,	
	M7SYSTEM_REV_V2A,	
	M7SYSTEM_REV_V3A,	
	M7SYSTEM_REV_V4A,	
	M7SYSTEM_REV_V5A,
	M7SYSTEM_REV_V6A,
	M7SYSTEM_REV_V7A,
	M7SYSTEM_REV_V8A,
	M7SYSTEM_REV_V9A,
	M7SYSTEM_REV_V10A,
	M7SYSTEM_REV_MAX,
};

int get_m7system_board_revision(void);


void __init add_ramconsole_devices(void);



void __init add_fatal_info_handler_devices(void);
int  m7_get_magic_for_subsystem(void);
void m7_set_magic_for_subsystem(const char* subsys_name);

#endif


