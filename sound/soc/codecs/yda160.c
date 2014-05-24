/**********************************************************************
* File Name: sound/soc/codecs/yda160.c
* 
* (C) NEC CASIO Mobile Communications, Ltd. 2013
**********************************************************************/

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <sound/soc.h>

#include <linux/mfd/core.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpio.h>

#include <sound/yda160.h>

#include <linux/module.h>

#include "D4NP2_Ctrl_def.h"
#include "D4NP2_Ctrl.h"
#include "d4np2machdep.h"
#include "yda160.h"




#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <sound/es310.h>


#include <linux/msm_audio.h>
#include "yda160_sndamp_diag.h"
DEFINE_MUTEX(yda160_diag_dev_lock);




#define YDA160_I2C_ADDRESS		0x6C
static struct i2c_client *yda160_i2c_client;
static int yda160_mode_control; 


enum{
	YDA160_AMP_MONO_IN_PATH,		
	YDA160_AMP_LINE1_IN_PATH,		
	YDA160_AMP_LINE2_IN_PATH,		
	YDA160_AMP_RECV_IN_PATH,		
	YDA160_AMP_HP_OUT_PATH,		
	YDA160_AMP_SP_OUT_PATH,		
	YDA160_AMP_RECV_OUT_PATH,
	YDA160_AMP_OFF_OUT_PATH,
};
















enum{
    MODE_OFF_OUT,
  	MODE_HP_OUT,
  	MODE_SP_OUT,
  	MODE_RECV_OUT,
  	MODE_SP_HP_OUT, 
  	MODE_VIOCE_HP_OUT, 
  	MODE_VIOCE_SP_OUT, 
};


enum{
	MODE_IN_OFF,		
	MODE_IN_AUDIO,		
	MODE_IN_VOICE,		
};


extern UINT8 g_bD4Np2RegisterMap[];

#define SNDAMP_ADDR_BASE                            0x80
#define SNDAMP_ADDR(offset)                         (SNDAMP_ADDR_BASE + offset)
#define SNDAMP_BUF(offset)                          (g_bD4Np2RegisterMap[offset])

#define SNDAMP_BUFFER_LENGTH                        1

#define SNDAMP_DIAG_DRV_POWER_DOWN            0x00
#define SNDAMP_DIAG_DRV_POWER_DOWN_RELEASE    0x01


#define FEATURE_PATH_UPDATE 


#if defined(FEATURE_PATH_UPDATE)
static int amp_close_path(void);
static int amp_set_path(int path);
static int amp_set_cal(int path,D4NP2_SETTING_INFO *amp_info);
static int amp_switch_path(int path,int bOn,D4NP2_SETTING_INFO *amp_info);
#endif


struct yda160_priv {
	struct i2c_client *i2c;

	struct mutex	mutext_lock;
	unsigned char rxdata[2];

    struct yda160_platform_data *pdata;
	D4NP2_SETTING_INFO yda160_amp_info;

    
    
    
    
    
    bool amp_power_on_f;	
#if defined(FEATURE_PATH_UPDATE)
    
    int amp_cur_path; 
    #endif
	unsigned char mode_in_ctl;

	int reset_gpio;
	int spk_pwr_en_gpio;
	int rcv_switch_gpio;	
};


static int yda160_i2c_rxdata(unsigned char *rxdata)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = YDA160_I2C_ADDRESS,
			.flags = 0,
			.len   = 1,
			.buf   = &rxdata[0],
		},
		{
			.addr  = YDA160_I2C_ADDRESS,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = &rxdata[1],
		},
	};
	
	if (i2c_transfer(yda160_i2c_client->adapter, msgs, 2) < 0) {
		pr_err("yda160_i2c_rxdata faild \n");		
		
		return 0;	
	}
	
	return 1;	
	
	
}

static int yda160_i2c_txdata(unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = YDA160_I2C_ADDRESS,
			.flags = 0,
			.len = length,
			.buf = txdata,
		 },
	};

	if (i2c_transfer(yda160_i2c_client->adapter, msg, 1) < 0) {
		pr_err("yda160_i2c_txdata faild \n");
        return 0;	
		
	}

	
	return 1;	
}
















SINT32 d4np2Write(UINT8 bWriteRN, UINT8 bWritePrm)
{
	
	int ret;
	UINT8 buf[2];

	
	
 	buf[0] = bWriteRN|0x80;
	buf[1] = bWritePrm;
	ret = yda160_i2c_txdata(buf, 2);

	return ret;	
}
















SINT32 d4np2WriteN(UINT8 bWriteRN, UINT8 *pbWritePrm, UINT8 bWriteSize)
{
	
	int ret;
	UINT8 *buf;

	

	buf = (UINT8 *)kmalloc(bWriteSize+1,GFP_KERNEL);
	if(!buf)	return -1;

	buf[0] = bWriteRN|0x80;
	memcpy(&buf[1], pbWritePrm, bWriteSize);
	ret = yda160_i2c_txdata(buf, bWriteSize+1);
	kfree(buf);
	
	return ret;
}















