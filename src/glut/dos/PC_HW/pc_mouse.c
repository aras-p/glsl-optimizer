/*
 * PC/HW routine collection v0.4 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>

#include "pc_hw.h"



#define MOUSE_STACK_SIZE 16384

#define CLEAR_MICKEYS() \
        do { \
            __asm__ __volatile__ ("movw $0xb, %%ax; int $0x33":::"%eax", "%ecx", "%edx"); \
            ox = oy = 0; \
        } while (0)

extern void mouse_wrapper (void);
extern void mouse_wrapper_end (void);

static MFUNC mouse_func;
static void *mouse_stack;
static long mouse_callback;
static __dpmi_regs mouse_regs;

static volatile int pc_mouse_x, pc_mouse_y, pc_mouse_b;

static int minx = 0;
static int maxx = 319;
static int miny = 0;
static int maxy = 199;

static int sx = 2;
static int sy = 2;

static int emulat3 = FALSE;

static int ox, oy;

static void mouse (__dpmi_regs *r)
{
 int nx = (signed short)r->x.si / sx;
 int ny = (signed short)r->x.di / sy;
 int dx = nx - ox;
 int dy = ny - oy;
 ox = nx;
 oy = ny;

 pc_mouse_b = r->x.bx;
 pc_mouse_x = MID(minx, pc_mouse_x + dx, maxx);
 pc_mouse_y = MID(miny, pc_mouse_y + dy, maxy);

 if (emulat3) {
    if ((pc_mouse_b&3)==3) {
       pc_mouse_b = 4;
    }
 }

 if (mouse_func) {
    mouse_func(pc_mouse_x, pc_mouse_y, pc_mouse_b);
 }
} ENDOFUNC(mouse)

void pc_remove_mouse (void)
{
 if (mouse_callback) {
    pc_clexit(pc_remove_mouse);
    __asm__("\n\
		movl	%%edx, %%ecx	\n\
		shrl	$16, %%ecx	\n\
		movw	$0x0304, %%ax	\n\
		int	$0x31		\n\
		movw	$0x000c, %%ax	\n\
		xorl	%%ecx, %%ecx	\n\
		int	$0x33		\n\
    "::"d"(mouse_callback):"%eax", "%ecx");

    mouse_callback = 0;

    free((void *)((unsigned long)mouse_stack-MOUSE_STACK_SIZE));
 }
}

int pc_install_mouse (void)
{
 int buttons;

 /* fail if already call-backed */
 if (mouse_callback) {
    return 0;
 }

 /* reset mouse and get status */
 __asm__("\n\
		xorl	%%eax, %%eax	\n\
		int	$0x33		\n\
		andl	%%ebx, %%eax	\n\
		movl	%%eax, %0	\n\
 ":"=g" (buttons)::"%eax", "%ebx");
 if (!buttons) {
    return 0;
 }

 /* lock wrapper */
 LOCKDATA(mouse_func);
 LOCKDATA(mouse_stack);
 LOCKDATA(mouse_callback);
 LOCKDATA(mouse_regs);
 LOCKDATA(pc_mouse_x);
 LOCKDATA(pc_mouse_y);
 LOCKDATA(pc_mouse_b);
 LOCKDATA(minx);
 LOCKDATA(maxx);
 LOCKDATA(miny);
 LOCKDATA(maxy);
 LOCKDATA(sx);
 LOCKDATA(sy);
 LOCKDATA(emulat3);
 LOCKDATA(ox);
 LOCKDATA(oy);
 LOCKFUNC(mouse);
 LOCKFUNC(mouse_wrapper);

 /* grab a locked stack */
 if ((mouse_stack=pc_malloc(MOUSE_STACK_SIZE))==NULL) {
    return 0;
 }

 /* try to hook a call-back */
 __asm__("\n\
		pushl	%%ds		\n\
		pushl	%%es		\n\
		movw	$0x0303, %%ax	\n\
		pushl	%%ds		\n\
		pushl	%%cs		\n\
		popl	%%ds		\n\
		popl	%%es		\n\
		int	$0x31		\n\
		popl	%%es		\n\
		popl	%%ds		\n\
		jc	0f		\n\
		shll	$16, %%ecx	\n\
		movw	%%dx, %%cx	\n\
		movl	%%ecx, %0	\n\
	0:				\n\
 ":"=g"(mouse_callback)
  :"S" (mouse_wrapper), "D"(&mouse_regs)
  :"%eax", "%ecx", "%edx");
 if (!mouse_callback) {
    free(mouse_stack);
    return 0;
 }

 /* adjust stack */
 mouse_stack = (void *)((unsigned long)mouse_stack + MOUSE_STACK_SIZE);

 /* install the handler */
 mouse_regs.x.ax = 0x000c;
 mouse_regs.x.cx = 0x007f;
 mouse_regs.x.dx = mouse_callback&0xffff;
 mouse_regs.x.es = mouse_callback>>16;
 __dpmi_int(0x33, &mouse_regs);

 CLEAR_MICKEYS();

 emulat3 = buttons<3;
 pc_atexit(pc_remove_mouse);
 return buttons;
}

MFUNC pc_install_mouse_handler (MFUNC handler)
{
 MFUNC old;

 if (!mouse_callback && !pc_install_mouse()) {
    return NULL;
 }

 old = mouse_func;
 mouse_func = handler;
 return old;
}

void pc_mouse_area (int x1, int y1, int x2, int y2)
{
 minx = x1;
 maxx = x2;
 miny = y1;
 maxy = y2;
}

void pc_mouse_speed (int xspeed, int yspeed)
{
 DISABLE();

 sx = MAX(1, xspeed);
 sy = MAX(1, yspeed);

 ENABLE();
}

int pc_query_mouse (int *x, int *y)
{
 *x = pc_mouse_x;
 *y = pc_mouse_y;
 return pc_mouse_b;
}

void pc_show_mouse (void)
{
 /* not implemented */
}
void pc_scare_mouse (void)
{
 /* not implemented */
}
void pc_unscare_mouse (void)
{
 /* not implemented */
}

__asm__("\n\
		.balign	4				\n\
		.global	_mouse_wrapper			\n\
_mouse_wrapper:						\n\
		cld					\n\
		lodsl					\n\
		movl	%eax, %es:42(%edi)		\n\
		addw	$4, %es:46(%edi)		\n\
		pushl	%es				\n\
		movl	%ss, %ebx			\n\
		movl	%esp, %esi			\n\
		movl	%cs:___djgpp_ds_alias, %ss	\n\
		movl	%cs:_mouse_stack, %esp		\n\
		pushl	%ss				\n\
		pushl	%ss				\n\
		popl	%es				\n\
		popl	%ds				\n\
		movl	___djgpp_dos_sel, %fs		\n\
		pushl	%fs				\n\
		popl	%gs				\n\
		pushl	%edi				\n\
		call	_mouse				\n\
		popl	%edi				\n\
		movl	%ebx, %ss			\n\
		movl	%esi, %esp			\n\
		popl	%es				\n\
		iret					\n\
							\n\
		.balign	4				\n\
		.global	_mouse_wrapper_end		\n\
_mouse_wrapper_end:");
