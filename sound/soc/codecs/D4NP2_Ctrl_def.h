/****************************************************************************
 *
 *		CONFIDENTIAL
 *
 *		Copyright (c) 2009 Yamaha Corporation
 *
 *		Module		: D4NP2_Ctrl_def.h
 *
 *		Description	: D-4NP2 control define
 *
 *		Version		: 1.0.2 	2009.01.16
 *
 ****************************************************************************/
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/
#if !defined(_D4NP2_CTRL_DEF_H_)
#define	_D4NP2_CTRL_DEF_H_











#define PDPC		0x000100
#define PDP			0x000201


#define PD_CHP		0x020100
#define PD_REG		0x020402
#define HIZ_HP		0x020803


#define HIZ_SPR		0x030402
#define HIZ_SPL		0x030803
#define PD_REC		0x031004
#define PD_SNT		0x038007


#define DPLT		0x047004
#define DALC		0x040700


#define DREL		0x050C02
#define DATT		0x050300


#define MNX			0x071F00
#define ZCS_MV		0x078007


#define SVLA		0x081F00
#define LAT_VA		0x082005
#define ZCS_SVA		0x088007


#define SVRA		0x091F00


#define SVLB		0x0A1F00
#define LAT_VB		0x0A2005
#define ZCS_SVB		0x0A8007


#define SVRB		0x0B1F00


#define HPL_BMIX	0x0C0100
#define HPL_AMIX	0x0C0201
#define HPL_MMIX	0x0C0402
#define MONO_HP		0x0C0803
#define HPR_BMIX	0x0C1004
#define HPR_AMIX	0x0C2005
#define HPR_MMIX	0x0C4006


#define SPL_BMIX	0x0D0100
#define SPL_AMIX	0x0D0201
#define SPL_MMIX	0x0D0402
#define MONO_SP		0x0D0803
#define SPR_BMIX	0x0D1004
#define SPR_AMIX	0x0D2005
#define SPR_MMIX	0x0D4006
#define SWAP_SP		0x0D8007


#define MNA			0x0E1F00
#define SVOL_SP		0x0E4006
#define ZCS_SPA		0x0E8007


#define SALA		0x0F1F00
#define LAT_HP		0x0F2005
#define SVOL_HP		0x0F4006
#define ZCS_HPA		0x0F8007


#define SARA		0x101F00


#define OTP_ERR		0x118006
#define OCP_ERR		0x118007



#define D4NP2_SUCCESS			0
#define D4NP2_ERROR				-1


#define MAX_WRITE_REGISTER_NUMBER		19
#define MAX_READ_REGISTER_NUMBER		20

#define D4NP2_SOFTVOLUME_STEPTIME		250		 


#define D4NP2_STATE_POWER_ON	0
#define D4NP2_STATE_POWER_OFF	1


#define SINT32 signed long
#define UINT32 unsigned long
#define SINT8 signed char
#define UINT8 unsigned char

#endif	