SINT32 d4np2Read(UINT8 bReadRN, UINT8 *pbReadPrm)
{
	
	int ret;
	UINT8 buf[2];	
	
	pr_debug("d4np2Read RN:%d \n", bReadRN);
	
	buf[0] = bReadRN|0x80;
	buf[1] = 0;
	
	ret = yda160_i2c_rxdata(buf);
	*pbReadPrm = buf[1];

	pr_debug("ret:%d, 0x%02x \n", ret,  buf[1]);
	
	return ret;	
}













void d4np2Wait(UINT32 dTime)
{
	
	usleep(dTime);
}













void d4np2Sleep(UINT32 dTime)
{
	
	msleep(dTime);
}













void d4np2ErrHandler(int dError)
{
	
	pr_err("d4np2ErrHandler : %d\n", dError);
	
}






































#define DEFAULT_MIN_VOL		    26 
#define DEFALUT_LINE1_VOL		22 
#define DEFAULT_LINE2_VOL		22 





































#define DEFAULT_HP_VOL			18 
#define DEFAULT_SP_VOL			30 

#define AUDIO_DEFAULT_HP_VOL			26 
#define AUDIO_DEFAULT_SP_VOL			29 


#define YDA160_AMP_MAX_VOL	0x1F










static void yda160_amp_init_info(void)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	D4NP2_SETTING_INFO *amp_info = &yda160->yda160_amp_info;
		
	pr_debug("%s start !!!\n", __func__);


	
	
	
	amp_info->bInVolMode = 1;		
 	
	amp_info->bMinVol = DEFAULT_MIN_VOL; 
	
	amp_info->bLine1Vol = DEFALUT_LINE1_VOL;		
	
	amp_info->bLine2Vol = 	DEFAULT_LINE2_VOL; 
	
	
	
	

	
	amp_info->bHpCh = 0;			
	
	amp_info->bHpVolMode = 2;		
	
	amp_info->bHpMixer_Min = 0;	
	
	amp_info->bHpMixer_Line1 = 0;	
	
	amp_info->bHpMixer_Line2 = 0;	
	
	amp_info->bHpVol = DEFAULT_HP_VOL; 
	
	

	
	amp_info->bSpCh = 0;			
	
	amp_info->bSpSwap = 0;		
	
	amp_info->bSpVolMode = 2;		
	
	amp_info->bSpMixer_Min = 0;	
	
	amp_info->bSpMixer_Line1 = 0;	
	
	amp_info->bSpMixer_Line2 = 0;	
	
	amp_info->bSpVol = DEFAULT_SP_VOL;		
	
	
	

	
	amp_info->bSpNonClip = 0;		
	
	amp_info->bPowerLimit = 0;		
	
	amp_info->bDistortion = 4;		
	
	amp_info->bReleaseTime = 3;	
	
	amp_info->bAttackTime = 1;		

	
	amp_info->bRecSW = 1;		




}



#if !defined(FEATURE_PATH_UPDATE)

static void yda160_amp_update_info(void)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	D4NP2_SETTING_INFO *amp_info = &yda160->yda160_amp_info;
		
	pr_debug("%s start !!!\n", __func__);

	D4Np2_PowerOn(amp_info);
}
#endif





















static void yda160_amp_power_off(void)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	
		
	pr_debug("%s start !!!\n", __func__);

	if(yda160->amp_power_on_f != true)
		return;
	
	D4Np2_PowerOff();
	yda160->amp_power_on_f = false;
}


static int yda160_amp_spk_gpio_control(int bOn);
static int yda160_amp_recv_gpio_control(int bOn);





#if defined(FEATURE_PATH_UPDATE)
int yda160_amp_set_path(int path, int bOn)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	
	
	pr_debug("[%s] %d, %d, %d\n", __func__, path, bOn, yda160->amp_power_on_f);

    if(bOn){
      amp_set_path(path);
    }else{
      amp_close_path();
    }

    return 0;

}

