

/*
* Copyright (c) 2011 M7System Co., Ltd.
*    
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*/
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/

#if !defined(__MXT224E_H__)
#define __MXT224E_H__

#define MXT224E_DEV_NAME "Atmel MXT224E"

enum {
     RESERVED_T0 = 0,
     RESERVED_T1,
     DEBUG_DELTAS_T2,
     DEBUG_REFERENCES_T3,
     DEBUG_SIGNALS_T4,
     GEN_MESSAGEPROCESSOR_T5,
     GEN_COMMANDPROCESSOR_T6,
     GEN_POWERCONFIG_T7,
     GEN_ACQUISITIONCONFIG_T8,
     TOUCH_MULTITOUCHSCREEN_T9,
     TOUCH_SINGLETOUCHSCREEN_T10,
     TOUCH_XSLIDER_T11,
     TOUCH_YSLIDER_T12,
     TOUCH_XWHEEL_T13,
     TOUCH_YWHEEL_T14,
     TOUCH_KEYARRAY_T15,
     PROCG_SIGNALFILTER_T16,
     PROCI_LINEARIZATIONTABLE_T17,
     SPT_COMCONFIG_T18,
     SPT_GPIOPWM_T19,
     PROCI_GRIPFACESUPPRESSION_T20,
     RESERVED_T21,
     PROCG_NOISESUPPRESSION_T22,
     TOUCH_PROXIMITY_T23,
     PROCI_ONETOUCHGESTUREPROCESSOR_T24,
     SPT_SELFTEST_T25,
     DEBUG_CTERANGE_T26,
     PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
     SPT_CTECONFIG_T28,
     SPT_GPI_T29,
     SPT_GATE_T30,
     TOUCH_KEYSET_T31,
     TOUCH_XSLIDERSET_T32,
     RESERVED_T33,
     GEN_MESSAGEBLOCK_T34,
     SPT_GENERICDATA_T35,
     RESERVED_T36,
     DEBUG_DIAGNOSTIC_T37,
     SPT_USERDATA_T38,
     SPARE_T39,
     PROCI_GRIPSUPPRESSION_T40,
     SPARE_T41,
     PROCI_TOUCHSUPPRESSION_T42,
     SPARE_T43,
     SPARE_T44,
     SPARE_T45,
     SPT_CTECONFIG_T46,
     PROCI_STYLUS_T47,
     PROCG_NOISESUPPRESSION_T48,
     SPARE_T49,
     SPARE_T50,
     MXT_MAX_OBJECT_TYPES,
     RESERVED_T255 = 255,
};

struct mxt224E_platform_data {
     int max_finger_touches;
     const u8 **config;
     int gpio_read_done;
     int min_x;
     int max_x;
     int min_y;
     int max_y;
     int min_z;
     int max_z;
     int min_w;
     int max_w;
     void (*power_on)(void);
     void (*power_off)(void);
};


#define T38_USERDATA0             0
#define T38_USERDATA1             0     
#define T38_USERDATA2             0     
#define T38_USERDATA3             0    
#define T38_USERDATA4             0     
#define T38_USERDATA5             0         
#define T38_USERDATA6             0
#define T38_USERDATA7             0

#define T7_IDLEACQINT             64
#define T7_ACTVACQINT             255
#define T7_ACTV2IDLETO            50


#define T8_CHRGTIME               24        
#define T8_CHRGTIME_TA     		  0         
#define T8_ATCHDRIFT              0
#define T8_TCHDRIFT               5
#define T8_DRIFTST                1
#define T8_TCHAUTOCAL             0
#define T8_SYNC                   0
#define T8_ATCHCALST               	0
#define T8_ATCHCALSTHR             	1
#define T8_ATCHFRCCALTHR          	127          
#define T8_ATCHFRCCALRATIO          127          


#define T9_CTRL                   139
#define T9_XORIGIN                0
#define T9_YORIGIN                0
#define T9_XSIZE                  19
#define T9_YSIZE                  11
#define T9_AKSCFG                 0
#define T9_BLEN                   16
#define T9_TCHTHR                 30
#define T9_TCHDI                  2
#define T9_ORIENT                 3
#define T9_MRGTIMEOUT             0
#define T9_MOVHYSTI               3
#define T9_MOVHYSTN               1
#define T9_MOVFILTER              1
#define T9_NUMTOUCH               5        
#define T9_MRGHYST                10        
#define T9_MRGTHR                 20    
#define T9_AMPHYST                20
#define T9_XRANGE_LOW   0xFF
#define T9_XRANGE_HIGH  0x3
#define T9_YRANGE_LOW    0xFF
#define T9_YRANGE_HIGH 0x3
#define T9_XLOCLIP                0
#define T9_XHICLIP                0
#define T9_YLOCLIP                0
#define T9_YHICLIP                0
#define T9_XEDGECTRL              33  
#define T9_XEDGEDIST              15    
#define T9_YEDGECTRL              36  
#define T9_YEDGEDIST              13    
#define T9_JUMPLIMIT              20    
#define T9_TCHHYST                20     
#define T9_XPITCH                 0     
#define T9_YPITCH                 0     
#define T9_NEXTTCHDI              1        


#define T15_CTRL                  0   
#define T15_XORIGIN               0
#define T15_XORIGIN_4KEY          0
#define T15_YORIGIN               0
#define T15_XSIZE                    0
#define T15_XSIZE_4KEY          0
#define T15_YSIZE                 0
#define T15_AKSCFG                0
#define T15_BLEN                  0
#define T15_TCHTHR                0
#define T15_TCHDI                 0
#define T15_RESERVED_0            0
#define T15_RESERVED_1            0


#define T18_CTRL                  0
#define T18_COMMAND               0


#define T19_CTRL                  0
#define T19_REPORTMASK            0
#define T19_DIR                   0
#define T19_INTPULLUP             0
#define T19_OUT                   0
#define T19_WAKE                  0
#define T19_PWM                   0
#define T19_PERIOD                0
#define T19_DUTY_0                0
#define T19_DUTY_1                0
#define T19_DUTY_2                0
#define T19_DUTY_3                0
#define T19_TRIGGER_0             0
#define T19_TRIGGER_1             0
#define T19_TRIGGER_2             0
#define T19_TRIGGER_3             0


#define T23_CTRL                  0
#define T23_XORIGIN               0
#define T23_YORIGIN               0
#define T23_XSIZE                 0
#define T23_YSIZE                 0
#define T23_RESERVED              0
#define T23_BLEN                  0
#define T23_FXDDTHR               0
#define T23_FXDDI                 0
#define T23_AVERAGE               0
#define T23_MVNULLRATE            0
#define T23_MVDTHR                0


#define T24_CTRL                  0
#define T24_NUMGEST               0
#define T24_GESTEN                0
#define T24_PROCESS               0
#define T24_TAPTO                 0
#define T24_FLICKTO               0
#define T24_DRAGTO                0
#define T24_SPRESSTO              0
#define T24_LPRESSTO              0
#define T24_REPPRESSTO            0
#define T24_FLICKTHR              0
#define T24_DRAGTHR               0
#define T24_TAPTHR                0
#define T24_THROWTHR              0


#define T25_CTRL                  0
#define T25_CMD                   0
#define T25_SIGLIM_0_UPSIGLIM     0
#define T25_SIGLIM_0_LOSIGLIM     0
#define T25_SIGLIM_1_UPSIGLIM     0
#define T25_SIGLIM_1_LOSIGLIM     0
#define T25_SIGLIM_2_UPSIGLIM     0
#define T25_SIGLIM_2_LOSIGLIM     0



#define T40_CTRL                  0
#define T40_XLOGRIP               0
#define T40_XHIGRIP               0
#define T40_YLOGRIP               0
#define T40_YHIGRIP               0



#define T42_CTRL                  3
#define T42_APPRTHR               20   
#define T42_MAXAPPRAREA           40     
#define T42_MAXTCHAREA            35     
#define T42_SUPSTRENGTH           224   
#define T42_SUPEXTTO              0   
#define T42_MAXNUMTCHS            8   
#define T42_SHAPESTRENGTH         0   


