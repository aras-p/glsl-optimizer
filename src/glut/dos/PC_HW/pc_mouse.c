/*
 * PC/HW routine collection v1.3 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>
#include <sys/exceptn.h>
#include <sys/segments.h>

#include "pc_hw.h"


#define PC_CUTE_WHEEL 1 /* CuteMouse WheelAPI */

#define MOUSE_STACK_SIZE 16384

#define CLEAR_MICKEYS() \
   do { \
      __asm __volatile ("movw $0xb, %%ax; int $0x33":::"%eax", "%ecx", "%edx"); \
      ox = oy = 0; \
   } while (0)

extern void mouse_wrap (void);
extern int mouse_wrap_end[];

static MFUNC mouse_func;
static long mouse_callback;
static __dpmi_regs mouse_regs;

static volatile struct {
   volatile int x, y, z, b;
} pc_mouse;

static int minx = 0;
static int maxx = 319;
static int miny = 0;
static int maxy = 199;
static int minz = 0;
static int maxz = 255;

static int sx = 2;
static int sy = 2;

static int emulat3 = FALSE;

static int ox, oy;


static void
mouse (__dpmi_regs *r)
{
   int nx = (signed short)r->x.si / sx;
   int ny = (signed short)r->x.di / sy;
   int dx = nx - ox;
   int dy = ny - oy;
#if PC_CUTE_WHEEL
   int dz = (signed char)r->h.bh;
#endif
   ox = nx;
   oy = ny;

   pc_mouse.b = r->h.bl;
   pc_mouse.x = MID(minx, pc_mouse.x + dx, maxx);
   pc_mouse.y = MID(miny, pc_mouse.y + dy, maxy);
#if PC_CUTE_WHEEL
   pc_mouse.z = MID(minz, pc_mouse.z + dz, maxz);
#endif

   if (emulat3) {
      if ((pc_mouse.b & 3) == 3) {
         pc_mouse.b = 4;
      }
   }

   if (mouse_func) {
      mouse_func(pc_mouse.x, pc_mouse.y, pc_mouse.z, pc_mouse.b);
   }
} ENDOFUNC(mouse)


void
pc_remove_mouse (void)
{
   if (mouse_callback) {
      pc_clexit(pc_remove_mouse);
      __asm("\n\
		movl	%%edx, %%ecx	\n\
		shrl	$16, %%ecx	\n\
		movw	$0x0304, %%ax	\n\
		int	$0x31		\n\
		movw	$0x000c, %%ax	\n\
		xorl	%%ecx, %%ecx	\n\
		int	$0x33		\n\
      "::"d"(mouse_callback):"%eax", "%ecx");

      mouse_callback = 0;

      free((void *)(mouse_wrap_end[0] - MOUSE_STACK_SIZE));
   }
}


int
pc_install_mouse (void)
{
   int buttons;

   /* fail if already call-backed */
   if (mouse_callback) {
      return 0;
   }

   /* reset mouse and get status */
   __asm("\n\
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
   LOCKDATA(mouse_callback);
   LOCKDATA(mouse_regs);
   LOCKDATA(pc_mouse);
   LOCKDATA(minx);
   LOCKDATA(maxx);
   LOCKDATA(miny);
   LOCKDATA(maxy);
   LOCKDATA(minz);
   LOCKDATA(maxz);
   LOCKDATA(sx);
   LOCKDATA(sy);
   LOCKDATA(emulat3);
   LOCKDATA(ox);
   LOCKDATA(oy);
   LOCKFUNC(mouse);
   LOCKFUNC(mouse_wrap);

   mouse_wrap_end[1] = __djgpp_ds_alias;
   /* grab a locked stack */
   if ((mouse_wrap_end[0] = (int)pc_malloc(MOUSE_STACK_SIZE)) == NULL) {
      return 0;
   }

   /* try to hook a call-back */
   __asm("\n\
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
    :"S" (mouse_wrap), "D"(&mouse_regs)
    :"%eax", "%ecx", "%edx");
   if (!mouse_callback) {
      free((void *)mouse_wrap_end[0]);
      return 0;
   }

   /* adjust stack */
   mouse_wrap_end[0] += MOUSE_STACK_SIZE;

   /* install the handler */
   mouse_regs.x.ax = 0x000c;
#if PC_CUTE_WHEEL
   mouse_regs.x.cx = 0x7f | 0x80;
#else
   mouse_regs.x.cx = 0x7f;
#endif
   mouse_regs.x.dx = mouse_callback & 0xffff;
   mouse_regs.x.es = mouse_callback >> 16;
   __dpmi_int(0x33, &mouse_regs);

   CLEAR_MICKEYS();

   emulat3 = (buttons < 3);
   pc_atexit(pc_remove_mouse);
   return buttons;
}


MFUNC
pc_install_mouse_handler (MFUNC handler)
{
   MFUNC old;

   if (!mouse_callback && !pc_install_mouse()) {
      return NULL;
   }

   old = mouse_func;
   mouse_func = handler;
   return old;
}


void
pc_mouse_area (int x1, int y1, int x2, int y2)
{
   minx = x1;
   maxx = x2;
   miny = y1;
   maxy = y2;
}


void
pc_mouse_speed (int xspeed, int yspeed)
{
   DISABLE();

   sx = MAX(1, xspeed);
   sy = MAX(1, yspeed);

   ENABLE();
}


int
pc_query_mouse (int *x, int *y, int *z)
{
   *x = pc_mouse.x;
   *y = pc_mouse.y;
   *z = pc_mouse.z;
   return pc_mouse.b;
}


void
pc_warp_mouse (int x, int y)
{
   CLEAR_MICKEYS();

   pc_mouse.x = MID(minx, x, maxx);
   pc_mouse.y = MID(miny, y, maxy);

   if (mouse_func) {
      mouse_func(pc_mouse.x, pc_mouse.y, pc_mouse.z, pc_mouse.b);
   }
}


/* Hack alert:
 * `mouse_wrap_end' actually holds the
 * address of stack in a safe data selector.
 */
__asm("\n\
		.text				\n\
		.p2align 5,,31			\n\
		.global	_mouse_wrap		\n\
_mouse_wrap:					\n\
		cld				\n\
		lodsl				\n\
		movl	%eax, %es:42(%edi)	\n\
		addw	$4, %es:46(%edi)	\n\
		pushl	%es			\n\
		movl	%ss, %ebx		\n\
		movl	%esp, %esi		\n\
		lss	%cs:_mouse_wrap_end, %esp\n\
		pushl	%ss			\n\
		pushl	%ss			\n\
		popl	%es			\n\
		popl	%ds			\n\
		movl	___djgpp_dos_sel, %fs	\n\
		pushl	%fs			\n\
		popl	%gs			\n\
		pushl	%edi			\n\
		call	_mouse			\n\
		popl	%edi			\n\
		movl	%ebx, %ss		\n\
		movl	%esi, %esp		\n\
		popl	%es			\n\
		iret				\n\
		.global	_mouse_wrap_end		\n\
_mouse_wrap_end:.long	0, 0");