#else
int yda160_amp_set_path(int path, int bOn)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	D4NP2_SETTING_INFO *amp_info = &yda160->yda160_amp_info;
	
	pr_debug("%s] %d, %d, %d\n", __func__, path, bOn, yda160->amp_power_on_f);


	switch(path)
	{
	    
		case YDA160_HPMIXER_MIN_PATH:
		case YDA160_SPMIXER_MIN_PATH:
		case YDA160_RECV_OUT_PATH:
			
			amp_info->bHpVol = DEFAULT_HP_VOL; 
			
			amp_info->bSpVol = DEFAULT_SP_VOL;		
			
			amp_info->bSpNonClip = 0;		
			
			amp_info->bPowerLimit = 0;		
		    break;

        
        case YDA160_SPHPMIXER_LRIN2_PATH: 
		case YDA160_HPMIXER_LRIN1_PATH:
		case YDA160_HPMIXER_LRIN2_PATH:
		case YDA160_SPMIXER_LRIN1_PATH:
		case YDA160_SPMIXER_LRIN2_PATH:
						
			amp_info->bHpVol = AUDIO_DEFAULT_HP_VOL; 
			
			amp_info->bSpVol = AUDIO_DEFAULT_SP_VOL; 

 			
			amp_info->bSpNonClip = 1;		
			
			amp_info->bPowerLimit = 0;		
 
		default:
		    break;
	}
    
	switch(path)
	{
		case YDA160_HPMIXER_MIN_PATH:
			if(bOn)
				amp_info->bHpMixer_Min = 1;
			else
				amp_info->bHpMixer_Min = 0;
			break;

		case YDA160_HPMIXER_LRIN1_PATH:
			if(bOn)
				amp_info->bHpMixer_Line1 = 1;
			else
				amp_info->bHpMixer_Line1 = 0;
			break;

		case YDA160_HPMIXER_LRIN2_PATH:
			if(bOn)
				amp_info->bHpMixer_Line2 = 1;
			else
				amp_info->bHpMixer_Line2 = 0;			
			break;

		case YDA160_SPMIXER_MIN_PATH:
			if(bOn)
				amp_info->bSpMixer_Min = 1;
			else
				amp_info->bSpMixer_Min = 0;

			
				
			break;

		case YDA160_SPMIXER_LRIN1_PATH:
			if(bOn)
				amp_info->bSpMixer_Line1 = 1;
			else
				amp_info->bSpMixer_Line1 = 0;
				
			
			break;

		case YDA160_SPMIXER_LRIN2_PATH:
			if(bOn)
				amp_info->bSpMixer_Line2 = 1;
			else
				amp_info->bSpMixer_Line2 = 0;		

			
				
			break;

		case YDA160_RECV_OUT_PATH:
			if(bOn)
				amp_info->bRecSW = 0; 
			else
				amp_info->bRecSW = 1;			

			
			
			break;

        
		case YDA160_SPHPMIXER_LRIN2_PATH:
			if(bOn){
				amp_info->bSpMixer_Line2 = 1;
				amp_info->bHpMixer_Line2 = 1;
			}else{
				amp_info->bSpMixer_Line2 = 0;	
				amp_info->bHpMixer_Line2 = 0;
			}
			break;
		


		case YDA160_OFF_PATH:
			amp_info->bHpMixer_Min = 0;
			amp_info->bHpMixer_Line1 = 0;
			amp_info->bHpMixer_Line2 = 0;
			amp_info->bSpMixer_Min = 0;
			amp_info->bSpMixer_Line1 = 0;
			amp_info->bSpMixer_Line2 = 0;
		    amp_info->bRecSW = 1;            

			
			
		    break;

		default:
			return -2;
	}

    if(bOn)
  	  yda160_amp_update_info();
	else
	  D4Np2_PowerOff();


	return 0;

}
#endif

#if defined(FEATURE_PATH_UPDATE)
static int amp_switch_path(int path,int bOn,D4NP2_SETTING_INFO *amp_info)
{
    pr_debug("[%s] start path %d bOb %d\n", __func__,path,bOn);
    
	switch(path)
	{
		case YDA160_HPMIXER_MIN_PATH:
			if(bOn)
				amp_info->bHpMixer_Min = 1;
			else
				amp_info->bHpMixer_Min = 0;
			break;

		case YDA160_HPMIXER_LRIN1_PATH:
			if(bOn)
				amp_info->bHpMixer_Line1 = 1;
			else
				amp_info->bHpMixer_Line1 = 0;
			break;

		case YDA160_HPMIXER_LRIN2_PATH:
			if(bOn)
				amp_info->bHpMixer_Line2 = 1;
			else
				amp_info->bHpMixer_Line2 = 0;			
			break;

		case YDA160_SPMIXER_MIN_PATH:
			if(bOn)
				amp_info->bSpMixer_Min = 1;
			else
				amp_info->bSpMixer_Min = 0;				

			yda160_amp_spk_gpio_control(bOn); 
			break;

		case YDA160_SPMIXER_LRIN1_PATH:
			if(bOn)
				amp_info->bSpMixer_Line1 = 1;
			else
				amp_info->bSpMixer_Line1 = 0;				

			yda160_amp_spk_gpio_control(bOn); 
			break;

		case YDA160_SPMIXER_LRIN2_PATH:
			if(bOn)
				amp_info->bSpMixer_Line2 = 1;
			else
				amp_info->bSpMixer_Line2 = 0;		

			yda160_amp_spk_gpio_control(bOn); 
			break;

		case YDA160_RECV_OUT_PATH:
			if(bOn)
				amp_info->bRecSW = 0; 
			else
				amp_info->bRecSW = 1;			

			yda160_amp_recv_gpio_control(bOn); 
			
			break;

        
		case YDA160_SPHPMIXER_LRIN2_PATH:
			if(bOn){
				amp_info->bSpMixer_Line2 = 1;
				amp_info->bHpMixer_Line2 = 1;
			}else{
				amp_info->bSpMixer_Line2 = 0;	
				amp_info->bHpMixer_Line2 = 0;
			}

			yda160_amp_recv_gpio_control(0); 
			yda160_amp_spk_gpio_control(bOn); 
			break;
		

		case YDA160_OFF_PATH:









			yda160_amp_recv_gpio_control(0); 
			yda160_amp_spk_gpio_control(0); 
		    break;

		default:
			break;
	}


	return 0;

}

