/* drivers/video/backlight/lm3530.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <linux/earlysuspend.h>
#include <mach/board_gg3.h>
#include <mach/gpio.h>

#define LM3530_MAX_LEVEL		0x7f
#define LM3530_MIN_LEVEL 		0x0f
#define LM3530_DEFAULT_LEVEL	0x33

#define I2C_BL_NAME "lm3530"

#define LM3530_BL_ON	1
#define LM3530_BL_OFF	0

static struct i2c_client *lm3530_i2c_client;

struct backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;
	int max_current;
	int init_on_boot;
	int min_brightness;
	int max_brightness;
};

struct lm3530_device {
	struct i2c_client *client;
	struct backlight_device *bl_dev;
	int gpio;
	int max_current;
	int min_brightness;
	int max_brightness;
	struct mutex bl_mutex;
};

static const struct i2c_device_id lm3530_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};

static int lm3530_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val);

static int cur_main_lcd_level;
static int saved_main_lcd_level;
static int backlight_status = LM3530_BL_OFF;
static int backlight_poweroff_charging = 0; 

static struct lm3530_device *main_lm3530_dev;





static struct early_suspend * h;

static int lcdonbooting = 0; 

static void lm3530_hw_reset(void)
{
	int gpio = main_lm3530_dev->gpio;
 
	if(!lcdonbooting){ 
		gpio_direction_output(gpio, 0);
		gpio_set_value_cansleep(gpio, 0);
	}
	else
	{
		lcdonbooting = 1;
	}
 
		if (gpio_is_valid(gpio)) {
		gpio_direction_output(gpio, 1);
		gpio_set_value_cansleep(gpio, 1);
		
		mdelay(2);
	}
}

static int lm3530_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val)
{
	u8 buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};
	int ret;

	buf[0] = reg;
	buf[1] = val;

	if ((ret=i2c_transfer(client->adapter, &msg, 1)) < 0)
		dev_err(&client->dev, "i2c write error ret = %0x\n", -ret);

	return ret;
}
void lm3530_backlight_on(int level)
{

	if (backlight_status == LM3530_BL_OFF) {
		lm3530_hw_reset();

		

		lm3530_write_reg(main_lm3530_dev->client, 0x10, 0xB5);

	    	lm3530_write_reg(main_lm3530_dev->client, 0x30, 0x09);   
	}

	printk("%s received (prev backlight_status: %s)\n",  __func__, backlight_status?"ON":"OFF");
	
	backlight_status = LM3530_BL_ON;

	return;
}

void lm3530_lcd_backlight_set_level2(int level)
{
	if (level > LM3530_MAX_LEVEL)
		level = LM3530_MAX_LEVEL;

	lm3530_write_reg(main_lm3530_dev->client, 0xA0, level); 

	msleep(130); 
}
EXPORT_SYMBOL(lm3530_lcd_backlight_set_level2);


void lm3530_backlight_off(struct early_suspend * h)
{
	int gpio = main_lm3530_dev->gpio;
	
	printk("%s, backlight_status : %d\n",__func__,backlight_status);

	if (backlight_status == LM3530_BL_OFF)
		return;
	saved_main_lcd_level = cur_main_lcd_level;

	backlight_status = LM3530_BL_OFF;

	if(!backlight_poweroff_charging) 
	{
		lm3530_lcd_backlight_set_level2(0x00); 
	}else{
		backlight_poweroff_charging = 0;
	}
	
	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(gpio, 0);
	msleep(6);
	return;
}
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

void lm3530_lcd_backlight_enable(int level)
{
	int ret=0;
	
	printk("%s, backlight_status : %d\n",__func__,backlight_status);
	
	if (backlight_status == LM3530_BL_OFF) {

		ret = lm3530_write_reg(main_lm3530_dev->client, 0x10, 0xB5);
		ret |= lm3530_write_reg(main_lm3530_dev->client, 0x30, 0x09);	 
	}
	ret |= lm3530_write_reg(main_lm3530_dev->client, 0xA0, level); 

	
	
	backlight_status = LM3530_BL_ON;

	return;
}

void lm3530_lcd_backlight_disable(int level)
{
	int ret;
	
	printk("%s, backlight_status : %d\n",__func__,backlight_status);
	
	if (backlight_status == LM3530_BL_OFF)
		return;
	
	backlight_status = LM3530_BL_OFF;

	ret = lm3530_write_reg(main_lm3530_dev->client, 0xA0, level); 
	

	return;	
}

int get_backlight_status(void)
{
	return backlight_status;
}

void lm3530_lcd_backlight_set_level(int level)
{

	
		
		
	if(level == 0xFF)
	{
		if (backlight_status == LM3530_BL_OFF)
		return;

		backlight_poweroff_charging = 1;
		lm3530_backlight_off(h);
	}
	else
	{
		backlight_poweroff_charging = 0; 

		if (lm3530_i2c_client != NULL) 
		{
			if (level == 0)
				lm3530_backlight_off(h);
			else
				lm3530_backlight_on(level);
			}
			else
			{
			printk(KERN_INFO "%s(): No client\n", __func__);
		}

	}
}
EXPORT_SYMBOL(lm3530_lcd_backlight_set_level);

static int bl_set_intensity(struct backlight_device *bd)
{	
	printk("%s, backlight_status : brightness[%d] status[%d]\n",__func__, bd->props.brightness, backlight_status);

	backlight_status = LM3530_BL_OFF;

	
	
	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
    return cur_main_lcd_level;
}

static struct backlight_ops lm3530_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int __devinit lm3530_probe(struct i2c_client *i2c_dev,
		const struct i2c_device_id *id)
{
	struct backlight_platform_data *pdata;
	struct lm3530_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;




	printk("%s: i2c probe start\n", __func__);

	pdata = i2c_dev->dev.platform_data;
	lm3530_i2c_client = i2c_dev;

	dev = kzalloc(sizeof(struct lm3530_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for lm3530_device\n");
		return 0;
	}

	main_lm3530_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = LM3530_MAX_LEVEL;

	bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev, NULL,
			&lm3530_bl_ops, &props);
	bl_dev->props.max_brightness = LM3530_MAX_LEVEL;
	bl_dev->props.brightness = LM3530_DEFAULT_LEVEL;
	bl_dev->props.power = FB_BLANK_UNBLANK;

	dev->bl_dev = bl_dev;
	dev->client = i2c_dev;
	dev->gpio = pdata->gpio;
	dev->max_current = pdata->max_current;
	dev->min_brightness = pdata->min_brightness;
	dev->max_brightness = pdata->max_brightness;
	i2c_set_clientdata(i2c_dev, dev);

	if (dev->gpio && gpio_request(dev->gpio, "lm3530 reset") != 0)
		return -ENODEV;

	mutex_init(&dev->bl_mutex);






	return 0;
}

static int __devexit lm3530_remove(struct i2c_client *i2c_dev)
{
	struct lm3530_device *dev;
	int gpio = main_lm3530_dev->gpio;

	dev = (struct lm3530_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(gpio, 0);
	
	printk("%s, backlight_status : %d\n",__func__,gpio_get_value(gpio));


	if (gpio_is_valid(gpio))
		gpio_free(gpio);
	return 0;
}

static struct i2c_driver main_lm3530_driver = {
	.probe = lm3530_probe,
	.remove = lm3530_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = lm3530_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
	},
};


static int __init lcd_backlight_init(void)
{
	static int err;

	err = i2c_add_driver(&main_lm3530_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("LM3530 Backlight Control");
MODULE_AUTHOR(" kevin <kevin.jeong@beyless.com>");
MODULE_LICENSE("GPL");