#define T46_CTRL                  0  
#define T46_MODE                  3  
#define T46_IDLESYNCSPERX          16
#define T46_ACTVSYNCSPERX          32
#define T46_IDLESYNCSPERX_TA          0
#define T46_ACTVSYNCSPERX_TA          0
#define T46_ADCSPERSYNC           0
#define T46_PULSESPERADC          0  
#define T46_XSLEW                 1  
#define T46_SYNCDELAY                 0


#define T47_CTRL                  0
#define T47_CONTMIN               0
#define T47_CONTMAX               0
#define T47_STABILITY             0
#define T47_MAXTCHAREA            0
#define T47_AMPLTHR               0
#define T47_STYSHAPE              0
#define T47_HOVERSUP              0
#define T47_CONFTHR               0
#define T47_SYNCSPERX             0



#define T48_CTRL_TA                  23
#define T48_CFG_TA                   0 
#define T48_CALCFG_TA                96 
#define T48_BASEFREQ_TA              0
#define     T48_RESERVED0_TA             0 
#define     T48_RESERVED1_TA             0 
#define     T48_RESERVED2_TA             0 
#define     T48_RESERVED3_TA             0 
#define T48_MFFREQ_2_TA              0
#define T48_MFFREQ_3_TA              0
#define     T48_RESERVED4_TA             0 
#define     T48_RESERVED5_TA             0 
#define     T48_RESERVED6_TA             0 
#define T48_GCACTVINVLDADCS_TA       	 6
#define T48_GCIDLEINVLDADCS_TA           6  
#define     T48_RESERVED7_TA             0 
#define     T48_RESERVED8_TA             0 
#define T48_GCMAXADCSPERX_TA         100
#define T48_GCLIMITMIN_TA               6
#define T48_GCLIMITMAX_TA               64
#define T48_GCCOUNTMINTGT_TA          10
#define T48_MFINVLDDIFFTHR_TA          20
#define T48_MFINCADCSPXTHR_TA          5
#define T48_MFERRORTHR_TA               42
#define     T48_SELFREQMAX_TA            10
#define     T48_RESERVED9_TA             0 
#define     T48_RESERVED10_TA            0 
#define     T48_RESERVED11_TA            0 
#define     T48_RESERVED12_TA           0 
#define     T48_RESERVED13_TA            0 
#define     T48_RESERVED14_TA            0 

#define T48_BLEN_TA                    0
#define T48_TCHTHR_TA                   100
#define T48_TCHDI_TA                   2
#define T48_MOVHYSTI_TA                3
#define T48_MOVHYSTN_TA                1
#define     T48_MOVFILTER_TA             0 
#define T48_NUMTOUCH_TA                5
#define T48_MRGHYST_TA                 10
#define T48_MRGTHR_TA                  20
#define T48_XLOCLIP_TA                 0
#define T48_XHICLIP_TA                 0
#define T48_YLOCLIP_TA                 0
#define T48_YHICLIP_TA                 0
#define T48_XEDGECTRL_TA               33
#define T48_XEDGEDIST_TA               15
#define T48_YEDGECTRL_TA               36
#define T48_YEDGEDIST_TA               13
#define T48_JUMPLIMIT_TA               30
#define T48_TCHHYST_TA                20
#define T48_NEXTTCHDI_TA               0
#define T48_CHGON_BIT          0x20




#define T38_GLOVE_USERDATA0             0
#define T38_GLOVE_USERDATA1             0     
#define T38_GLOVE_USERDATA2             0     
#define T38_GLOVE_USERDATA3             0    
#define T38_GLOVE_USERDATA4             0     
#define T38_GLOVE_USERDATA5             0         
#define T38_GLOVE_USERDATA6             0
#define T38_GLOVE_USERDATA7               0

#define T7_GLOVE_IDLEACQINT             64
#define T7_GLOVE_ACTVACQINT             255
#define T7_GLOVE_ACTV2IDLETO            50


#define T8_GLOVE_CHRGTIME          24        
#define T8_GLOVE_CHRGTIME_TA     0        
#define T8_GLOVE_ATCHDRIFT              0
#define T8_GLOVE_TCHDRIFT               5
#define T8_GLOVE_DRIFTST                1
#define T8_GLOVE_TCHAUTOCAL             0
#define T8_GLOVE_SYNC                   0
#define T8_GLOVE_ATCHCALST               0
#define T8_GLOVE_ATCHCALSTHR               1
#define T8_GLOVE_ATCHFRCCALTHR          127          
#define T8_GLOVE_ATCHFRCCALRATIO        127          


#define T9_GLOVE_CTRL                   139
#define T9_GLOVE_XORIGIN                0
#define T9_GLOVE_YORIGIN                0
#define T9_GLOVE_XSIZE                  19
#define T9_GLOVE_YSIZE                  11
#define T9_GLOVE_AKSCFG                 0
#define T9_GLOVE_BLEN                   32
#define T9_GLOVE_TCHTHR            25
#define T9_GLOVE_TCHDI                  5
#define T9_GLOVE_ORIENT                 3
#define T9_GLOVE_MRGTIMEOUT             0
#define T9_GLOVE_MOVHYSTI               3
#define T9_GLOVE_MOVHYSTN               1
#define T9_GLOVE_MOVFILTER              1
#define T9_GLOVE_NUMTOUCH               1        
#define T9_GLOVE_MRGHYST                10        
#define T9_GLOVE_MRGTHR                 20    
#define T9_GLOVE_AMPHYST                20
#define T9_GLOVE_XRANGE_LOW   0xFF
#define T9_GLOVE_XRANGE_HIGH  0x3
#define T9_GLOVE_YRANGE_LOW    0xFF
#define T9_GLOVE_YRANGE_HIGH 0x3
#define T9_GLOVE_XLOCLIP                0
#define T9_GLOVE_XHICLIP                0
#define T9_GLOVE_YLOCLIP                0
#define T9_GLOVE_YHICLIP                0
#define T9_GLOVE_XEDGECTRL              33  
#define T9_GLOVE_XEDGEDIST              15    
#define T9_GLOVE_YEDGECTRL              36  
#define T9_GLOVE_YEDGEDIST              13    
#define T9_GLOVE_JUMPLIMIT              20    
#define T9_GLOVE_TCHHYST                20     
#define T9_GLOVE_XPITCH                 0     
#define T9_GLOVE_YPITCH                 0     
#define T9_GLOVE_NEXTTCHDI              3        


#define T15_GLOVE_CTRL                  0   
#define T15_GLOVE_XORIGIN               0
#define T15_GLOVE_XORIGIN_4KEY          0
#define T15_GLOVE_YORIGIN               0
#define T15_GLOVE_XSIZE                    0
#define T15_GLOVE_XSIZE_4KEY          0
#define T15_GLOVE_YSIZE                 0
#define T15_GLOVE_AKSCFG                0
#define T15_GLOVE_BLEN                  0
#define T15_GLOVE_TCHTHR                0
#define T15_GLOVE_TCHDI                 0
#define T15_GLOVE_RESERVED_0            0
#define T15_GLOVE_RESERVED_1            0


#define T18_GLOVE_CTRL                  0
#define T18_GLOVE_COMMAND               0


#define T19_GLOVE_CTRL                  0
#define T19_GLOVE_REPORTMASK            0
#define T19_GLOVE_DIR                   0
#define T19_GLOVE_INTPULLUP             0
#define T19_GLOVE_OUT                   0
#define T19_GLOVE_WAKE                  0
#define T19_GLOVE_PWM                   0
#define T19_GLOVE_PERIOD                0
#define T19_GLOVE_DUTY_0                0
#define T19_GLOVE_DUTY_1                0
#define T19_GLOVE_DUTY_2                0
#define T19_GLOVE_DUTY_3                0
#define T19_GLOVE_TRIGGER_0             0
#define T19_GLOVE_TRIGGER_1             0
#define T19_GLOVE_TRIGGER_2             0
#define T19_GLOVE_TRIGGER_3             0


