/*
 * PC/HW routine collection v1.3 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


		.file	"pc_irq.S"

		.text

#define	IRQ_STACK_SIZE	16384

#define	IRQ_WRAPPER_LEN	(__irq_wrapper_1-__irq_wrapper_0)
#define	IRQ_OLD		(__irq_old_0-__irq_wrapper_0)
#define	IRQ_HOOK	(__irq_hook_0-__irq_wrapper_0)
#define	IRQ_STACK	(__irq_stack_0-__irq_wrapper_0)

		.balign	4
common:
		movw	$0x0400, %ax
		int	$0x31

		movl	%ss:8(%ebp), %ebx
		cmpl	$15, %ebx
		jbe	0f
	fail:
		orl	$-1, %eax
		popl	%edi
		popl	%ebx
		leave
		ret

	0:
		movl	%ebx, %edi
		imull	$IRQ_WRAPPER_LEN, %edi
		addl	$__irq_wrapper_0, %edi

		cmpb	$7, %bl
		jbe	1f
		movb	%dl, %dh
		subb	$8, %dh
	1:
		addb	%dh, %bl
		ret

		.balign	4
		.global	_pc_install_irq
_pc_install_irq:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%ebx
		pushl	%edi

		call	common

		cmpl	$0, IRQ_HOOK(%edi)
		jne	fail

		pushl	$IRQ_WRAPPER_LEN
		pushl	%edi
		call	__go32_dpmi_lock_code
		addl	$8, %esp
		testl	%eax, %eax
		jnz	fail

		pushl	$IRQ_STACK_SIZE
		call	_pc_malloc
		popl	%edx
		testl	%eax, %eax
		jz	fail
		addl	%edx, %eax
		movl	%eax, IRQ_STACK(%edi)

		movl	___djgpp_ds_alias, %eax
		movl	%eax, IRQ_STACK+4(%edi)

		movl	%ss:12(%ebp), %eax
		movl	%eax, IRQ_HOOK(%edi)

		movw	$0x0204, %ax
		int	$0x31
		movl	%edx, IRQ_OLD(%edi)
		movw	%cx, IRQ_OLD+4(%edi)
		movw	$0x0205, %ax
		movl	%edi, %edx
		movl	%cs, %ecx
		int	$0x31

	done:
		xorl	%eax, %eax
		popl	%edi
		popl	%ebx
		leave
		ret

		.balign	4
		.global	_pc_remove_irq
_pc_remove_irq:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%ebx
		pushl	%edi

		call	common

		cmpl	$0, IRQ_HOOK(%edi)
		je	fail

		movl	$0, IRQ_HOOK(%edi)

		movw	$0x0205, %ax
		movl	IRQ_OLD(%edi), %edx
		movl	IRQ_OLD+4(%edi), %ecx
		int	$0x31

		movl	IRQ_STACK(%edi), %eax
		subl	$IRQ_STACK_SIZE, %eax
		pushl	%eax
		call	_free
		popl	%eax

		jmp	done

#define	WRAPPER(x)							   ; \
		.balign	4						   ; \
__irq_wrapper_##x:							   ; \
		pushal							   ; \
		pushl	%ds						   ; \
		pushl	%es						   ; \
		pushl	%fs						   ; \
		pushl	%gs						   ; \
		movl	%ss, %ebx					   ; \
		movl	%esp, %esi					   ; \
		lss	%cs:__irq_stack_##x, %esp			   ; \
		pushl	%ss						   ; \
		pushl	%ss						   ; \
		popl	%es						   ; \
		popl	%ds						   ; \
		movl	___djgpp_dos_sel, %fs				   ; \
		pushl	%fs						   ; \
		popl	%gs						   ; \
		call	*__irq_hook_##x					   ; \
		movl	%ebx, %ss					   ; \
		movl	%esi, %esp					   ; \
		testl	%eax, %eax					   ; \
		popl	%gs						   ; \
		popl	%fs						   ; \
		popl	%es						   ; \
		popl	%ds						   ; \
		popal							   ; \
		jz	__irq_ignore_##x				   ; \
__irq_bypass_##x:							   ; \
		ljmp	*%cs:__irq_old_##x				   ; \
__irq_ignore_##x:							   ; \
		iret							   ; \
		.balign	4						   ; \
__irq_old_##x:								   ; \
		.long	0, 0						   ; \
__irq_hook_##x:								   ; \
		.long	0						   ; \
__irq_stack_##x:							   ; \
		.long	0, 0

		WRAPPER(0);
		WRAPPER(1);
		WRAPPER(2);
		WRAPPER(3);
		WRAPPER(4);
		WRAPPER(5);
		WRAPPER(6);
		WRAPPER(7);
		WRAPPER(8);
		WRAPPER(9);
		WRAPPER(10);
		WRAPPER(11);
		WRAPPER(12);
		WRAPPER(13);
		WRAPPER(14);
		WRAPPER(15);
