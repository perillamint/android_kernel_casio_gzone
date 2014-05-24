/**********************************************************************
* File Name: sound/soc/codecs/yda160.h
* 
* (C) NEC CASIO Mobile Communications, Ltd. 2013
**********************************************************************/
#if !defined(__YDA160_H__)
#define __YDA160_H__















enum{
	YDA160_HPMIXER_MIN_PATH,		
	YDA160_HPMIXER_LRIN1_PATH,		
	YDA160_HPMIXER_LRIN2_PATH,		
	YDA160_SPMIXER_MIN_PATH,		
	YDA160_SPMIXER_LRIN1_PATH,		
	YDA160_SPMIXER_LRIN2_PATH,		
	YDA160_RECV_OUT_PATH,
	YDA160_SPHPMIXER_LRIN2_PATH,
	YDA160_OFF_PATH,
};

typedef struct _YDA160_REG_RW
{
  unsigned char addr;
  unsigned char data[4];
}yda160_reg_rw; 

#define  SNDAMP_ADDR_BASE  0x80 


typedef enum {
  SNDAMP_REG_0_POWERDOWN1,
  SNDAMP_REG_1_POWERDOWN2,
  SNDAMP_REG_2_POWERDOWN3,
  SNDAMP_REG_3_POWERDOWN4,
  SNDAMP_REG_4_NONCLIP2,
  SNDAMP_REG_5_ATTACK_RELEASE_TIME,
  SNDAMP_REG_6_RESERVED,
  SNDAMP_REG_7_MONAURAL,
  SNDAMP_REG_8_LINE1_LCH,
  SNDAMP_REG_9_LINE1_RCH,
  SNDAMP_REG_10_LINE2_LCH,
  SNDAMP_REG_11_LINE2_RCH,
  SNDAMP_REG_12_HEADPHONE_MIXER,
  SNDAMP_REG_13_SPEAKER_MIXER,
  SNDAMP_REG_14_SPEAKER_ATT,
  SNDAMP_REG_15_HEADPHONE_ATT_LCH,
  SNDAMP_REG_16_HEADPHONE_ATT_RCH,
  SNDAMP_REG_17_ERROR_FLAG,
  SNDAMP_REG_MAX,
} sndamp_yda160_register_type;


extern int yda160_amp_set_path(int path, int bOn);
extern int yda160_add_controls(struct snd_soc_codec *codec); 


#endif 
