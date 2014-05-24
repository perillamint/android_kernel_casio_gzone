/****************************************************************************
 *
 *		CONFIDENTIAL
 *
 *		Copyright (c) 2009 Yamaha Corporation
 *
 *		Module		: D4NP2_Ctrl.h
 *
 *		Description	: D-4NP2 control module define
 *
 *		Version		: 1.0.2 	2009.01.16
 *
 ****************************************************************************/
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/
#if !defined(_D4NP2_CTRL_H_)
#define	_D4NP2_CTRL_H_




typedef struct {
	unsigned char bInVolMode;		
	unsigned char bMinVol;			
	unsigned char bLine1Vol;		
	unsigned char bLine2Vol;		

	unsigned char bHpCh;			
	unsigned char bHpVolMode;		
	unsigned char bHpMixer_Min;		
	unsigned char bHpMixer_Line1;	
	unsigned char bHpMixer_Line2;	
	unsigned char bHpVol;			

	unsigned char bSpCh;			
	unsigned char bSpSwap;			
	unsigned char bSpVolMode;		
	unsigned char bSpMixer_Min;		
	unsigned char bSpMixer_Line1;	
	unsigned char bSpMixer_Line2;	
	unsigned char bSpVol;			

	unsigned char bSpNonClip;		
	unsigned char bPowerLimit;		
	unsigned char bDistortion;		
	unsigned char bReleaseTime;		
	unsigned char bAttackTime;		

	unsigned char bRecSW;			
} D4NP2_SETTING_INFO;




void D4Np2_PowerOn(D4NP2_SETTING_INFO *pstSettingInfo);	
void D4Np2_PowerOff(void);								
void D4Np2_WriteRegisterBit( unsigned long bName, unsigned char bPrm );		
void D4Np2_WriteRegisterByte( unsigned char bNumber, unsigned char bPrm );	
void D4Np2_WriteRegisterByteN( unsigned char bNumber, unsigned char *pbPrm, unsigned char bPrmSize );	
void D4Np2_ReadRegisterBit( unsigned long bName, unsigned char *pbPrm);		
void D4Np2_ReadRegisterByte( unsigned char bNumber, unsigned char *pbPrm);	


void D4Np2_PowerOn_SpOut_LRch(D4NP2_SETTING_INFO *pstSetInfo, int bOnLch, int bOnRch); 

#endif	