#define T23_GLOVE_CTRL                  0
#define T23_GLOVE_XORIGIN               0
#define T23_GLOVE_YORIGIN               0
#define T23_GLOVE_XSIZE                 0
#define T23_GLOVE_YSIZE                 0
#define T23_GLOVE_RESERVED              0
#define T23_GLOVE_BLEN                  0
#define T23_GLOVE_FXDDTHR               0
#define T23_GLOVE_FXDDI                 0
#define T23_GLOVE_AVERAGE               0
#define T23_GLOVE_MVNULLRATE            0
#define T23_GLOVE_MVDTHR                0


#define T24_GLOVE_CTRL                  0
#define T24_GLOVE_NUMGEST               0
#define T24_GLOVE_GESTEN                0
#define T24_GLOVE_PROCESS               0
#define T24_GLOVE_TAPTO                 0
#define T24_GLOVE_FLICKTO               0
#define T24_GLOVE_DRAGTO                0
#define T24_GLOVE_SPRESSTO              0
#define T24_GLOVE_LPRESSTO              0
#define T24_GLOVE_REPPRESSTO            0
#define T24_GLOVE_FLICKTHR              0
#define T24_GLOVE_DRAGTHR               0
#define T24_GLOVE_TAPTHR                0
#define T24_GLOVE_THROWTHR              0


#define T25_GLOVE_CTRL                  0
#define T25_GLOVE_CMD                   0
#define T25_GLOVE_SIGLIM_0_UPSIGLIM     0
#define T25_GLOVE_SIGLIM_0_LOSIGLIM     0
#define T25_GLOVE_SIGLIM_1_UPSIGLIM     0
#define T25_GLOVE_SIGLIM_1_LOSIGLIM     0
#define T25_GLOVE_SIGLIM_2_UPSIGLIM     0
#define T25_GLOVE_SIGLIM_2_LOSIGLIM     0



#define T40_GLOVE_CTRL                  0
#define T40_GLOVE_XLOGRIP               0
#define T40_GLOVE_XHIGRIP               0
#define T40_GLOVE_YLOGRIP               0
#define T40_GLOVE_YHIGRIP               0



#define T42_GLOVE_CTRL                  2
#define T42_GLOVE_APPRTHR               20   
#define T42_GLOVE_MAXAPPRAREA           40     
#define T42_GLOVE_MAXTCHAREA            35     
#define T42_GLOVE_SUPSTRENGTH           224   
#define T42_GLOVE_SUPEXTTO              0   
#define T42_GLOVE_MAXNUMTCHS            8   
#define T42_GLOVE_SHAPESTRENGTH         0   


#define T46_GLOVE_CTRL                  0  
#define T46_GLOVE_MODE                  3  
#define T46_GLOVE_IDLESYNCSPERX          32
#define T46_GLOVE_ACTVSYNCSPERX          63
#define T46_GLOVE_IDLESYNCSPERX_TA          0
#define T46_GLOVE_ACTVSYNCSPERX_TA          0
#define T46_GLOVE_ADCSPERSYNC           0
#define T46_GLOVE_PULSESPERADC          0  
#define T46_GLOVE_XSLEW                 1 
#define T46_GLOVE_SYNCDELAY                 0


#define T47_GLOVE_CTRL                  0
#define T47_GLOVE_CONTMIN               40
#define T47_GLOVE_CONTMAX               200
#define T47_GLOVE_STABILITY             5
#define T47_GLOVE_MAXTCHAREA            4
#define T47_GLOVE_AMPLTHR               35
#define T47_GLOVE_STYSHAPE              40
#define T47_GLOVE_HOVERSUP              255
#define T47_GLOVE_CONFTHR               0
#define T47_GLOVE_SYNCSPERX             40



#define T48_GLOVE_CTRL_TA                  23
#define T48_GLOVE_CFG_TA                   0
#define T48_GLOVE_CALCFG_TA                96 
#define T48_GLOVE_BASEFREQ_TA              0
#define     T48_GLOVE_RESERVED0_TA             0
#define     T48_GLOVE_RESERVED1_TA             0 
#define     T48_GLOVE_RESERVED2_TA             0 
#define     T48_GLOVE_RESERVED3_TA             0 
#define T48_GLOVE_MFFREQ_2_TA              0
#define T48_GLOVE_MFFREQ_3_TA              0
#define     T48_GLOVE_RESERVED4_TA             0 
#define     T48_GLOVE_RESERVED5_TA             0 
#define     T48_GLOVE_RESERVED6_TA             0 
#define T48_GLOVE_GCACTVINVLDADCS_TA       6
#define T48_GLOVE_GCIDLEINVLDADCS_TA         6  
#define     T48_GLOVE_RESERVED7_TA             0 
#define     T48_GLOVE_RESERVED8_TA             0 
#define T48_GLOVE_GCMAXADCSPERX_TA         100
#define T48_GLOVE_GCLIMITMIN_TA               6
#define T48_GLOVE_GCLIMITMAX_TA               64
#define T48_GLOVE_GCCOUNTMINTGT_TA          10
#define T48_GLOVE_MFINVLDDIFFTHR_TA          20
#define T48_GLOVE_MFINCADCSPXTHR_TA          0
#define T48_GLOVE_MFERRORTHR_TA               42
#define     T48_GLOVE_SELFREQMAX_TA               10
#define     T48_GLOVE_RESERVED9_TA             0 
#define     T48_GLOVE_RESERVED10_TA            0 
#define     T48_GLOVE_RESERVED11_TA            0 
#define     T48_GLOVE_RESERVED12_TA           0 
#define     T48_GLOVE_RESERVED13_TA            0 
#define     T48_GLOVE_RESERVED14_TA            0 

#define T48_GLOVE_BLEN_TA                    0
#define T48_GLOVE_TCHTHR_TA                  100
#define T48_GLOVE_TCHDI_TA                   2
#define T48_GLOVE_MOVHYSTI_TA                3
#define T48_GLOVE_MOVHYSTN_TA                1
#define     T48_GLOVE_MOVFILTER_TA           0 
#define T48_GLOVE_NUMTOUCH_TA                5
#define T48_GLOVE_MRGHYST_TA                 10
#define T48_GLOVE_MRGTHR_TA                  20
#define T48_GLOVE_XLOCLIP_TA                 0
#define T48_GLOVE_XHICLIP_TA                 0
#define T48_GLOVE_YLOCLIP_TA                 0
#define T48_GLOVE_YHICLIP_TA                 0
#define T48_GLOVE_XEDGECTRL_TA               33
#define T48_GLOVE_XEDGEDIST_TA               15
#define T48_GLOVE_YEDGECTRL_TA               36
#define T48_GLOVE_YEDGEDIST_TA               13
#define T48_GLOVE_JUMPLIMIT_TA               30
#define T48_GLOVE_TCHHYST_TA                 20
#define T48_GLOVE_NEXTTCHDI_TA               0
#define T48_GLOVE_CHGON_BIT          		 0x20




#define T7_CHARGER_IDLEACQINT             255
#define T7_CHARGER_ACTVACQINT             255
#define T7_CHARGER_ACTV2IDLETO            50


#define T8_CHARGER_CHRGTIME               24        
#define T8_CHRGTIME_TA     		  0         
#define T8_CHARGER_ATCHDRIFT              0
#define T8_CHARGER_TCHDRIFT               5
#define T8_CHARGER_DRIFTST                1
#define T8_CHARGER_TCHAUTOCAL             0
#define T8_CHARGER_SYNC                   0
#define T8_CHARGER_ATCHCALST               	0
#define T8_CHARGER_ATCHCALSTHR             	1
#define T8_CHARGER_ATCHFRCCALTHR          	127          
#define T8_CHARGER_ATCHFRCCALRATIO          127          


