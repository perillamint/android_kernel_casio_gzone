/****************************************************************************
 *
 *		CONFIDENTIAL
 *
 *		Copyright (c) 2009 Yamaha Corporation
 *
 *		Module		: D4NP2_Ctrl.c
 *
 *		Description	: D-4NP2 control module
 *
 *		Version		: 1.0.4 	2009.11.26
 *
 ****************************************************************************/
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/
#include <linux/interrupt.h> 

#include "D4NP2_Ctrl.h"
#include "D4NP2_Ctrl_def.h"
#include "d4np2machdep.h"


UINT8 g_bD4Np2RegisterMap[21] = 
	{	0x03,								
		0x07, 0x07, 0x93, 0x00, 0x0D,		
		0x00, 0x80, 0x80, 0x00, 0x80,		
		0x00, 0x00, 0x00, 0x40, 0x40,		
		0x00, 0x00, 0x00, 0x00, 0x00 };		

UINT8 g_bFlag_WaitCharge = 1;

UINT8 g_bSpState = D4NP2_STATE_POWER_OFF;
UINT8 g_bHpState = D4NP2_STATE_POWER_OFF;

#define DELAY_ON_TIME_MSEC 20 















static void D4Np2_UpdateRegisterMap( SINT32 sdRetVal, UINT8 bRN, UINT8 bPrm )
{


	if(sdRetVal < 0)
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	else
	{
		
		g_bD4Np2RegisterMap[ bRN ] = bPrm;
	}
}
















static void D4Np2_UpdateRegisterMapN( SINT32 sdRetVal, UINT8 bRN, UINT8 *pbPrm, UINT8 bPrmSize )
{
	UINT8 bCnt = 0;

	if(sdRetVal < 0)
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	else
	{
		
		for(bCnt = 0; bCnt < bPrmSize; bCnt++)
		{
			g_bD4Np2RegisterMap[ bRN + bCnt ] = pbPrm[ bCnt ];
		}
	}
}














