/*
 * PC/HW routine collection v1.4 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef PC_HW_H_included
#define PC_HW_H_included

#include <dpmi.h>
#include <stdlib.h>

/*
 * misc C definitions
 */
#define FALSE 0
#define TRUE !FALSE

#define SQR(x)       ((x) * (x))

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))

typedef void (*VFUNC) (void);
typedef void (*PFUNC) (void *);
typedef void (*MFUNC) (int x, int y, int z, int b);

/*
 * atexit
 */
int pc_atexit (VFUNC f);
int pc_clexit (VFUNC f);

/*
 * locked memory
 */
#define ENDOFUNC(x)    static void x##_end() { }
#define LOCKFUNC(x)    _go32_dpmi_lock_code((void *)x, (long)x##_end - (long)x)
#define LOCKDATA(x)    _go32_dpmi_lock_data((void *)&x, sizeof(x))
#define LOCKBUFF(x, l) _go32_dpmi_lock_data((void *)x, l)

void *pc_malloc (size_t size);

/*
 * IRQ
 */
#define ENABLE()  __asm __volatile ("sti")
#define DISABLE() __asm __volatile ("cli")

extern int pc_install_irq (int i, int (*handler) ());
extern int pc_remove_irq (int i);

/*
 * keyboard
 */
#define KB_SHIFT_FLAG         0x0001
#define KB_CTRL_FLAG          0x0002
#define KB_ALT_FLAG           0x0004
#define KB_LWIN_FLAG          0x0008
#define KB_RWIN_FLAG          0x0010
#define KB_MENU_FLAG          0x0020
#define KB_SCROLOCK_FLAG      0x0100
#define KB_NUMLOCK_FLAG       0x0200
#define KB_CAPSLOCK_FLAG      0x0400
#define KB_INALTSEQ_FLAG      0x0800
#define KB_ACCENT1_FLAG       0x1000
#define KB_ACCENT2_FLAG       0x2000
#define KB_ACCENT3_FLAG       0x4000
#define KB_ACCENT4_FLAG       0x8000

#define KEY_A                 1
#define KEY_B                 2
#define KEY_C                 3
#define KEY_D                 4
#define KEY_E                 5
#define KEY_F                 6
#define KEY_G                 7
#define KEY_H                 8
#define KEY_I                 9
#define KEY_J                 10
#define KEY_K                 11
#define KEY_L                 12
#define KEY_M                 13
#define KEY_N                 14
#define KEY_O                 15
#define KEY_P                 16
#define KEY_Q                 17
#define KEY_R                 18
#define KEY_S                 19
#define KEY_T                 20
#define KEY_U                 21
#define KEY_V                 22
#define KEY_W                 23
#define KEY_X                 24
#define KEY_Y                 25
#define KEY_Z                 26
#define KEY_0                 27
#define KEY_1                 28
#define KEY_2                 29
#define KEY_3                 30
#define KEY_4                 31
#define KEY_5                 32
#define KEY_6                 33
#define KEY_7                 34
#define KEY_8                 35
#define KEY_9                 36
#define KEY_0_PAD             37
#define KEY_1_PAD             38
#define KEY_2_PAD             39
#define KEY_3_PAD             40
#define KEY_4_PAD             41
#define KEY_5_PAD             42
#define KEY_6_PAD             43
#define KEY_7_PAD             44
#define KEY_8_PAD             45
#define KEY_9_PAD             46
#define KEY_F1                47
#define KEY_F2                48
#define KEY_F3                49
#define KEY_F4                50
#define KEY_F5                51
#define KEY_F6                52
#define KEY_F7                53
#define KEY_F8                54
#define KEY_F9                55
#define KEY_F10               56
#define KEY_F11               57
#define KEY_F12               58
#define KEY_ESC               59
#define KEY_TILDE             60
#define KEY_MINUS             61
#define KEY_EQUALS            62
#define KEY_BACKSPACE         63
#define KEY_TAB               64
#define KEY_OPENBRACE         65
#define KEY_CLOSEBRACE        66
#define KEY_ENTER             67
#define KEY_COLON             68
#define KEY_QUOTE             69
#define KEY_BACKSLASH         70
#define KEY_BACKSLASH2        71
#define KEY_COMMA             72
#define KEY_STOP              73
#define KEY_SLASH             74
#define KEY_SPACE             75
#define KEY_INSERT            76
#define KEY_DEL               77
#define KEY_HOME              78
#define KEY_END               79
#define KEY_PGUP              80
#define KEY_PGDN              81
#define KEY_LEFT              82
#define KEY_RIGHT             83
#define KEY_UP                84
#define KEY_DOWN              85
#define KEY_SLASH_PAD         86
#define KEY_ASTERISK          87
#define KEY_MINUS_PAD         88
#define KEY_PLUS_PAD          89
#define KEY_DEL_PAD           90
#define KEY_ENTER_PAD         91
#define KEY_PRTSCR            92
#define KEY_PAUSE             93
#define KEY_ABNT_C1           94
#define KEY_YEN               95
#define KEY_KANA              96
#define KEY_CONVERT           97
#define KEY_NOCONVERT         98
#define KEY_AT                99
#define KEY_CIRCUMFLEX        100
#define KEY_COLON2            101
#define KEY_KANJI             102

#define KEY_MODIFIERS         103

#define KEY_LSHIFT            103
#define KEY_RSHIFT            104
#define KEY_LCONTROL          105
#define KEY_RCONTROL          106
#define KEY_ALT               107
#define KEY_ALTGR             108
#define KEY_LWIN              109
#define KEY_RWIN              110
#define KEY_MENU              111
#define KEY_SCRLOCK           112
#define KEY_NUMLOCK           113
#define KEY_CAPSLOCK          114

#define KEY_MAX               115

int pc_install_keyb (void);
void pc_remove_keyb (void);
int pc_keypressed (void);
int pc_readkey (void);
int pc_keydown (int code);
int pc_keyshifts (void);

/*
 * timer
 */
int pc_install_int (PFUNC func, void *parm, unsigned int freq);
int pc_remove_int (int fid);
int pc_adjust_int (int fid, unsigned int freq);
void pc_remove_timer (void);

/*
 * mouse
 */
int pc_install_mouse (void);
void pc_remove_mouse (void);
MFUNC pc_install_mouse_handler (MFUNC handler);
void pc_mouse_area (int x1, int y1, int x2, int y2);
void pc_mouse_speed (int xspeed, int yspeed);
int pc_query_mouse (int *x, int *y, int *z);
void pc_warp_mouse (int x, int y);

/*
 * standard redirection
 */
int pc_open_stdout (void);
int pc_open_stderr (void);
void pc_close_stdout (void);
void pc_close_stderr (void);

#endif