static int amp_set_cal(int path,D4NP2_SETTING_INFO *amp_info)
{
	switch(path)
	{
	    
		case YDA160_HPMIXER_MIN_PATH:
		case YDA160_SPMIXER_MIN_PATH:
		case YDA160_RECV_OUT_PATH:
			
			amp_info->bHpVol = DEFAULT_HP_VOL; 
			
			amp_info->bSpVol = DEFAULT_SP_VOL;		
			
			amp_info->bSpNonClip = 0;		
			
			amp_info->bPowerLimit = 0;		
		    break;

        
        case YDA160_SPHPMIXER_LRIN2_PATH: 
		case YDA160_HPMIXER_LRIN1_PATH:
		case YDA160_HPMIXER_LRIN2_PATH:
		case YDA160_SPMIXER_LRIN1_PATH:
		case YDA160_SPMIXER_LRIN2_PATH:
						
			amp_info->bHpVol = AUDIO_DEFAULT_HP_VOL; 
			
			amp_info->bSpVol = AUDIO_DEFAULT_SP_VOL; 

 			
			amp_info->bSpNonClip = 1;		
			
			amp_info->bPowerLimit = 0;		
 
		default:
		    break;
	}

    return 0;

}

static int amp_set_path(int path)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	D4NP2_SETTING_INFO *amp_info = &yda160->yda160_amp_info;
	int amp_cur_path;
	
	amp_cur_path = yda160->amp_cur_path;

	pr_debug("[%s] start path %d amp_cur_path %d\n", __func__,path,amp_cur_path);


	if(amp_cur_path == path){
	   pr_debug("same path %d",amp_cur_path);
	   return 0;
	}

	
	
	
	

	
	amp_set_cal(path,amp_info);

	
    amp_switch_path(amp_cur_path,0,amp_info);
    
	
    amp_switch_path(path,1,amp_info);
	D4Np2_PowerOn(amp_info);
    yda160->amp_cur_path = path;

	return 0;
}

static int amp_close_path(void)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	D4NP2_SETTING_INFO *amp_info = &yda160->yda160_amp_info;
	int amp_cur_path;
	
	pr_debug("[%s] start\n", __func__);

	amp_cur_path = yda160->amp_cur_path;	

	
	
	
	
	switch(amp_cur_path)
	{
		case YDA160_HPMIXER_MIN_PATH:
		case YDA160_HPMIXER_LRIN1_PATH:
		case YDA160_HPMIXER_LRIN2_PATH:
		case YDA160_SPMIXER_MIN_PATH:
		case YDA160_SPMIXER_LRIN1_PATH:
		case YDA160_SPMIXER_LRIN2_PATH:
		case YDA160_RECV_OUT_PATH:
		case YDA160_SPHPMIXER_LRIN2_PATH:
			
			amp_switch_path(amp_cur_path,0,amp_info);	 
			D4Np2_PowerOn(amp_info);
			
			
			yda160->amp_cur_path = YDA160_OFF_PATH;
		    break;

		default:
			break;
	}

	return 0;
}
#endif



EXPORT_SYMBOL_GPL(yda160_amp_set_path);


int yda160_get_amp_cur_path(void) {
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	int amp_cur_path;

	amp_cur_path = yda160->amp_cur_path;	
	pr_debug("[%s]amp_cur_path=%d\n", __func__, amp_cur_path);

	return amp_cur_path;
}
EXPORT_SYMBOL_GPL(yda160_get_amp_cur_path);


static void yda160_reset_control(int ctl)
{
  struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);

  pr_debug("%s start reset_gpio %d ctl %d!!!\n", __func__,yda160->reset_gpio,ctl);

  if(ctl==1){
    gpio_set_value_cansleep(yda160->reset_gpio, 1);  
  }
  else{
    gpio_set_value_cansleep(yda160->reset_gpio, 0);  
  }  	
}