void D4Np2_WriteRegisterBit( UINT32 dName, UINT8 bPrm )
{
	UINT8 bWritePrm;		
	UINT8 bDummy;			
	UINT8 bRN, bMB, bSB;	

	






	bRN = (UINT8)(( dName & 0xFF0000 ) >> 16);
	bMB = (UINT8)(( dName & 0x00FF00 ) >> 8);
	bSB = (UINT8)( dName & 0x0000FF );

	
	if( bRN > MAX_WRITE_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	
	bPrm = (bPrm << bSB) & bMB;
	bDummy = bMB ^ 0xFF;
	bWritePrm = g_bD4Np2RegisterMap[ bRN ] & bDummy;	
	bWritePrm = bWritePrm | bPrm;						
	
	D4Np2_UpdateRegisterMap( d4np2Write( bRN, bWritePrm ), bRN, bWritePrm );
}















void D4Np2_WriteRegisterByte(UINT8 bNumber, UINT8 bPrm)
{
	
	if( bNumber > MAX_WRITE_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	D4Np2_UpdateRegisterMap( d4np2Write( bNumber, bPrm ), bNumber, bPrm );
}
















void D4Np2_WriteRegisterByteN(UINT8 bNumber, UINT8 *pbPrm, UINT8 bPrmSize)
{
	
	if( bNumber > MAX_WRITE_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	D4Np2_UpdateRegisterMapN( d4np2WriteN( bNumber, pbPrm, bPrmSize ), bNumber, pbPrm, bPrmSize);
}














void D4Np2_ReadRegisterBit( UINT32 dName, UINT8 *pbPrm)
{
	SINT32 sdRetVal = D4NP2_SUCCESS;
	UINT8 bRN, bMB, bSB;	

	






	bRN = (UINT8)(( dName & 0xFF0000 ) >> 16);
	bMB = (UINT8)(( dName & 0x00FF00 ) >> 8);
	bSB = (UINT8)( dName & 0x0000FF );

	
	if(bRN > MAX_READ_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	
	sdRetVal = d4np2Read( bRN, pbPrm );
	D4Np2_UpdateRegisterMap( sdRetVal, bRN, *pbPrm );
	
	*pbPrm = ((g_bD4Np2RegisterMap[ bRN ] & bMB) >> bSB);
}














void D4Np2_ReadRegisterByte( UINT8 bNumber, UINT8 *pbPrm)
{
	SINT32 sdRetVal = D4NP2_SUCCESS;

	
	if(bNumber > MAX_READ_REGISTER_NUMBER )
	{
		d4np2ErrHandler( D4NP2_ERROR );
	}
	
	sdRetVal = d4np2Read( bNumber, pbPrm );
	D4Np2_UpdateRegisterMap( sdRetVal, bNumber, *pbPrm );
}














static void D4Np2_SoftVolume_Wait( UINT8 bStartVol, UINT8 bEndVol )
{
	UINT32 dIdleTime;

	
	if(bStartVol < bEndVol)
	{
		dIdleTime = (UINT32)((bEndVol - bStartVol) * D4NP2_SOFTVOLUME_STEPTIME);
		d4np2Wait( dIdleTime );
	}
	
	else if(bStartVol > bEndVol)
	{
		dIdleTime = (UINT32)((bStartVol - bEndVol) * D4NP2_SOFTVOLUME_STEPTIME);
		d4np2Wait( dIdleTime );
	}
}













static void D4Np2_PowerOff_ReceiverOut(void)
{
	
	D4Np2_WriteRegisterBit( PD_SNT, 0 ); 
	D4Np2_WriteRegisterBit( PD_REC, 1 );
	D4Np2_WriteRegisterBit( PD_SNT, 1 ); 
	



	D4Np2_WriteRegisterBit( PD_SNT, 1 );

}













static void D4Np2_PowerOff_SpOut(void)
{
	UINT8 bWriteRN, bWritePrm;
	UINT8 bSvol, bTempVol;

	
	
	bSvol = (g_bD4Np2RegisterMap[14] & 0x40) >> (SVOL_SP & 0xFF);
	bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
	D4Np2_WriteRegisterBit( MNA, 0 );
	
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
	}
	
	D4Np2_WriteRegisterBit( HIZ_SPR, 0 ); 
	D4Np2_WriteRegisterBit( HIZ_SPL, 0 ); 
	bWriteRN = 13;
	bWritePrm = g_bD4Np2RegisterMap[13] & 0x80;
	D4Np2_WriteRegisterByte( bWriteRN, bWritePrm );
	D4Np2_WriteRegisterBit( HIZ_SPR, 1 ); 
	D4Np2_WriteRegisterBit( HIZ_SPL, 1 ); 
	
	g_bSpState = D4NP2_STATE_POWER_OFF;
}













static void D4Np2_PowerOff_HpOut(void)
{
	UINT8 bWriteRN, abWritePrm[2];
	UINT8 bSvol, bTempVol;

	
	
	bSvol = (g_bD4Np2RegisterMap[15] & 0x40) >> (SVOL_HP & 0xFF);
	bTempVol = g_bD4Np2RegisterMap[15] & 0x1F;
	abWritePrm[0] = g_bD4Np2RegisterMap[15] & 0xE0;
	abWritePrm[1] = g_bD4Np2RegisterMap[16] & 0xE0;
	bWriteRN = 15;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
	}
	
	D4Np2_WriteRegisterBit( HIZ_HP, 0 ); 
	bWriteRN = 12;
	abWritePrm[0] = g_bD4Np2RegisterMap[12] & 0x00;
	D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
	
	D4Np2_WriteRegisterBit( PD_CHP, 1 );
	
	D4Np2_WriteRegisterBit( PD_REG, 1 );
	
	D4Np2_WriteRegisterBit( HIZ_HP, 1 ); 
	g_bHpState = D4NP2_STATE_POWER_OFF;
}













static void D4Np2_PowerOff_OutputPart(void)
{
	
	if( ((g_bD4Np2RegisterMap[3] >> (PD_REC & 0xFF)) & 0x01) == 0x00 )
	{
		D4Np2_PowerOff_ReceiverOut();
	}
	
	if( g_bSpState == D4NP2_STATE_POWER_ON )
	{
		D4Np2_PowerOff_SpOut();
	}

	
	if( g_bHpState == D4NP2_STATE_POWER_ON )
	{
		D4Np2_PowerOff_HpOut();
	}
}













static void D4Np2_PowerOff_InputPart(void)
{
	UINT8 bWriteRN, abWritePrm[5];

	
	abWritePrm[0] = g_bD4Np2RegisterMap[7] & 0xE0;
	abWritePrm[1] = g_bD4Np2RegisterMap[8] & 0xE0;
	abWritePrm[2] = g_bD4Np2RegisterMap[9] & 0xE0;
	abWritePrm[3] = g_bD4Np2RegisterMap[10] & 0xE0;
	abWritePrm[4] = g_bD4Np2RegisterMap[11] & 0xE0;
	bWriteRN = 7;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 5 );
}













static void D4Np2_PowerOff_CommonPart(void)
{
	
	if( ((g_bD4Np2RegisterMap[0] & 0x02) >> (PDP & 0xFF)) == 0 )
	{
		D4Np2_WriteRegisterBit( PDP, 1 );
	}
	
	D4Np2_WriteRegisterBit( PDPC, 1 );
}













static void D4Np2_PowerOn_CommonPart(void)
{
	
	if( (g_bD4Np2RegisterMap[0] & 0x01) == 1 )
	{
		D4Np2_WriteRegisterBit( PDPC, 0 );
	}
	
	if( ((g_bD4Np2RegisterMap[0] & 0x02) >> (PDP & 0xFF)) == 1 )
	{
		D4Np2_WriteRegisterBit( PDP, 0 );
	}
}













static void D4Np2_PowerOn_InputPart(D4NP2_SETTING_INFO *pstSetInfo)
{
	UINT8 bWriteRN, abWritePrm[5];
	UINT8 bZcs;
	UINT8 bPowerOff_Flag;

	
	bPowerOff_Flag = 0;
	
	if( (pstSetInfo->bMinVol == 0) || ((g_bD4Np2RegisterMap[7] & 0x1F) == 0) )
	{
		if(pstSetInfo->bMinVol == (g_bD4Np2RegisterMap[7] & 0x1F))
		{
			pstSetInfo->bHpMixer_Min = 0;
			pstSetInfo->bSpMixer_Min = 0;
		}
		else if(pstSetInfo->bMinVol < (g_bD4Np2RegisterMap[7] & 0x1F))
		{
			bPowerOff_Flag = 1;
			pstSetInfo->bHpMixer_Min = 0;
			pstSetInfo->bSpMixer_Min = 0;
		}
		else
		{
			bPowerOff_Flag=1;
		}
	}
	
	if( (pstSetInfo->bLine1Vol == 0) || ((g_bD4Np2RegisterMap[8] & 0x1F) == 0) )
	{
		if(pstSetInfo->bLine1Vol == (g_bD4Np2RegisterMap[8] & 0x1F))
		{
			pstSetInfo->bHpMixer_Line1 = 0;
			pstSetInfo->bSpMixer_Line1 = 0;
		}
		else if(pstSetInfo->bLine1Vol < (g_bD4Np2RegisterMap[8] & 0x1F))
		{
			bPowerOff_Flag = 1;
			pstSetInfo->bHpMixer_Line1 = 0;
			pstSetInfo->bSpMixer_Line1 = 0;
		}
		else
		{
			bPowerOff_Flag=1;
		}
	}
	
	if( (pstSetInfo->bLine2Vol == 0) || ((g_bD4Np2RegisterMap[10] & 0x1F) == 0) )
	{
		if(pstSetInfo->bLine2Vol == (g_bD4Np2RegisterMap[10] & 0x1F))
		{
			pstSetInfo->bHpMixer_Line2 = 0;
			pstSetInfo->bSpMixer_Line2 = 0;
		}
		else if(pstSetInfo->bLine2Vol < (g_bD4Np2RegisterMap[10] & 0x1F))
		{
			bPowerOff_Flag = 1;
			pstSetInfo->bHpMixer_Line2 = 0;
			pstSetInfo->bSpMixer_Line2 = 0;
		}
		else
		{
			bPowerOff_Flag=1;
		}
	}
	
	if(bPowerOff_Flag == 1)
	{
		D4Np2_PowerOff_OutputPart();
	}

	
	if( pstSetInfo->bInVolMode == 1 )
	{
		bZcs = 1;
	}
	else
	{
		bZcs = 0;
	}
	abWritePrm[0] = ( bZcs << (ZCS_MV & 0xFF)) | ((pstSetInfo->bMinVol & 0x1F) << (MNX & 0xFF));
	abWritePrm[1] = ( bZcs << (ZCS_SVA & 0xFF))	| (0x01 << (LAT_VA & 0xFF))
					| ((pstSetInfo->bLine1Vol & 0x1F) << (SVLA & 0xFF));
	abWritePrm[2] = ((pstSetInfo->bLine1Vol & 0x1F) << (SVRA & 0xFF));
	abWritePrm[3] = (bZcs << (ZCS_SVB & 0xFF))	| (0x01 << (LAT_VB & 0xFF))
					| ((pstSetInfo->bLine2Vol & 0x1F) << (SVLB & 0xFF));
	abWritePrm[4] = ((pstSetInfo->bLine2Vol & 0x1F) << (SVLB & 0xFF));
	bWriteRN = 7;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 5 );
	
	d4np2Sleep( D4NP2_VREFCHARGETIME );
}













