/****************************************************************************
 *
 *		CONFIDENTIAL
 *
 *		Copyright (c) 2009 Yamaha Corporation
 *
 *		Module		: d4np2machdep.h
 *
 *		Description	: machine dependent define for D-4NP2
 *
 *		Version		: 1.0.2 	2009.01.16
 *
 ****************************************************************************/
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/
#if !defined(_D4NP2MACHDEP_H_)
#define _D4NP2MACHDEP_H_



			
			
			


#define D4NP2_VREFCHARGETIME		6	
#define D4NP2_I2CCTRLTIME			23	
#define D4NP2_CHARGEPUMPWAKEUPTIME	500	
#define D4NP2_HPIDLINGTIME			8	




signed long d4np2Write(unsigned char bWriteRN, unsigned char bWritePrm);								
signed long d4np2WriteN(unsigned char bWriteRN, unsigned char *pbWritePrm, unsigned char dWriteSize);	
signed long d4np2Read(unsigned char bReadRN, unsigned char *pbReadPrm);		
void d4np2Wait(unsigned long dTime);			
void d4np2Sleep(unsigned long dTime);			
void d4np2ErrHandler(int dError);		

#endif	
