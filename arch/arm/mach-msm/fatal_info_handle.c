/*
 * Copyright (c) 2012 M7System Co., Ltd.

 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/setup.h>
#include <linux/reboot.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/ptrace.h>
#include <mach/board_gg3.h>
#include <mach/subsystem_restart.h>


struct kernel_panic_log {
	unsigned int size;
	unsigned char buffer[0];
};


struct fatal_cpu_regs {
	
	unsigned long uregs[17];
};

struct fatal_mmu_regs {
	
	unsigned long cp15_sctlr;
	unsigned long cp15_ttb0;
	unsigned long cp15_ttb1;
	unsigned long cp15_dacr;	
	
	

};


struct fatal_cpu_mmu_regs {
	unsigned int online_cpu;
	struct fatal_cpu_regs cpu_reg[2];

	struct fatal_mmu_regs mmu_reg[2];		
};
	




static struct kernel_panic_log *kernel_panic_log_buf;
static unsigned int kernel_panic_log_buf_size = 0;
static int enable_save_log = 0;

static struct fatal_cpu_mmu_regs  fatal_cpu_mmu_reg_dump;

extern void set_kernel_panic_magic_num(void);


static int dont_save_panic_log(struct notifier_block *this, unsigned long event,
		void *ptr)
{

	enable_save_log = 0;

	return NOTIFY_DONE;
}

static struct notifier_block panic_handler_block = {
	.notifier_call  = dont_save_panic_log,
};



void set_kernel_panic_log(int enable)
{
	enable_save_log = enable;

	return;
}


void save_kernel_panic_log(char *p)
{
	if (!enable_save_log)
		return;

	if (kernel_panic_log_buf->size >= (kernel_panic_log_buf_size-100))
		return;
	

	for ( ; *p; p++) {
		if (*p == '[') {
			for ( ; *p != ']'; p++)
				;
			p++;
			if (*p == ' ')
				p++;
		}

		if (*p == '<') {
			for ( ; *p != '>'; p++)
				;
			p++;
		}


		kernel_panic_log_buf->buffer[kernel_panic_log_buf->size] = *p;
		kernel_panic_log_buf->size++;

		if (kernel_panic_log_buf->size >= (kernel_panic_log_buf_size-100))
		{
			kernel_panic_log_buf->buffer[kernel_panic_log_buf->size] = 0;
			return;
		}
				

	}
	kernel_panic_log_buf->buffer[kernel_panic_log_buf->size] = 0;

	return;
}




void save_cpu_mmu_register_dump(struct pt_regs* regs, unsigned int sel_cpu, struct fatal_mmu_regs mmu_regs )

{
	fatal_cpu_mmu_reg_dump.online_cpu=sel_cpu;

	
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[0] = regs->ARM_r0;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[1] = regs->ARM_r1;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[2] = regs->ARM_r2;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[3] = regs->ARM_r3;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[4] = regs->ARM_r4;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[5] = regs->ARM_r5;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[6] = regs->ARM_r6;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[7] = regs->ARM_r7;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[8] = regs->ARM_r8;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[9] = regs->ARM_r9;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[10] = regs->ARM_r10;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[11] = regs->ARM_fp;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[12] = regs->ARM_ip;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[13] = regs->ARM_sp;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[14] = regs->ARM_lr;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[15] = regs->ARM_pc;
	fatal_cpu_mmu_reg_dump.cpu_reg[sel_cpu].uregs[16] = regs->ARM_cpsr;
	
	fatal_cpu_mmu_reg_dump.mmu_reg[sel_cpu].cp15_sctlr = mmu_regs.cp15_sctlr;
	fatal_cpu_mmu_reg_dump.mmu_reg[sel_cpu].cp15_ttb0 = mmu_regs.cp15_ttb0;
	fatal_cpu_mmu_reg_dump.mmu_reg[sel_cpu].cp15_ttb1= mmu_regs.cp15_ttb1;
	fatal_cpu_mmu_reg_dump.mmu_reg[sel_cpu].cp15_dacr= mmu_regs.cp15_dacr;
	
	

}


static int add_panic_notifier(void)
{
	atomic_notifier_chain_register(&panic_notifier_list, &panic_handler_block);
	return 0;
}

static int __init fatal_info_handler_probe(struct platform_device *pdev)
{
	struct resource *res = pdev->resource;
	size_t start;
	size_t buffer_size;
	void *buffer;
	
	
	int ret = 0;


	if (res == NULL) {
		return -ENXIO;
	}

	buffer_size = res->end - res->start + 1;
	start = res->start;

	printk(KERN_INFO "fatal_info_handler: got buffer  %zx, size %zx\n",
			start, buffer_size);

	buffer = ioremap(res->start, buffer_size);

	if (buffer == NULL) {
		printk(KERN_ERR "fatal_info_handler: failed to map memory\n");
		return -ENOMEM;
	}

	kernel_panic_log_buf = (struct kernel_panic_log *)buffer;
	memset(kernel_panic_log_buf, 0, buffer_size);

	kernel_panic_log_buf_size = buffer_size - offsetof(struct kernel_panic_log, buffer);











	
	add_panic_notifier();

	return ret;
}

static int __devexit fatal_info_handler_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver fatal_info_handler_driver __refdata = {

	.driver = {
		.name = "fatal-info-handler",
		.owner = THIS_MODULE,
	},
	.probe = fatal_info_handler_probe,
	.remove = __devexit_p(fatal_info_handler_remove),
};

static int __init fatal_info_handler_init(void)
{
	return platform_driver_register(&fatal_info_handler_driver);
}

static void __exit fatal_info_handler_exit(void)
{
	platform_driver_unregister(&fatal_info_handler_driver);
}

module_init(fatal_info_handler_init);
module_exit(fatal_info_handler_exit);

MODULE_DESCRIPTION("FATAL INFO");
MODULE_AUTHOR("M7-B-ojh ");
MODULE_LICENSE("GPL");