static void D4Np2_PowerOn_ReceiverOut(D4NP2_SETTING_INFO *pstSetInfo)
{
	
	D4Np2_WriteRegisterBit( PD_SNT, 0 ); 
	
	D4Np2_WriteRegisterBit( PD_REC, pstSetInfo->bRecSW & 0x01 );
	D4Np2_WriteRegisterBit( PD_SNT, 1 ); 
}













static void D4Np2_PowerOn_SpOut(D4NP2_SETTING_INFO *pstSetInfo)
{
	UINT8 bWriteRN, abWritePrm[3];
	UINT8 bZcs, bSvol, bTempVol, bTempMixer;
	unsigned long endtime; 
	
	D4Np2_WriteRegisterBit( HIZ_SPR, 0 ); 
	D4Np2_WriteRegisterBit( HIZ_SPL, 0 ); 
	




	abWritePrm[0] = (g_bD4Np2RegisterMap[3] & 0xF3)
					| (0x00 << (HIZ_SPL & 0xFF)) | (0x00 << (HIZ_SPR & 0xFF));

	
    
	if( pstSetInfo->bSpNonClip == 1)
	{
		abWritePrm[1] = ((pstSetInfo->bDistortion & 0x07) << (DALC & 0xFF));
	}
	else
	{
		abWritePrm[1] = ((pstSetInfo->bPowerLimit & 0x07) << (DPLT & 0xFF));
	}
	
	abWritePrm[2] = ((pstSetInfo->bReleaseTime & 0x03)<< (DREL & 0xFF))
					| ((pstSetInfo->bAttackTime & 0x03) << (DATT & 0xFF));
	bWriteRN = 3;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 3 );

	
	if(pstSetInfo->bSpVolMode == 2)
	{
		bZcs = 0;
		bSvol = 1;
	}
	else if(pstSetInfo->bSpVolMode == 1)
	{
		bZcs = 1;
		bSvol = 0;
	}
	else
	{
		bZcs = 0;
		bSvol = 0;
	}
	bTempMixer = ((pstSetInfo->bSpSwap & 0x01) << (SWAP_SP & 0xFF))
					| ((pstSetInfo->bSpMixer_Min & 0x01) << (SPR_MMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line1 & 0x01) << (SPR_AMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line2 & 0x01) << (SPR_BMIX & 0xFF))
					| ((pstSetInfo->bSpCh & 0x01) << (MONO_SP & 0xFF))
					| ((pstSetInfo->bSpMixer_Min & 0x01) << (SPL_MMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line1 & 0x01) << (SPL_AMIX & 0xFF))
					| ((pstSetInfo->bSpMixer_Line2 & 0x01) << (SPL_BMIX & 0xFF));
	
	if(g_bSpState == D4NP2_STATE_POWER_ON )
	{
		if(bTempMixer != g_bD4Np2RegisterMap[13])
		{	
			
			bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
			abWritePrm[0] = (bZcs << (ZCS_SPA & 0xFF)) | (bSvol << (SVOL_SP & 0xFF))
							| (0x00 << (MNA & 0xFF));
			bWriteRN = 14;
			D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
			
			if(bSvol == 1)
			{	
				D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
			}
		}
	}

	endtime = get_jiffies_64()+DELAY_ON_TIME_MSEC; 
	while(jiffies < endtime) ; 
	
	
	abWritePrm[0] = bTempMixer;
	
	bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
	abWritePrm[1] = (bZcs << (ZCS_SPA & 0xFF)) | (bSvol << (SVOL_SP & 0xFF))
					| ((pstSetInfo->bSpVol & 0x1F) << (MNA & 0xFF));
	bWriteRN = 13;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, pstSetInfo->bSpVol & 0x1F );
	}
	
	D4Np2_WriteRegisterBit( HIZ_SPR, 1 ); 
	D4Np2_WriteRegisterBit( HIZ_SPL, 1 ); 
	g_bSpState = D4NP2_STATE_POWER_ON;
}