static int yda160_amp_spk_gpio_control(int bOn)
{

	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	
	pr_debug("%s start gpio %d ctl %d!!!\n", __func__,yda160->spk_pwr_en_gpio,bOn);

	if(bOn==1){
	  gpio_set_value_cansleep(yda160->spk_pwr_en_gpio, 1);  
	}
	else{
	  gpio_set_value_cansleep(yda160->spk_pwr_en_gpio, 0);  
	}	  
	
	return 0;
}

static int yda160_amp_recv_gpio_control(int bOn)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
		
	pr_debug("%s start gpio %d ctl %d!!!\n", __func__,yda160->rcv_switch_gpio,bOn);

	if(bOn==1){
	  gpio_set_value_cansleep(yda160->rcv_switch_gpio, 1);	
	}
	else{
	  gpio_set_value_cansleep(yda160->rcv_switch_gpio, 0);	
	}	  
	
    return 0;
}


static int gg3_yda160_amp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	
	pr_debug("%s() event %d voice/audio %d\n", __func__, SND_SOC_DAPM_EVENT_ON(event),yda160->mode_in_ctl);
	pr_debug("%s() yda160_mode_control %d\n", __func__, yda160_mode_control);

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if(!strncmp(w->name, "YDA160 Amp Spk", 14)) {			
			if(yda160_mode_control == MODE_HP_OUT){
			    
				yda160_amp_set_path(YDA160_HPMIXER_LRIN2_PATH, 1);				
			}
			else if(yda160_mode_control == MODE_SP_OUT){
			    
 				yda160_amp_set_path(YDA160_SPMIXER_LRIN2_PATH, 1);			
			}
			
			else if(yda160_mode_control == MODE_SP_HP_OUT){
			    
 				yda160_amp_set_path(YDA160_SPHPMIXER_LRIN2_PATH, 1);			
			}			
			
		}
		else {
			pr_err("%s() Invalid Amp Widget = %s\n",__func__, w->name);
			return -EINVAL;
		}
		
	} else if(SND_SOC_DAPM_EVENT_OFF(event)){
		if(!strncmp(w->name, "YDA160 Amp Spk", 14)) {
			if(yda160_mode_control == MODE_HP_OUT){
				yda160_amp_set_path(YDA160_HPMIXER_LRIN2_PATH, 0);				
			}
			else if(yda160_mode_control == MODE_SP_OUT){
 				yda160_amp_set_path(YDA160_SPMIXER_LRIN2_PATH, 0);			
			}
			
			else if(yda160_mode_control == MODE_SP_HP_OUT){
			    yda160_amp_set_path(YDA160_SPHPMIXER_LRIN2_PATH, 0);			
			}			
			
		}
		else {
			pr_err("%s() Invalid Amp Widget = %s\n",__func__, w->name);
			return -EINVAL;
		}
	}
	return 0;
}


static int gg3_yda160_voice_amp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	
	pr_debug("%s() event %d voice/audio %d\n", __func__, SND_SOC_DAPM_EVENT_ON(event),yda160->mode_in_ctl);
	pr_debug("%s() yda160_mode_control %d\n", __func__, yda160_mode_control);

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if(!strncmp(w->name, "ES310 voice", 11)) {			
			if(yda160_mode_control == MODE_RECV_OUT){
				yda160_amp_set_path(YDA160_RECV_OUT_PATH, 1);				
			}
			else if(yda160_mode_control == MODE_VIOCE_HP_OUT){
 				yda160_amp_set_path(YDA160_HPMIXER_MIN_PATH, 1);			
			}
			else if(yda160_mode_control == MODE_VIOCE_SP_OUT){
 				yda160_amp_set_path(YDA160_SPMIXER_MIN_PATH, 1);			
			}			
		}
		else {
			pr_err("%s() Invalid Amp Widget = %s\n",__func__, w->name);
			return -EINVAL;
		}
		
	} else if(SND_SOC_DAPM_EVENT_OFF(event)){
		if(!strncmp(w->name, "ES310 voice", 11)) {			
			if(yda160_mode_control == MODE_RECV_OUT){
				yda160_amp_set_path(YDA160_RECV_OUT_PATH, 0);				
			}
			else if(yda160_mode_control == MODE_VIOCE_HP_OUT){
 				yda160_amp_set_path(YDA160_HPMIXER_MIN_PATH, 0);			
			}
			else if(yda160_mode_control == MODE_VIOCE_SP_OUT){
 				yda160_amp_set_path(YDA160_SPMIXER_MIN_PATH, 0);			
			}			
		}
		else {
			pr_err("%s() Invalid Amp Widget = %s\n",__func__, w->name);
			return -EINVAL;
		}
	}
	return 0;
}