#define T9_CHARGER_CTRL                   139
#define T9_CHARGER_XORIGIN                0
#define T9_CHARGER_YORIGIN                0
#define T9_CHARGER_XSIZE                  19
#define T9_CHARGER_YSIZE                  11
#define T9_CHARGER_AKSCFG                 0
#define T9_CHARGER_BLEN                   16
#define T9_CHARGER_TCHTHR            	  50
#define T9_CHARGER_TCHDI                  2
#define T9_CHARGER_ORIENT                 3
#define T9_CHARGER_MRGTIMEOUT             0
#define T9_CHARGER_MOVHYSTI               3
#define T9_CHARGER_MOVHYSTN               1
#define T9_CHARGER_MOVFILTER              1
#define T9_CHARGER_NUMTOUCH               5        
#define T9_CHARGER_MRGHYST                10        
#define T9_CHARGER_MRGTHR                 20    
#define T9_CHARGER_AMPHYST                20
#define T9_CHARGER_XRANGE_LOW   0xFF
#define T9_CHARGER_XRANGE_HIGH  0x3
#define T9_CHARGER_YRANGE_LOW    0xFF
#define T9_CHARGER_YRANGE_HIGH 0x3
#define T9_CHARGER_XLOCLIP                0
#define T9_CHARGER_XHICLIP                0
#define T9_CHARGER_YLOCLIP                0
#define T9_CHARGER_YHICLIP                0
#define T9_CHARGER_XEDGECTRL              33  
#define T9_CHARGER_XEDGEDIST              15    
#define T9_CHARGER_YEDGECTRL              36  
#define T9_CHARGER_YEDGEDIST              13    
#define T9_CHARGER_JUMPLIMIT              20    
#define T9_CHARGER_TCHHYST                20     
#define T9_CHARGER_XPITCH                 0     
#define T9_CHARGER_YPITCH                 0     
#define T9_CHARGER_NEXTTCHDI              1        


#define T15_CHARGER_CTRL                  0   
#define T15_CHARGER_XORIGIN               0
#define T15_CHARGER_XORIGIN_4KEY          0
#define T15_CHARGER_YORIGIN               0
#define T15_CHARGER_XSIZE                 0
#define T15_CHARGER_XSIZE_4KEY          0
#define T15_CHARGER_YSIZE                 0
#define T15_CHARGER_AKSCFG                0
#define T15_CHARGER_BLEN                  0
#define T15_CHARGER_TCHTHR                0
#define T15_CHARGER_TCHDI                 0
#define T15_CHARGER_RESERVED_0            0
#define T15_CHARGER_RESERVED_1            0


#define T18_CHARGER_CTRL                  0
#define T18_CHARGER_COMMAND               0


#define T19_CHARGER_CTRL                  0
#define T19_CHARGER_REPORTMASK            0
#define T19_CHARGER_DIR                   0
#define T19_CHARGER_INTPULLUP             0
#define T19_CHARGER_OUT                   0
#define T19_CHARGER_WAKE                  0
#define T19_CHARGER_PWM                   0
#define T19_CHARGER_PERIOD                0
#define T19_CHARGER_DUTY_0                0
#define T19_CHARGER_DUTY_1                0
#define T19_CHARGER_DUTY_2                0
#define T19_CHARGER_DUTY_3                0
#define T19_CHARGER_TRIGGER_0             0
#define T19_CHARGER_TRIGGER_1             0
#define T19_CHARGER_TRIGGER_2             0
#define T19_CHARGER_TRIGGER_3             0


#define T23_CHARGER_CTRL                  0
#define T23_CHARGER_XORIGIN               0
#define T23_CHARGER_YORIGIN               0
#define T23_CHARGER_XSIZE                 0
#define T23_CHARGER_YSIZE                 0
#define T23_CHARGER_RESERVED              0
#define T23_CHARGER_BLEN                  0
#define T23_CHARGER_FXDDTHR               0
#define T23_CHARGER_FXDDI                 0
#define T23_CHARGER_AVERAGE               0
#define T23_CHARGER_MVNULLRATE            0
#define T23_CHARGER_MVDTHR                0


#define T24_CHARGER_CTRL                  0
#define T24_CHARGER_NUMGEST               0
#define T24_CHARGER_GESTEN                0
#define T24_CHARGER_PROCESS               0
#define T24_CHARGER_TAPTO                 0
#define T24_CHARGER_FLICKTO               0
#define T24_CHARGER_DRAGTO                0
#define T24_CHARGER_SPRESSTO              0
#define T24_CHARGER_LPRESSTO              0
#define T24_CHARGER_REPPRESSTO            0
#define T24_CHARGER_FLICKTHR              0
#define T24_CHARGER_DRAGTHR               0
#define T24_CHARGER_TAPTHR                0
#define T24_CHARGER_THROWTHR              0


#define T25_CHARGER_CTRL                  0
#define T25_CHARGER_CMD                   0
#define T25_CHARGER_SIGLIM_0_UPSIGLIM     0
#define T25_CHARGER_SIGLIM_0_LOSIGLIM     0
#define T25_CHARGER_SIGLIM_1_UPSIGLIM     0
#define T25_CHARGER_SIGLIM_1_LOSIGLIM     0
#define T25_CHARGER_SIGLIM_2_UPSIGLIM     0
#define T25_CHARGER_SIGLIM_2_LOSIGLIM     0



#define T40_CHARGER_CTRL                  0
#define T40_CHARGER_XLOGRIP               0
#define T40_CHARGER_XHIGRIP               0
#define T40_CHARGER_YLOGRIP               0
#define T40_CHARGER_YHIGRIP               0



#define T42_CHARGER_CTRL                  3
#define T42_CHARGER_APPRTHR               20   
#define T42_CHARGER_MAXAPPRAREA           40     
#define T42_CHARGER_MAXTCHAREA            35     
#define T42_CHARGER_SUPSTRENGTH           224   
#define T42_CHARGER_SUPEXTTO              0   
#define T42_CHARGER_MAXNUMTCHS            8   
#define T42_CHARGER_SHAPESTRENGTH         0   


#define T46_CHARGER_CTRL                  0  
#define T46_CHARGER_MODE                  3  
#define T46_CHARGER_IDLESYNCSPERX          16
#define T46_CHARGER_ACTVSYNCSPERX          32
#define T46_CHARGER_IDLESYNCSPERX_TA          0
#define T46_CHARGER_ACTVSYNCSPERX_TA          0
#define T46_CHARGER_ADCSPERSYNC           0
#define T46_CHARGER_PULSESPERADC          0  
#define T46_CHARGER_XSLEW                 1  
#define T46_CHARGER_SYNCDELAY                 0


#define T47_CHARGER_CTRL                  0
#define T47_CHARGER_CONTMIN               0
#define T47_CHARGER_CONTMAX               0
#define T47_CHARGER_STABILITY             0
#define T47_CHARGER_MAXTCHAREA            0
#define T47_CHARGER_AMPLTHR               0
#define T47_CHARGER_STYSHAPE              0
#define T47_CHARGER_HOVERSUP              0
#define T47_CHARGER_CONFTHR               0
#define T47_CHARGER_SYNCSPERX             0