static void D4Np2_PowerOn_HpOut(D4NP2_SETTING_INFO *pstSetInfo)
{
	UINT8 bWriteRN, abWritePrm[2];
	UINT8 bZcs, bSvol, bTempVol, bTempMixer;
	unsigned long endtime; 

	D4Np2_WriteRegisterBit( HIZ_HP, 0 ); 
	
	if( ((g_bD4Np2RegisterMap[2] & 0x04) >> (PD_REG & 0xFF)) == 1 )
	{
		D4Np2_WriteRegisterBit( PD_REG, 0 );
	}
	
	if( ((g_bD4Np2RegisterMap[2] & 0x01) >> (PD_CHP & 0xFF)) == 1 )
	{



		abWritePrm[0] = (g_bD4Np2RegisterMap[2] & 0xFE);

		bWriteRN = 2;
		D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
		d4np2Wait(D4NP2_CHARGEPUMPWAKEUPTIME);	
	}
	
	if(pstSetInfo->bHpVolMode == 2)
	{
			bZcs = 0;
			bSvol = 1;
	}
	else if(pstSetInfo->bHpVolMode == 1)
	{
		bZcs = 1;
		bSvol = 0;
	}
	else
	{
		bZcs = 0;
		bSvol = 0;
	}
	 bTempMixer = ((pstSetInfo->bHpMixer_Min & 0x01) << (HPR_MMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line1 & 0x01) << (HPR_AMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line2 & 0x01) << (HPR_BMIX & 0xFF))
					| ((pstSetInfo->bHpCh & 0x01) << (MONO_HP & 0xFF))
					| ((pstSetInfo->bHpMixer_Min & 0x01) << (HPL_MMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line1 & 0x01) << (HPL_AMIX & 0xFF))
					| ((pstSetInfo->bHpMixer_Line2 & 0x01) << (HPL_BMIX & 0xFF));
	
	if(g_bHpState == D4NP2_STATE_POWER_ON)
	{
		if( bTempMixer != g_bD4Np2RegisterMap[12])
		{
			
			bTempVol = g_bD4Np2RegisterMap[15] & 0x1F;
			abWritePrm[0] = (bZcs << (ZCS_HPA & 0xFF)) | (bSvol << (SVOL_HP & 0xFF))
							| (0x01 << (LAT_HP & 0xFF))
							| (0x00 << (SALA & 0xFF));
			abWritePrm[1] = (0x00 << (SARA & 0xFF));
			bWriteRN = 15;
			D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
			
			if(bSvol == 1)
			{	
				D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
			}
		}
	}

	endtime = get_jiffies_64()+DELAY_ON_TIME_MSEC; 
	while(jiffies < endtime) ; 

	
	abWritePrm[0] = bTempMixer;
	bWriteRN = 12;
	D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
	
	bTempVol = g_bD4Np2RegisterMap[15] & 0x1F;
	abWritePrm[0] = (bZcs << (ZCS_HPA & 0xFF)) | (bSvol << (SVOL_HP & 0xFF))
					| (0x01 << (LAT_HP & 0xFF))
					| ((pstSetInfo->bHpVol & 0x1F) << (SALA & 0xFF));
	abWritePrm[1] = ((pstSetInfo->bHpVol & 0x1F) << (SARA & 0xFF));
	bWriteRN = 15;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, pstSetInfo->bHpVol & 0x1F );
	}
	
	D4Np2_WriteRegisterBit( HIZ_HP, 1 ); 
	g_bHpState = D4NP2_STATE_POWER_ON;
}