static int yda160_get_mode_control(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: mode_control = %d", __func__, yda160_mode_control);
	ucontrol->value.integer.value[0] = yda160_mode_control;
	return 0;
}

static int yda160_set_mode_control(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
	

	

	yda160_mode_control = ucontrol->value.integer.value[0];

	pr_debug("%s() mode %d\n", __func__,yda160_mode_control);

    yda160->mode_in_ctl = MODE_IN_AUDIO;	
	
	
	return 1;
}



static const struct snd_soc_dapm_widget yda160_dapm_widgets[] = {
	SND_SOC_DAPM_SPK("YDA160 Amp Spk", gg3_yda160_amp_event), 
	
	SND_SOC_DAPM_SPK("ES310 voice", gg3_yda160_voice_amp_event), 
};

static const struct snd_soc_dapm_route yda160_audio_map[] = {
	{"YDA160 Amp Spk", NULL, "LINEOUT2"},
	
	{"ES310 voice", NULL, "ES310 Amp"}, 
};

























static const char *yda160_mode_ctl[] = {
  "off",
  "headset",		  
  "speaker", 	  
  "receiver", 	  
  "sp_headset",
  "voice_headset", 
  "voice_speaker", 
}; 





static const struct soc_enum yda160_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(yda160_mode_ctl),yda160_mode_ctl), 
};

static const struct snd_kcontrol_new yda160_controls[] = {
	SOC_ENUM_EXT("yda160 mode control", yda160_enum[0], yda160_get_mode_control, 
		yda160_set_mode_control),
};





int yda160_add_controls(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int ret;

	if (!yda160_i2c_client) {
		pr_err("ES310 not yet probed\n");
		return -ENODEV;
	}

	ret = snd_soc_dapm_new_controls(dapm, yda160_dapm_widgets,
					ARRAY_SIZE(yda160_dapm_widgets));
	if (ret < 0)
		return ret;

	ret = snd_soc_dapm_add_routes(dapm, yda160_audio_map, ARRAY_SIZE(yda160_audio_map));
	if (ret < 0)
		return ret;

	return snd_soc_add_codec_controls(codec, yda160_controls,
			ARRAY_SIZE(yda160_controls));





}
EXPORT_SYMBOL_GPL(yda160_add_controls);




static int __devinit yda160_i2c_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
    
	struct yda160_priv *yda160; 
	struct yda160_platform_data *pdata; 
	unsigned char txdata[2];
	int ret;

    
	if (client->dev.platform_data == NULL) {
		
		
		return -ENODEV;
	}
	
	if (yda160_i2c_client) {
		pr_debug("%s Another es310 is already registered\n", __func__);
		return -EINVAL;
	}

    
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_debug("%s i2c_check_functionality failed\n", __func__);
		return -EINVAL;
	}
	
	pr_debug("%s i2c_check_functionality success\n", __func__);

	yda160_i2c_client = client;

    
	yda160 = kzalloc(sizeof(struct yda160_priv), GFP_KERNEL);
	if (yda160 == NULL) {
		pr_debug("%s Unable to allocate private data\n", __func__);
		return -ENOMEM;
	}
    i2c_set_clientdata(yda160_i2c_client, yda160);
	
	
	pdata = client->dev.platform_data;
	yda160->pdata = pdata;		
	yda160->reset_gpio = pdata->reset_gpio;
	yda160->spk_pwr_en_gpio = pdata->spk_pwr_en_gpio;
	yda160->rcv_switch_gpio = pdata->rcv_switch_gpio;


    

	
	ret = pdata->dev_setup(1);
	if(ret != 0)
	{
		pr_debug("%s Unable to setup gpio/power\n", __func__);	
		goto err_setup;
	}	
	pdata->power_on(1);

	
 	yda160_reset_control(1); 
	msleep(10);
	yda160_reset_control(0); 
	msleep(10);
	yda160_reset_control(1); 	
	msleep(20);	

	
	txdata[0] = 0x80;
	txdata[1] = 0x00;	
	yda160_i2c_txdata(txdata, 2);

    
    yda160_amp_init_info();

	yda160->mode_in_ctl = MODE_IN_OFF;

#if defined(FEATURE_PATH_UPDATE)
	yda160->amp_power_on_f = false; 
	yda160->amp_cur_path = YDA160_OFF_PATH; 

	#else
	yda160->amp_power_on_f = true; 
	#endif

	
	
 	return 0;

err_setup:
	kfree(yda160);
	i2c_set_clientdata(yda160_i2c_client, NULL);
	yda160_i2c_client = NULL;
	
	return ret;
	
}