#define T48_CHARGER_CTRL_TA                  23
#define T48_CHARGER_CFG_TA                   0 
#define T48_CHARGER_CALCFG_TA                112
#define T48_CHARGER_BASEFREQ_TA              0
#define T48_CHARGER_RESERVED0_TA             0 
#define T48_CHARGER_RESERVED1_TA             0 
#define T48_CHARGER_RESERVED2_TA             0 
#define T48_CHARGER_RESERVED3_TA             0 
#define T48_CHARGER_MFFREQ_2_TA              0
#define T48_CHARGER_MFFREQ_3_TA              0
#define T48_CHARGER_RESERVED4_TA             0 
#define T48_CHARGER_RESERVED5_TA             0 
#define T48_CHARGER_RESERVED6_TA             0 
#define T48_CHARGER_GCACTVINVLDADCS_TA       	 6
#define T48_CHARGER_GCIDLEINVLDADCS_TA           6  
#define T48_CHARGER_RESERVED7_TA             0 
#define T48_CHARGER_RESERVED8_TA             0 
#define T48_CHARGER_GCMAXADCSPERX_TA         100
#define T48_CHARGER_GCLIMITMIN_TA               6
#define T48_CHARGER_GCLIMITMAX_TA               64
#define T48_CHARGER_GCCOUNTMINTGT_TA          10
#define T48_CHARGER_MFINVLDDIFFTHR_TA          20
#define T48_CHARGER_MFINCADCSPXTHR_TA          5
#define T48_CHARGER_MFERRORTHR_TA               60
#define T48_CHARGER_SELFREQMAX_TA               10
#define T48_CHARGER_RESERVED9_TA             0 
#define T48_CHARGER_RESERVED10_TA            0 
#define T48_CHARGER_RESERVED11_TA            0 
#define T48_CHARGER_RESERVED12_TA           0 
#define T48_CHARGER_RESERVED13_TA            0 
#define T48_CHARGER_RESERVED14_TA            0 

#define T48_CHARGER_BLEN_TA                    0
#define T48_CHARGER_TCHTHR_TA                   80
#define T48_CHARGER_TCHDI_TA                   3
#define T48_CHARGER_MOVHYSTI_TA                3
#define T48_CHARGER_MOVHYSTN_TA                1
#define T48_CHARGER_MOVFILTER_TA             0 
#define T48_CHARGER_NUMTOUCH_TA                5
#define T48_CHARGER_MRGHYST_TA                 10
#define T48_CHARGER_MRGTHR_TA                  20
#define T48_CHARGER_XLOCLIP_TA                 0
#define T48_CHARGER_XHICLIP_TA                 0
#define T48_CHARGER_YLOCLIP_TA                 0
#define T48_CHARGER_YHICLIP_TA                 0
#define T48_CHARGER_XEDGECTRL_TA               33
#define T48_CHARGER_XEDGEDIST_TA               15
#define T48_CHARGER_YEDGECTRL_TA               36
#define T48_CHARGER_YEDGEDIST_TA               13
#define T48_CHARGER_JUMPLIMIT_TA               15
#define T48_CHARGER_TCHHYST_TA                20
#define T48_CHARGER_NEXTTCHDI_TA               0
#define T48_CHARGER_CHGON_BIT          0x20


#define T38_WIRELESS_USERDATA0             0
#define T38_WIRELESS_USERDATA1             0     
#define T38_WIRELESS_USERDATA2             0     
#define T38_WIRELESS_USERDATA3             0    
#define T38_WIRELESS_USERDATA4             0     
#define T38_WIRELESS_USERDATA5             0         
#define T38_WIRELESS_USERDATA6             0
#define T38_WIRELESS_USERDATA7               0

#define T7_WIRELESS_IDLEACQINT             64
#define T7_WIRELESS_ACTVACQINT             255
#define T7_WIRELESS_ACTV2IDLETO            50


#define T8_WIRELESS_CHRGTIME               24        
#define T8_WIRELESS_CHRGTIME_TA     		  0         
#define T8_WIRELESS_ATCHDRIFT              0
#define T8_WIRELESS_TCHDRIFT               5
#define T8_WIRELESS_DRIFTST                1
#define T8_WIRELESS_TCHAUTOCAL             0
#define T8_WIRELESS_SYNC                   0
#define T8_WIRELESS_ATCHCALST               	0
#define T8_WIRELESS_ATCHCALSTHR             	1
#define T8_WIRELESS_ATCHFRCCALTHR          	 127          
#define T8_WIRELESS_ATCHFRCCALRATIO          127          


#define T9_WIRELESS_CTRL                   139
#define T9_WIRELESS_XORIGIN                0
#define T9_WIRELESS_YORIGIN                0
#define T9_WIRELESS_XSIZE                  19
#define T9_WIRELESS_YSIZE                  11
#define T9_WIRELESS_AKSCFG                 0
#define T9_WIRELESS_BLEN                   16
#define T9_WIRELESS_TCHTHR                 50
#define T9_WIRELESS_TCHDI                  2
#define T9_WIRELESS_ORIENT                 3
#define T9_WIRELESS_MRGTIMEOUT             0
#define T9_WIRELESS_MOVHYSTI               3
#define T9_WIRELESS_MOVHYSTN               1
#define T9_WIRELESS_MOVFILTER              1
#define T9_WIRELESS_NUMTOUCH               5        
#define T9_WIRELESS_MRGHYST                10        
#define T9_WIRELESS_MRGTHR                 20    
#define T9_WIRELESS_AMPHYST                20
#define T9_WIRELESS_XRANGE_LOW   0xFF
#define T9_WIRELESS_XRANGE_HIGH  0x3
#define T9_WIRELESS_YRANGE_LOW    0xFF
#define T9_WIRELESS_YRANGE_HIGH 0x3
#define T9_WIRELESS_XLOCLIP                0
#define T9_WIRELESS_XHICLIP                0
#define T9_WIRELESS_YLOCLIP                0
#define T9_WIRELESS_YHICLIP                0
#define T9_WIRELESS_XEDGECTRL             33  
#define T9_WIRELESS_XEDGEDIST              15    
#define T9_WIRELESS_YEDGECTRL              36  
#define T9_WIRELESS_YEDGEDIST              13    
#define T9_WIRELESS_JUMPLIMIT              20    
#define T9_WIRELESS_TCHHYST                20     
#define T9_WIRELESS_XPITCH                 0     
#define T9_WIRELESS_YPITCH                 0     
#define T9_WIRELESS_NEXTTCHDI              1        


#define T15_WIRELESS_CTRL                  0   
#define T15_WIRELESS_XORIGIN               0
#define T15_WIRELESS_XORIGIN_4KEY          0
#define T15_WIRELESS_YORIGIN               0
#define T15_WIRELESS_XSIZE                    0
#define T15_WIRELESS_XSIZE_4KEY          0
#define T15_WIRELESS_YSIZE                 0
#define T15_WIRELESS_AKSCFG                0
#define T15_WIRELESS_BLEN                  0
#define T15_WIRELESS_TCHTHR                0
#define T15_WIRELESS_TCHDI                 0
#define T15_WIRELESS_RESERVED_0            0
#define T15_WIRELESS_RESERVED_1            0


#define T18_WIRELESS_CTRL                  0
#define T18_WIRELESS_COMMAND               0


#define T19_WIRELESS_CTRL                  0
#define T19_WIRELESS_REPORTMASK            0
#define T19_WIRELESS_DIR                   0
#define T19_WIRELESS_INTPULLUP             0
#define T19_WIRELESS_OUT                   0
#define T19_WIRELESS_WAKE                  0
#define T19_WIRELESS_PWM                   0
#define T19_WIRELESS_PERIOD                0
#define T19_WIRELESS_DUTY_0                0
#define T19_WIRELESS_DUTY_1                0
#define T19_WIRELESS_DUTY_2                0
#define T19_WIRELESS_DUTY_3                0
#define T19_WIRELESS_TRIGGER_0             0
#define T19_WIRELESS_TRIGGER_1             0
#define T19_WIRELESS_TRIGGER_2             0
#define T19_WIRELESS_TRIGGER_3             0


#define T23_WIRELESS_CTRL                  0
#define T23_WIRELESS_XORIGIN               0
#define T23_WIRELESS_YORIGIN               0
#define T23_WIRELESS_XSIZE                 0
#define T23_WIRELESS_YSIZE                 0
#define T23_WIRELESS_RESERVED              0
#define T23_WIRELESS_BLEN                  0
#define T23_WIRELESS_FXDDTHR               0
#define T23_WIRELESS_FXDDI                 0
#define T23_WIRELESS_AVERAGE               0
#define T23_WIRELESS_MVNULLRATE            0
#define T23_WIRELESS_MVDTHR                0