static void D4Np2_PowerOn_OutputPart(D4NP2_SETTING_INFO *pstSetInfo)
{
	
	if(pstSetInfo->bRecSW == 0){
		
		if( ((g_bD4Np2RegisterMap[3] & 0x10) >> (PD_REC & 0xFF)) == 0x01 )
		{
			D4Np2_PowerOn_ReceiverOut(pstSetInfo);
		}
	}
	else
	{
		
		if( ((g_bD4Np2RegisterMap[3] & 0x10) >> (PD_REC & 0xFF)) == 0x00 )
		{
			D4Np2_PowerOff_ReceiverOut();
		}
	}
	
	if( (pstSetInfo->bSpMixer_Min + pstSetInfo->bSpMixer_Line1 + pstSetInfo->bSpMixer_Line2) == 0 )
	{
		
		D4Np2_PowerOff_SpOut();
	}
	else if((pstSetInfo->bMinVol == 0) && (pstSetInfo->bLine1Vol == 0) && (pstSetInfo->bLine2Vol == 0))
	{
		
		D4Np2_PowerOff_SpOut();
	}
	else
	{
		
		D4Np2_PowerOn_SpOut(pstSetInfo);
	}
	
	if( (pstSetInfo->bHpMixer_Min + pstSetInfo->bHpMixer_Line1 + pstSetInfo->bHpMixer_Line2) == 0 )
	{
		
		D4Np2_PowerOff_HpOut();
	}
	else if((pstSetInfo->bMinVol == 0) && (pstSetInfo->bLine1Vol == 0) && (pstSetInfo->bLine2Vol == 0))
	{
		
		D4Np2_PowerOff_HpOut();
	}
	else
	{
		
		D4Np2_PowerOn_HpOut(pstSetInfo);
	}
	
	if( (g_bSpState == D4NP2_STATE_POWER_OFF) && (g_bHpState == D4NP2_STATE_POWER_OFF) )
	{
		
		D4Np2_PowerOff_InputPart();
		
		D4Np2_WriteRegisterBit( PDP, 1 );
	}
}













