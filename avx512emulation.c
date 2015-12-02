#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/livepatch.h>
#include <linux/ptrace.h>
#include <linux/context_tracking.h>
#include <linux/kdebug.h>
#include <asm/traps.h>
#include <asm/uaccess.h>
#include <asm/vm86.h>

static void (*do_error_trap)(struct pt_regs *regs, long error_code, char *str,
						unsigned long trapnr, int signr);

static u32 zmm0_register[16];

static void avx512emulation_do_invalid_op(struct pt_regs *regs, long error_code)
{
	u8 inst[6] = {0};
	u8 vmovdqa32_rax_zmm0[] = {0x62, 0xf1, 0x7d, 0x48, 0x6f, 0x00};
	u8 vpaddd_rbx_zmm0_zmm0[] = {0x62, 0xf1, 0x7d, 0x48, 0xfe, 0x03};
	u8 vmovdqa32_zmm0_rcx[] = {0x62, 0xf1, 0x7d, 0x48, 0x7f, 0x01};
	int error_bytes = copy_from_user(inst, (void*)regs->ip, sizeof(inst));
	if (error_bytes != 0) {
		goto no_emulation;
	} else if (memcmp(inst, vmovdqa32_rax_zmm0, sizeof(inst)) == 0) {
		printk("do_invalid_op: vmovdqa32_rax_zmm0\n");
		if (copy_from_user(zmm0_register, (void*)regs->ax, sizeof(zmm0_register)) != 0) {
			printk("do_invalid_op: copy failed\n");
			goto no_emulation;
		}
	} else if (memcmp(inst, vpaddd_rbx_zmm0_zmm0, sizeof(inst)) == 0) {
		u32 addition[16];
		int i;
		printk("do_invalid_op: vpaddd_rbx_zmm0_zmm0\n");
		if (copy_from_user(addition, (void*)regs->bx, sizeof(addition)) != 0) {
			printk("do_invalid_op: copy failed\n");
			goto no_emulation;
		}
		for (i = 0; i < 16; i++) {
			zmm0_register[i] += addition[i];
		}
	} else if (memcmp(inst, vmovdqa32_zmm0_rcx, sizeof(inst)) == 0) {
		printk("do_invalid_op: vmovdqa32_zmm0_rcx\n");
		if (copy_to_user((void*)regs->cx, zmm0_register, sizeof(zmm0_register)) != 0) {
			printk("do_invalid_op: copy failed\n");
			goto no_emulation;
		}
	} else {
		goto no_emulation;
	}
	regs->ip += sizeof(inst);
	return;
no_emulation:
	printk("do_invalid_op: error_bytes=%d, inst={0x%x, 0x%x, 0x%x, 0x%x, 0x%x}\n",
		   error_bytes, inst[0], inst[1], inst[2], inst[3], inst[4]);
	do_error_trap(regs, error_code, "invalid opcode", X86_TRAP_UD, SIGILL);
}

static struct klp_func funcs[] = {
	{
		.old_name = "do_invalid_op",
		.new_func = avx512emulation_do_invalid_op,
	}, { }
};

static struct klp_object objs[] = {
	{
		/* name being NULL means vmlinux */
		.funcs = funcs,
	}, { }
};

static struct klp_patch patch = {
	.mod = THIS_MODULE,
	.objs = objs,
};

static int avx512emulation_init(void)
{
	int ret;

	ret = klp_register_patch(&patch);
	if (ret)
		return ret;
	ret = klp_enable_patch(&patch);
	if (ret) {
		WARN_ON(klp_unregister_patch(&patch));
		return ret;
	}

	do_error_trap = (void*)kallsyms_lookup_name("do_error_trap");
	if (!do_error_trap) {
		printk("do_error_trap = NULL");
		return 1;
	}

	return 0;
}

static void avx512emulation_exit(void)
{
	WARN_ON(klp_disable_patch(&patch));
	WARN_ON(klp_unregister_patch(&patch));
}

module_init(avx512emulation_init);
module_exit(avx512emulation_exit);
MODULE_LICENSE("GPL");