#define T24_WIRELESS_CTRL                  0
#define T24_WIRELESS_NUMGEST               0
#define T24_WIRELESS_GESTEN                0
#define T24_WIRELESS_PROCESS               0
#define T24_WIRELESS_TAPTO                 0
#define T24_WIRELESS_FLICKTO               0
#define T24_WIRELESS_DRAGTO                0
#define T24_WIRELESS_SPRESSTO              0
#define T24_WIRELESS_LPRESSTO              0
#define T24_WIRELESS_REPPRESSTO            0
#define T24_WIRELESS_FLICKTHR              0
#define T24_WIRELESS_DRAGTHR               0
#define T24_WIRELESS_TAPTHR                0
#define T24_WIRELESS_THROWTHR              0


#define T25_WIRELESS_CTRL                  0
#define T25_WIRELESS_CMD                   0
#define T25_WIRELESS_SIGLIM_0_UPSIGLIM     0
#define T25_WIRELESS_SIGLIM_0_LOSIGLIM     0
#define T25_WIRELESS_SIGLIM_1_UPSIGLIM     0
#define T25_WIRELESS_SIGLIM_1_LOSIGLIM     0
#define T25_WIRELESS_SIGLIM_2_UPSIGLIM     0
#define T25_WIRELESS_SIGLIM_2_LOSIGLIM     0



#define T40_WIRELESS_CTRL                  0
#define T40_WIRELESS_XLOGRIP               0
#define T40_WIRELESS_XHIGRIP               0
#define T40_WIRELESS_YLOGRIP               0
#define T40_WIRELESS_YHIGRIP               0



#define T42_WIRELESS_CTRL                  3
#define T42_WIRELESS_APPRTHR               20   
#define T42_WIRELESS_MAXAPPRAREA           40     
#define T42_WIRELESS_MAXTCHAREA            35     
#define T42_WIRELESS_SUPSTRENGTH           224   
#define T42_WIRELESS_SUPEXTTO              0   
#define T42_WIRELESS_MAXNUMTCHS            8   
#define T42_WIRELESS_SHAPESTRENGTH         0   


#define T46_WIRELESS_CTRL                  0  
#define T46_WIRELESS_MODE                  3  
#define T46_WIRELESS_IDLESYNCSPERX          32
#define T46_WIRELESS_ACTVSYNCSPERX          48
#define T46_WIRELESS_IDLESYNCSPERX_TA          0
#define T46_WIRELESS_ACTVSYNCSPERX_TA          0
#define T46_WIRELESS_ADCSPERSYNC           0
#define T46_WIRELESS_PULSESPERADC          0  
#define T46_WIRELESS_XSLEW                 2  
#define T46_WIRELESS_SYNCDELAY                 0


#define T47_WIRELESS_CTRL                  0
#define T47_WIRELESS_CONTMIN               0
#define T47_WIRELESS_CONTMAX               0
#define T47_WIRELESS_STABILITY             0
#define T47_WIRELESS_MAXTCHAREA            0
#define T47_WIRELESS_AMPLTHR               0
#define T47_WIRELESS_STYSHAPE              0
#define T47_WIRELESS_HOVERSUP              0
#define T47_WIRELESS_CONFTHR               0
#define T47_WIRELESS_SYNCSPERX             0



#define T48_WIRELESS_CTRL_TA                  23
#define T48_WIRELESS_CFG_TA                   0 
#define T48_WIRELESS_CALCFG_TA                114 
#define T48_WIRELESS_BASEFREQ_TA              5
#define T48_WIRELESS_RESERVED0_TA             0 
#define T48_WIRELESS_RESERVED1_TA             0 
#define T48_WIRELESS_RESERVED2_TA             0 
#define T48_WIRELESS_RESERVED3_TA             0 
#define T48_WIRELESS_MFFREQ_2_TA              10
#define T48_WIRELESS_MFFREQ_3_TA              20
#define T48_WIRELESS_RESERVED4_TA             0 
#define T48_WIRELESS_RESERVED5_TA             0 
#define T48_WIRELESS_RESERVED6_TA             0 
#define T48_WIRELESS_GCACTVINVLDADCS_TA       	 	6
#define T48_WIRELESS_GCIDLEINVLDADCS_TA           	6  
#define T48_WIRELESS_RESERVED7_TA             		0 
#define T48_WIRELESS_RESERVED8_TA             		0 
#define T48_WIRELESS_GCMAXADCSPERX_TA         		100
#define T48_WIRELESS_GCLIMITMIN_TA               	6
#define T48_WIRELESS_GCLIMITMAX_TA               	64
#define T48_WIRELESS_GCCOUNTMINTGT_TA          		10
#define T48_WIRELESS_MFINVLDDIFFTHR_TA          	20
#define T48_WIRELESS_MFINCADCSPXTHR_TA          	5
#define T48_WIRELESS_MFERRORTHR_TA               	42
#define T48_WIRELESS_SELFREQMAX_TA               	5
#define T48_WIRELESS_RESERVED9_TA             		0 
#define T48_WIRELESS_RESERVED10_TA            		0 
#define T48_WIRELESS_RESERVED11_TA            		0 
#define T48_WIRELESS_RESERVED12_TA           		0 
#define T48_WIRELESS_RESERVED13_TA            		0 
#define T48_WIRELESS_RESERVED14_TA            		0 

#define T48_WIRELESS_BLEN_TA                    	0
#define T48_WIRELESS_TCHTHR_TA                   	34
#define T48_WIRELESS_TCHDI_TA                   	2
#define T48_WIRELESS_MOVHYSTI_TA                	3
#define T48_WIRELESS_MOVHYSTN_TA                	1
#define T48_WIRELESS_MOVFILTER_TA             		1 
#define T48_WIRELESS_NUMTOUCH_TA                	5
#define T48_WIRELESS_MRGHYST_TA                 	10
#define T48_WIRELESS_MRGTHR_TA                  	20
#define T48_WIRELESS_XLOCLIP_TA                 	0
#define T48_WIRELESS_XHICLIP_TA                 	0
#define T48_WIRELESS_YLOCLIP_TA                 	0
#define T48_WIRELESS_YHICLIP_TA                 	0
#define T48_WIRELESS_XEDGECTRL_TA               	33
#define T48_WIRELESS_XEDGEDIST_TA               	15
#define T48_WIRELESS_YEDGECTRL_TA               	36
#define T48_WIRELESS_YEDGEDIST_TA               	13
#define T48_WIRELESS_JUMPLIMIT_TA               	30
#define T48_WIRELESS_TCHHYST_TA                		20
#define T48_WIRELESS_NEXTTCHDI_TA               	0
#define T48_WIRELESS_CHGON_BIT          			0x20







#define MXT_GEN_MESSAGEPROCESSOR_T5                     5
#define MXT_GEN_COMMANDPROCESSOR_T6                     6
#define MXT_GEN_POWERCONFIG_T7                          7
#define MXT_GEN_ACQUIRECONFIG_T8                        8
#define MXT_TOUCH_MULTITOUCHSCREEN_T9                   9
#define MXT_TOUCH_KEYARRAY_T15                          15
#define MXT_SPT_COMMSCONFIG_T18                         18
#define MXT_SPT_GPIOPWM_T19                             19
#define MXT_TOUCH_PROXIMITY_T23                         23
#define MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24          24
#define MXT_SPT_SELFTEST_T25                            25
#define MXT_DEBUG_DIAGNOSTICS_T37                       37
#define MXT_USER_INFO_T38                               38
#define MXT_PROCI_TOUCHSUPPRESSION_T42					42

#define MXT_SPT_COMMSCONFIG_T18                         18
#define MXT_PROCI_GRIPSUPPRESSION_T40                   40  
#define MXT_PROCI_TOUCHSUPPRESSION_T42                  42  
#define MXT_SPT_CTECONFIG_T46                           46  
#define MXT_PROCI_STYLUS_T47                            47  
#define MXT_PROCG_NOISESUPPRESSION_T48                  48  
#define MXT_GEN_INFOBLOCK16BIT_T254                     254  
#define	MXT_MAX_OBJECT_TYPES							50	


