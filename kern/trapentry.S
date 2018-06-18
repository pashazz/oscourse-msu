/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>

###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.comm intr_ret_eip, 4
.comm intr_ebp_reg, 4
.comm intr_esp_reg, 4
.comm intr_cs, 4
.comm intr_ss, 4
.comm intr_eflags, 4
.comm intr_err, 4
.comm intr_trapno, 4

.text

TRAPHANDLER_NOEC(DE, T_DIVIDE)
TRAPHANDLER_NOEC(DB, T_DEBUG)
TRAPHANDLER_NOEC(NMI, T_NMI)
TRAPHANDLER_NOEC(BP, T_BRKPT)
TRAPHANDLER_NOEC(OF, T_OFLOW)
TRAPHANDLER_NOEC(BR, T_BOUND)
TRAPHANDLER_NOEC(UD, T_ILLOP)
TRAPHANDLER_NOEC(NM, T_DEVICE)
TRAPHANDLER(DF, T_DBLFLT)
TRAPHANDLER(TS, T_TSS)
TRAPHANDLER(NP, T_SEGNP)
TRAPHANDLER(SS, T_STACK)
TRAPHANDLER(GP, T_GPFLT)
TRAPHANDLER(PF, T_PGFLT)
TRAPHANDLER_NOEC(MF, T_FPERR)
TRAPHANDLER(AC, T_ALIGN)
TRAPHANDLER_NOEC(MC, T_MCHK)
TRAPHANDLER_NOEC(XM, T_SIMDERR)
TRAPHANDLER_NOEC(SYSCALL, T_SYSCALL);
TRAPHANDLER_NOEC(KBD, IRQ_OFFSET + IRQ_KBD);
TRAPHANDLER_NOEC(SERIAL, IRQ_OFFSET + IRQ_SERIAL);

.globl _alltraps
.type _alltraps, @function;
.align 2
_alltraps:
    pushl %ds
    pushl %es
    pushal
    pushl %eax
    mov $GD_KT, %eax
    mov %ax, %ds
    mov %ax, %es
    popl %eax
    pushl %esp
    call trap

.globl clock_thdlr
.type clock_thdlr, @function;
.align 2;
#ifdef CONFIG_KSPACE
clock_thdlr:
	popl intr_ret_eip
	popl intr_cs
	popl intr_eflags
	movl %ebp, intr_ebp_reg
	movl %esp, intr_esp_reg
	movl $0x0,%ebp
	movl $(bootstacktop),%esp
	pushl $GD_KD
	pushl intr_esp_reg
	pushl intr_eflags
	pushl intr_cs
	pushl intr_ret_eip
	pushl $0
	pushl $(IRQ_OFFSET + IRQ_CLOCK)
	pushl %ds
	pushl %es

	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl intr_esp_reg
	pushl intr_ebp_reg
	pushl %esi
	pushl %edi

	pushl %esp  /* trap(%esp) */
	call trap
	jmp .
#else
clock_thdlr:
	pushl $0;							\
	pushl $(IRQ_OFFSET + IRQ_CLOCK);							\
	jmp _alltraps
#endif