void D4Np2_PowerOn(D4NP2_SETTING_INFO *pstSetInfo)
{
	
	D4Np2_PowerOn_CommonPart();

	
	if( g_bD4Np2RegisterMap[ 0 ] == 0x00 )
	{
		
		D4Np2_PowerOn_InputPart(pstSetInfo);
		
		D4Np2_PowerOn_OutputPart(pstSetInfo);
	}
}













void D4Np2_PowerOff(void)
{
	
	D4Np2_PowerOff_OutputPart();
	
	D4Np2_PowerOff_InputPart();
	
	D4Np2_PowerOff_CommonPart();
}















void D4Np2_PowerOn_SpOut_LRch(D4NP2_SETTING_INFO *pstSetInfo, int bOnLch, int bOnRch) 
{
	UINT8 bWriteRN, abWritePrm[3];
	UINT8 bZcs, bSvol, bTempVol, bTempMixer;
	unsigned char bSpLMixer_Min = 0x00;	
	unsigned char bSpLMixer_Line1 = 0x00;	
	unsigned char bSpLMixer_Line2 = 0x00;	
	unsigned char bSpRMixer_Min = 0x00;	
	unsigned char bSpRMixer_Line1 = 0x00;	
	unsigned char bSpRMixer_Line2 = 0x00;	

	if(bOnLch == 1 && bOnRch == 0) {
		bSpLMixer_Min = pstSetInfo->bSpMixer_Min;	
		bSpLMixer_Line1 = pstSetInfo->bSpMixer_Line1;	
		bSpLMixer_Line2 = pstSetInfo->bSpMixer_Line2;
	}
	else if(bOnLch == 0 && bOnRch == 1) {
		bSpRMixer_Min = pstSetInfo->bSpMixer_Min;	
		bSpRMixer_Line1 = pstSetInfo->bSpMixer_Line1;	
		bSpRMixer_Line2 = pstSetInfo->bSpMixer_Line2;	
	}
	else {
		return;
	}
	
	




	abWritePrm[0] = (g_bD4Np2RegisterMap[3] & 0xF3)
					| (0x00 << (HIZ_SPL & 0xFF)) | (0x00 << (HIZ_SPR & 0xFF));

	
    
	if( pstSetInfo->bSpNonClip == 1)
	{
		abWritePrm[1] = ((pstSetInfo->bDistortion & 0x07) << (DALC & 0xFF));
	}
	else
	{
		abWritePrm[1] = ((pstSetInfo->bPowerLimit & 0x07) << (DPLT & 0xFF));
	}
	
	abWritePrm[2] = ((pstSetInfo->bReleaseTime & 0x03)<< (DREL & 0xFF))
					| ((pstSetInfo->bAttackTime & 0x03) << (DATT & 0xFF));
	bWriteRN = 3;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 3 );

	
	if(pstSetInfo->bSpVolMode == 2)
	{
		bZcs = 0;
		bSvol = 1;
	}
	else if(pstSetInfo->bSpVolMode == 1)
	{
		bZcs = 1;
		bSvol = 0;
	}
	else
	{
		bZcs = 0;
		bSvol = 0;
	}
	
	bTempMixer = ((pstSetInfo->bSpSwap & 0x01) << (SWAP_SP & 0xFF))
					| ((bSpRMixer_Min & 0x01) << (SPR_MMIX & 0xFF))
					| ((bSpRMixer_Line1 & 0x01) << (SPR_AMIX & 0xFF))
					| ((bSpRMixer_Line2 & 0x01) << (SPR_BMIX & 0xFF))
					| ((pstSetInfo->bSpCh & 0x01) << (MONO_SP & 0xFF))
					| ((bSpLMixer_Min & 0x01) << (SPL_MMIX & 0xFF))
					| ((bSpLMixer_Line1 & 0x01) << (SPL_AMIX & 0xFF))
					| ((bSpLMixer_Line2 & 0x01) << (SPL_BMIX & 0xFF));
	
	
	if(g_bSpState == D4NP2_STATE_POWER_ON )
	{
		if(bTempMixer != g_bD4Np2RegisterMap[13])
		{	
			
			bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
			abWritePrm[0] = (bZcs << (ZCS_SPA & 0xFF)) | (bSvol << (SVOL_SP & 0xFF))
							| (0x00 << (MNA & 0xFF));
			bWriteRN = 14;
			D4Np2_WriteRegisterByte( bWriteRN, abWritePrm[0] );
			
			if(bSvol == 1)
			{	
				D4Np2_SoftVolume_Wait( bTempVol, 0x00 );
			}
		}
	}
	
	abWritePrm[0] = bTempMixer;
	
	bTempVol = g_bD4Np2RegisterMap[14] & 0x1F;
	abWritePrm[1] = (bZcs << (ZCS_SPA & 0xFF)) | (bSvol << (SVOL_SP & 0xFF))
					| ((pstSetInfo->bSpVol & 0x1F) << (MNA & 0xFF));
	bWriteRN = 13;
	D4Np2_WriteRegisterByteN( bWriteRN, abWritePrm, 2 );
	
	if(bSvol == 1)
	{
		D4Np2_SoftVolume_Wait( bTempVol, pstSetInfo->bSpVol & 0x1F );
	}
	
	g_bSpState = D4NP2_STATE_POWER_ON;
}