#define MXT_ADR_T6_RESET                                0x00
#define MXT_ADR_T6_BACKUPNV                             0x01
#define MXT_ADR_T6_CALIBRATE                            0x02
#define MXT_ADR_T6_REPORTALL                            0x03
#define MXT_ADR_T6_RESERVED                             0x04
#define MXT_ADR_T6_DIAGNOSTICS                          0x05


#define MXT_ADR_T8_CHRGTIME                             0x00
#define MXT_ADR_T8_RESERVED                             0x01
#define MXT_ADR_T8_TCHDRIFT                             0x02
#define MXT_ADR_T8_DRIFTSTS                             0x03
#define MXT_ADR_T8_TCHAUTOCAL                           0x04
#define MXT_ADR_T8_SYNC                                 0x05
#define MXT_ADR_T8_ATCHCALST                            0x06
#define MXT_ADR_T8_ATCHCALSTHR                          0x07
#define MXT_ADR_T8_ATCHFRCCALTHR                        0x08         
#define MXT_ADR_T8_ATCHFRCCALRATIO                      0x09         

#define MXT_ADR_T9_XORIGIN                              0x01
#define MXT_ADR_T9_YORIGIN                              0x02
#define MXT_ADR_T9_XSIZE                                0x03
#define MXT_ADR_T9_YSIZE                                0x04
#define MXT_ADR_T9_AKSCFG                               0x05
#define MXT_ADR_T9_BLEN                                 0x06
#define MXT_T9_CFGBF_BL(x)                              (x & 0x0F)
#define MXT_T9_CFGBF_GAIN(x)                            ((x >> 4) & 0x0F)
#define MXT_ADR_T9_TCHTHR                               0x07
#define MXT_ADR_T9_TCHDI                                0x08
#define MXT_ADR_T9_ORIENT                               0x09
#define MXT_T9_CFGB_SWITCH(x)                           (((x) >> 0) & 0x01)
#define MXT_T9_CFGB_INVERTX(x)                          (((x) >> 1) & 0x01)
#define MXT_T9_CFGB_INVERTY(x)                          (((x) >> 2) & 0x01)
#define MXT_ADR_T9_MRGTIMEOUT                           0x0a
#define MXT_ADR_T9_MOVHYSTI                             0x0b
#define MXT_ADR_T9_MOVHYSTN                             0x0c
#define MXT_ADR_T9_MOVFILTER                            0x0d
#define MXT_T9_CFGBF_ADAPTTHR(x)                        (((x) >> 0) & 0xF)
#define MXT_T9_CFGB_DISABLE(x)                          (((x) >> 7) & 0x01)
#define MXT_ADR_T9_NUMTOUCH                             0x0e
#define MXT_ADR_T9_MRGHYST                              0x0f
#define MXT_ADR_T9_MRGTHR                               0x10
#define MXT_ADR_T9_AMPHYST                              0x11

#define MXT_ADR_T9_XRANGE                               0x12

#define MXT_ADR_T9_YRANGE                               0x14
#define MXT_ADR_T9_XLOCLIP                              0x16
#define MXT_ADR_T9_XHICLIP                              0x17
#define MXT_ADR_T9_YLOCLIP                              0x18
#define MXT_ADR_T9_YHICLIP                              0x19
#define MXT_ADR_T9_XEDGECTRL                            0x1a
#define MXT_ADR_T9_XEDGEDIST                            0x1b
#define MXT_ADR_T9_YEDGECTRL                            0x1c
#define MXT_ADR_T9_YEDGEDIST                            0x1d



#define MXT_ADR_T15_CTRL                                0x00
#define MXT_T15_CFGB_ENABLE(x)                         (((x) >> 0) & 0x01)
#define MXT_T15_CFGB_RPRTEN(x)                         (((x) >> 1) & 0x01)
#define MXT_T15_CFGB_INTAKSEN(x)                       (((x) >> 7) & 0x01)
#define MXT_ADR_T15_XORIGIN                             0x01
#define MXT_ADR_T15_YORIGIN                             0x02
#define MXT_ADR_T15_XSIZE                               0x03
#define MXT_ADR_T15_YSIZE                               0x04
#define MXT_ADR_T15_AKSCFG                              0x05
#define MXT_ADR_T15_BLEN                                0x06
#define MXT_T15_CFGBF_BL(x)                             (x & 0x0F)
#define MXT_T15_CFGBF_GAIN(x)                           ((x >> 4) & 0x0F)
#define MXT_ADR_T15_TCHTHR                              0x07
#define MXT_ADR_T15_TCHDI                               0x08
#define MXT_ADR_T15_RESERVED1                           0x09
#define MXT_ADR_T15_RESERVED2                           0x0a



#define MXT_ADR_T37_MODE                                0x00
#define MXT_ADR_T37_PAGE                                0x01
#define MXT_ADR_T37_DATA                                0x02


#define MXT_ADR_T38_USER0                               0x00
#define MXT_ADR_T38_USER1                               0x01
#define MXT_ADR_T38_USER2                               0x02
#define MXT_ADR_T38_USER3                               0x03
#define MXT_ADR_T38_USER4                               0x04
#define MXT_ADR_T38_USER5                               0x05
#define MXT_ADR_T38_USER6                               0x06
#define MXT_ADR_T38_USER7                               0x07


#define MXT_ADR_T42_CTRL                                0x00
#define MXT_ADR_T42_APPRTHR                             0x01   
#define MXT_ADR_T42_MAXAPPRAREA                         0x02   
#define MXT_ADR_T42_MAXTCHAREA                          0x03   
#define MXT_ADR_T42_SUPSTRENGTH                         0x04   
#define MXT_ADR_T42_SUPEXTTO                            0x05   
#define MXT_ADR_T42_MAXNUMTCHS                          0x06   
#define MXT_ADR_T42_SHAPESTRENGTH                       0x07   




#define MXT_ADR_T48_CTRL                                0x00
#define MXT_T48_CFGB_ENABLE(x)                         (((x) >> 0) & 0x01)
#define MXT_T48_CFGB_RPRTEN(x)                         (((x) >> 1) & 0x01)
#define MXT_T48_CFGB_RPTFREQ(x)                        (((x) >> 2) & 0x01)
#define MXT_T48_CFGB_RPTAPX(x)                         (((x) >> 3) & 0x01)

#define MXT_ADR_T48_CFG					0x01

#define MXT_ADR_T48_CALCFG                              0x02
#define MXT_T48_CFGB_MFEN(x)                            (((x) >> 1) & 0x01)
#define MXT_T48_CFGB_MEANEN(x)                          (((x) >> 3) & 0x01)
#define MXT_T48_CFGB_DUALXEN(x)                         (((x) >> 4) & 0x01)
#define MXT_T48_CFGB_CHRGON(x)                          (((x) >> 5) & 0x01)
#define MXT_T48_CFGB_INCBIAS(x)                         (((x) >> 6) & 0x01)
#define MXT_T48_CFGB_INCRST(x)                          (((x) >> 7) & 0x01)



#define MXT_MSG_T5_REPORTID                             0x00
#define MXT_MSG_T5_MESSAGE                              0x01
#define MXT_MSG_T5_CHECKSUM                             0x08


#define MXT_MSG_T6_STATUS_NORMAL                        0x00
#define MXT_MSG_T6_STATUS                               0x01
#define MXT_MSGB_T6_COMSERR                             0x04
#define MXT_MSGB_T6_CFGERR                              0x08
#define MXT_MSGB_T6_CAL                                 0x10
#define MXT_MSGB_T6_SIGERR                              0x20
#define MXT_MSGB_T6_OFL                                 0x40
#define MXT_MSGB_T6_RESET                               0x80

#define MXT_MSG_T6_CHECKSUM                             0x02