static int __devexit yda160_i2c_remove(struct i2c_client *client)
{
	struct yda160_priv *yda160 = i2c_get_clientdata(client);

	pr_debug("%s !!!\n", __func__);

	yda160_amp_power_off();
	
	if(yda160->pdata->power_on)
		yda160->pdata->power_on(0);
	
	return 0;
}



static int  yda160_i2c_suspend(struct i2c_client *client, pm_message_t message)
{
	pr_debug("%s start !!!\n", __func__);
	
	

	yda160_reset_control(0); 
	return 0;
}

static int  yda160_i2c_resume(struct i2c_client *client)
{
	pr_debug("%s start !!!\n", __func__);
	
	

    
	
	yda160_reset_control(1); 
	
	return 0;
}








static const struct i2c_device_id yda160_i2c_id[] = {
	{ "yamaha_yda160", 0 },
	{ }
};
 
static struct i2c_driver yda160_i2c_driver = {
	.driver = {
		.name = "yamaha_yda160",
		.owner = THIS_MODULE,			
	},
	.probe    = yda160_i2c_probe,
	.remove   = __devexit_p(yda160_i2c_remove),
	.suspend  = yda160_i2c_suspend,
	.resume	  = yda160_i2c_resume,
	.id_table = yda160_i2c_id,
};


static int yda160_diag_dev_open(struct inode *inode, struct file *file)
{
  pr_debug("%s - june\n", __func__);
  return 0;
}

static int yda160_diag_dev_release(struct inode *inode, struct file *file)
{
  pr_debug("%s - june\n", __func__);
  return 0;
}

int sndamp_yda160_WriteReg(sndamp_yda160_register_type r)
{
  int ret = 0;

  if( d4np2Write( (UINT8)SNDAMP_ADDR(r), (UINT8)SNDAMP_BUF(r) ) ) 
  {
    printk( KERN_INFO "[yda160] Write REG : ADDR=0x%x DATA=0x%x \n", SNDAMP_ADDR(r), SNDAMP_BUF(r) );

    ret = 0; 
  }
  else
  {
    printk( KERN_ERR "[yda160] yda160_WriteReg ERR \n" );

    ret = 1;
  }

  return ret;
}


int sndamp_yda160_ReadReg(sndamp_yda160_register_type r)
{
    int                    ret = 0;
    unsigned char          read_buf;
    int                    i2c_ret;

    pr_debug("%s : r=%d - june\n", __func__, r);
    i2c_ret = d4np2Read( (u8)(SNDAMP_ADDR(r)), (u8 *)&(read_buf) );
    pr_debug("read_buf = 0x%02X - june\n", read_buf);

    if(i2c_ret)
    {

        memcpy( (void *)&SNDAMP_BUF(r), (void*)&read_buf, SNDAMP_BUFFER_LENGTH );
        printk( KERN_INFO "[yda160] Read REG : ADDR=0x%x DATA=0x%x \n",
                SNDAMP_ADDR(r),
                SNDAMP_BUF(r) );

        ret =  0;
    }else{


        printk( KERN_ERR "[yda160] yda160_ReadReg ERR \n" );

        ret = 1;
    }


    return ret;

} 

int sndamp_read_reg_diag(unsigned char addrReg, unsigned char* buf)
{

    int result = 0;


    sndamp_yda160_register_type reg_e = addrReg - SNDAMP_ADDR_BASE;


    result = sndamp_yda160_ReadReg( reg_e );

    pr_debug("%s - june\n", __func__);
    if(result == 0)
    {

        *buf = SNDAMP_BUF( reg_e );
    }
    else
    {

        *buf = 0xFF;
    }

    pr_debug("*buf = 0x%02X - june\n", *buf);
    return result;

} 

int sndamp_write_reg_diag(unsigned char addrReg, unsigned char buf)
{

    int result = 0;


    sndamp_yda160_register_type reg_e = addrReg - SNDAMP_ADDR_BASE;


    SNDAMP_BUF(reg_e) = buf;


    result = sndamp_yda160_WriteReg( reg_e );

    return result;

} 

void sndamp_Drive_Control( sndamp_drive_ctrl_type  *ctrl )
{

    if(ctrl != NULL)
    {
        printk( KERN_INFO "[yda160] sndamp_Drive_Control() ctrl != NULL \n" );

        printk( KERN_INFO "[yda160] ctrl->vol_id=%d  /  ctrl->drv_ctl=%d - june\n", ctrl->vol_id, ctrl->drv_ctl);
      
      if(ctrl->vol_id < 10) 
        {
            switch( ctrl->drv_ctl )
            {

                case SNDAMP_DIAG_DRV_POWER_DOWN:
                    
                    D4Np2_PowerOff();
                    break;

                case SNDAMP_DIAG_DRV_POWER_DOWN_RELEASE:
                    
                    
                    
                    yda160_amp_set_path(ctrl->vol_id,1);
                    break;

                default:
                    printk( KERN_ERR "[yda160] sndamp_Drive_Control() drive parameter ERROR : 0x%x \n", ctrl->drv_ctl );
                    break;
            }

        }
        else
        {
            printk( KERN_ERR "[yda160] sndamp_Drive_Control() volume id ERROR : %d \n", ctrl->vol_id );
        }
    }
    
    else
    {
        printk( KERN_ERR "[yda160] sndamp_Drive_Control()  ERROR : ctrl == NULL \n" );
    }

    return;

} 