#define MXT_MSG_T9_STATUS                               0x01

#define MXT_MSGB_T9_SUPPRESS                            0x02
#define MXT_MSGB_T9_AMP                                 0x04
#define MXT_MSGB_T9_VECTOR                              0x08
#define MXT_MSGB_T9_MOVE                                0x10
#define MXT_MSGB_T9_RELEASE                             0x20
#define MXT_MSGB_T9_PRESS                               0x40
#define MXT_MSGB_T9_DETECT                              0x80

#define MXT_MSG_T9_XPOSMSB                              0x02
#define MXT_MSG_T9_YPOSMSB                              0x03
#define MXT_MSG_T9_XYPOSLSB                             0x04
#define MXT_MSG_T9_TCHAREA                              0x05
#define MXT_MSG_T9_TCHAMPLITUDE                         0x06
#define MXT_MSG_T9_TCHVECTOR                            0x07


#define MXT_MSG_T15_STATUS                              0x01
#define MXT_MSGB_T15_DETECT                             0x80

#define MXT_MSG_T15_KEYSTATE                            0x02


#define MXT_MSG_T19_STATUS                              0x01


#define MXT_MSG_T20_STATUS                              0x01
#define MXT_MSGB_T20_FACE_SUPPRESS                      0x01

#define MXT_MSG_T22_STATUS                              0x01
#define MXT_MSGB_T22_FHCHG                              0x01
#define MXT_MSGB_T22_GCAFERR                            0x04
#define MXT_MSGB_T22_FHERR                              0x08
#define MXT_MSG_T22_GCAFDEPTH                           0x02


#define MXT_MSG_T23_STATUS                              0x01
#define MXT_MSGB_T23_FALL                               0x20
#define MXT_MSGB_T23_RISE                               0x40
#define MXT_MSGB_T23_DETECT                             0x80

#define MXT_MSG_T23_PROXDELTA                           0x02


#define MXT_MSG_T24_STATUS                              0x01
#define MXT_MSG_T24_XPOSMSB                             0x02
#define MXT_MSG_T24_YPOSMSB                             0x03
#define MXT_MSG_T24_XYPOSLSB                            0x04
#define MXT_MSG_T24_DIR                                 0x05

#define MXT_MSG_T24_DIST                                0x06


#define MXT_MSG_T25_STATUS                              0x01

#define MXT_MSGR_T25_OK                                 0xFE
#define MXT_MSGR_T25_INVALID_TEST                       0xFD
#define MXT_MSGR_T25_PIN_FAULT                          0x11
#define MXT_MSGR_T25_SIGNAL_LIMIT_FAULT                 0x17
#define MXT_MSGR_T25_GAIN_ERROR                         0x20
#define MXT_MSG_T25_INFO                                0x02


#define MXT_MSG_T27_STATUS                              0x01
#define MXT_MSGB_T27_ROTATEDIR                          0x10
#define MXT_MSGB_T27_PINCH                              0x20
#define MXT_MSGB_T27_ROTATE                             0x40
#define MXT_MSGB_T27_STRETCH                            0x80
#define MXT_MSG_T27_XPOSMSB                             0x02
#define MXT_MSG_T27_YPOSMSB                             0x03
#define MXT_MSG_T27_XYPOSLSB                            0x04
#define MXT_MSG_T27_ANGLE                               0x05


#define MXT_MSG_T27_SEPARATION                          0x06


#define MXT_MSG_T28_STATUS                              0x01
#define MXT_MSGB_T28_CHKERR                             0x01



#define MXT_MSG_T42_STATUS                              0x01
#define MXT_MSGB_T42_TCHSUP				 				0x01



#define MXT_MSG_T46_STATUS                              0x01
#define MXT_MSGB_T46_CHKERR                             0x01


#define MT_GESTURE_RESERVED                             0x00
#define MT_GESTURE_PRESS                                0x01
#define MT_GESTURE_RELEASE                              0x02
#define MT_GESTURE_TAP                                  0x03
#define MT_GESTURE_DOUBLE_TAP                           0x04
#define MT_GESTURE_FLICK                                0x05
#define MT_GESTURE_DRAG                                 0x06
#define MT_GESTURE_SHORT_PRESS                          0x07
#define MT_GESTURE_LONG_PRESS                           0x08
#define MT_GESTURE_REPEAT_PRESS                         0x09
#define MT_GESTURE_TAP_AND_PRESS                        0x0a
#define MT_GESTURE_THROW                                0x0b


#define MXT_APP_LOW 	0x4a
#define MXT_APP_HIGH		0x4b
#define MXT_BOOT_LOW		0x24
#define MXT_BOOT_HIGH		0x25


#define MXT_BOOT_VALUE		0xa5
#define MXT_BACKUP_VALUE	0x55
#define MXT_BACKUP_TIME 	25	
#define MXT224_RESET_TIME		65	
#define MXT224E_RESET_TIME		22	
#define MXT1386_RESET_TIME		250 
#define MXT_RESET_TIME		250 
#define MXT_RESET_NOCHGREAD 	400 

#define MXT_FWRESET_TIME	175 

#define MXT_WAKE_TIME		25


#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc


#define MXT_WAITING_BOOTLOAD_CMD	0xc0	
#define MXT_WAITING_FRAME_DATA	0x80	
#define MXT_FRAME_CRC_CHECK 0x02
#define MXT_FRAME_CRC_FAIL	0x03
#define MXT_FRAME_CRC_PASS	0x04
#define MXT_APP_CRC_FAIL	0x40	
#define MXT_BOOT_STATUS_MASK	0x3f


#define	T48_CTRL				3
#define	T48_CFG					4  
#define	T48_CALCFG              0x40
#define	T48_BASEFREQ            0 
#define	T48_RESERVED0           0  
#define	T48_RESERVED1           0  
#define	T48_RESERVED2           0  
#define	T48_RESERVED3           0  
#define	T48_MFFREQ_2            0
#define	T48_MFFREQ_3            0
#define	T48_RESERVED4           0  
#define	T48_RESERVED5           0  
#define	T48_RESERVED6           0  
#define	T48_GCACTVINVLDADCS     6
#define	T48_GCIDLEINVLDADCS     6   
#define	T48_RESERVED7           0  
#define	T48_RESERVED8           0  
#define	T48_GCMAXADCSPERX       100
#define	T48_GCLIMITMIN			6
#define	T48_GCLIMITMAX			64
#define	T48_GCCOUNTMINTGT		10
#define	T48_MFINVLDDIFFTHR		32
#define	T48_MFINCADCSPXTHR		5
#define	T48_MFERRORTHR			38
#define	T48_SELFREQMAX			5 
#define	T48_RESERVED9           0  
#define	T48_RESERVED10          0  
#define	T48_RESERVED11          0  
#define	T48_RESERVED12          0  
#define	T48_RESERVED13          0  
#define	T48_RESERVED14          0  
#define	T48_BLEN                0x10
#define	T48_TCHTHR              70
#define	T48_TCHDI               2
#define	T48_MOVHYSTI            10
#define	T48_MOVHYSTN            3
#define	T48_MOVFILTER           0x2F
#define	T48_NUMTOUCH            10
#define	T48_MRGHYST             70
#define	T48_MRGTHR              70
#define	T48_XLOCLIP             10
#define	T48_XHICLIP             10
#define	T48_YLOCLIP             10
#define	T48_YHICLIP             10
#define	T48_XEDGECTRL           143
#define	T48_XEDGEDIST           40
#define	T48_YEDGECTRL           143
#define	T48_YEDGEDIST           80
#define	T48_JUMPLIMIT           9
#define	T48_TCHHYST             25
#define	T48_NEXTTCHDI           2

#define	T48_CHGON_BIT			0x20

#define MXT_MAX_NUM_TOUCHES 	5	

#define I2C_RETRY_COUNT 		10
#define I2C_PAYLOAD_SIZE 		254

#define MXT_END_OF_MESSAGES 	255


#endif