static long yda160_diag_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    sndamp_diag_ctrl_type  diag_ctrl;
    yda160_reg_rw yda_packet;
    memset(&yda_packet, 0, sizeof(yda160_reg_rw));

    pr_debug("%s - june\n", __func__);

    if(arg!=0)
    {
        if ( copy_from_user( &diag_ctrl, (void *)arg, sizeof(diag_ctrl) ) )
        {
            printk( KERN_ERR "[yda160] I/O Control : copy_from_user ERROR \n" );
            return -EFAULT;
        }
    }
  
    mutex_lock(&yda160_diag_dev_lock);
    switch(cmd)
    {
    case YDA160_GET_ID :
        pr_debug("%s : YDA160_GET_ID - june\n", __func__);
        ret = 1;
        break;

    case SNDAMP_IOCTL_REGISTER_READ:
        printk( KERN_INFO "[yda160] I/O Control : Cmd = SNDAMP_IOCTL_REGISTER_READ \n" );
        pr_debug("%s : SNDAMP_IOCTL_REGISTER_READ - june\n", __func__);

        sndamp_read_reg_diag( diag_ctrl.reg.reg_addr, (unsigned char *)&(diag_ctrl.reg.reg_data) );

        break;
            
    case SNDAMP_IOCTL_REGISTER_WRITE:
        printk( KERN_INFO "[yda160] I/O Control : Cmd = SNDAMP_IOCTL_REGISTER_WRITE \n" );

        sndamp_write_reg_diag( diag_ctrl.reg.reg_addr, diag_ctrl.reg.reg_data );
        break;

    case SNDAMP_IOCTL_DRIVE:
        printk( KERN_INFO "[yda160] I/O Control : Cmd = SNDAMP_IOCTL_DRIVE \n" );

        sndamp_Drive_Control( &diag_ctrl.drv );
        break;

	
	case SNDAMP_IOCTL_SETPATH:
        printk( KERN_INFO "[yda160] I/O Control : Cmd = SNDAMP_IOCTL_SETPATH path=%d.\n", diag_ctrl.path  );

        amp_set_path( diag_ctrl.path );
        break;
	

	
	case SNDAMP_IOCTL_SPEAKER_LR_ON_OFF:
	{
		struct yda160_priv *yda160 = i2c_get_clientdata(yda160_i2c_client);
		D4NP2_SETTING_INFO *amp_info = &yda160->yda160_amp_info;
		int bOnLch = 0;
		int bOnRch = 0;

		if(diag_ctrl.speaker_onoff & 0xF0)
			bOnLch = 1;
		if(diag_ctrl.speaker_onoff & 0x0F)
			bOnRch = 1;
		
		printk( KERN_INFO "[yda160] I/O Control : Cmd = SNDAMP_IOCTL_SPEAKER_LR_ON_OFF path=%x, bOnLch=%d, bOnRch=%d.\n", diag_ctrl.speaker_onoff, bOnLch, bOnRch  );

		yda160_amp_set_path(YDA160_SPMIXER_LRIN2_PATH, 1); 
		D4Np2_PowerOn_SpOut_LRch(amp_info, bOnLch, bOnRch);

		break;
	}
	
	
    default :
        ret = 0;
        break;
    }
    mutex_unlock(&yda160_diag_dev_lock);
	return ret;
}

static const struct file_operations yda160_fops = {
  .owner    = THIS_MODULE,
  .open     = yda160_diag_dev_open,
  .release  = yda160_diag_dev_release,
  .unlocked_ioctl = yda160_diag_dev_ioctl
};

struct miscdevice yda160_dev = {
  .minor  = MISC_DYNAMIC_MINOR,
  .name   = "yda160_diag_dev",
  .fops   = &yda160_fops,
};


static __init int yda160_i2c_init(void)
{
  misc_register(&yda160_dev);	
	return i2c_add_driver(&yda160_i2c_driver);
}

static __exit void yda160_i2c_exit(void)
{
	i2c_del_driver(&yda160_i2c_driver);
}


module_init(yda160_i2c_init);
module_exit(yda160_i2c_exit);

MODULE_AUTHOR("Dana.Park@m7system.com");
MODULE_DESCRIPTION("I2C bus driver for DVE068 External Audio Devices");
MODULE_LICENSE("GPL v2");

