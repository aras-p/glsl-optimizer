/*
 * PC/HW routine collection v1.3 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <pc.h>
#include <sys/exceptn.h>
#include <sys/farptr.h>

#include "pc_hw.h"


#define KEYB_IRQ 1

#define KEY_BUFFER_SIZE 64

#define KB_MODIFIERS (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG | KB_LWIN_FLAG | KB_RWIN_FLAG | KB_MENU_FLAG)
#define KB_LED_FLAGS (KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG)

static int keyboard_installed;

static volatile struct {
   volatile int start, end;
   volatile int key[KEY_BUFFER_SIZE];
} key_buffer;

static volatile int key_enhanced, key_pause_loop, key_shifts;
static int leds_ok = TRUE;
static int in_a_terrupt = FALSE;
static volatile char pc_key[KEY_MAX];


/* convert Allegro format scancodes into key_shifts flag bits */
static unsigned short modifier_table[KEY_MAX - KEY_MODIFIERS] = {
   KB_SHIFT_FLAG,    KB_SHIFT_FLAG,    KB_CTRL_FLAG,
   KB_CTRL_FLAG,     KB_ALT_FLAG,      KB_ALT_FLAG,
   KB_LWIN_FLAG,     KB_RWIN_FLAG,     KB_MENU_FLAG,
   KB_SCROLOCK_FLAG, KB_NUMLOCK_FLAG,  KB_CAPSLOCK_FLAG
};


/* lookup table for converting hardware scancodes into Allegro format */
static unsigned char hw_to_mycode[128] = {
   /* 0x00 */  0,              KEY_ESC,        KEY_1,          KEY_2, 
   /* 0x04 */  KEY_3,          KEY_4,          KEY_5,          KEY_6,
   /* 0x08 */  KEY_7,          KEY_8,          KEY_9,          KEY_0, 
   /* 0x0C */  KEY_MINUS,      KEY_EQUALS,     KEY_BACKSPACE,  KEY_TAB,
   /* 0x10 */  KEY_Q,          KEY_W,          KEY_E,          KEY_R, 
   /* 0x14 */  KEY_T,          KEY_Y,          KEY_U,          KEY_I,
   /* 0x18 */  KEY_O,          KEY_P,          KEY_OPENBRACE,  KEY_CLOSEBRACE, 
   /* 0x1C */  KEY_ENTER,      KEY_LCONTROL,   KEY_A,          KEY_S,
   /* 0x20 */  KEY_D,          KEY_F,          KEY_G,          KEY_H, 
   /* 0x24 */  KEY_J,          KEY_K,          KEY_L,          KEY_COLON,
   /* 0x28 */  KEY_QUOTE,      KEY_TILDE,      KEY_LSHIFT,     KEY_BACKSLASH, 
   /* 0x2C */  KEY_Z,          KEY_X,          KEY_C,          KEY_V,
   /* 0x30 */  KEY_B,          KEY_N,          KEY_M,          KEY_COMMA, 
   /* 0x34 */  KEY_STOP,       KEY_SLASH,      KEY_RSHIFT,     KEY_ASTERISK,
   /* 0x38 */  KEY_ALT,        KEY_SPACE,      KEY_CAPSLOCK,   KEY_F1, 
   /* 0x3C */  KEY_F2,         KEY_F3,         KEY_F4,         KEY_F5,
   /* 0x40 */  KEY_F6,         KEY_F7,         KEY_F8,         KEY_F9, 
   /* 0x44 */  KEY_F10,        KEY_NUMLOCK,    KEY_SCRLOCK,    KEY_7_PAD,
   /* 0x48 */  KEY_8_PAD,      KEY_9_PAD,      KEY_MINUS_PAD,  KEY_4_PAD, 
   /* 0x4C */  KEY_5_PAD,      KEY_6_PAD,      KEY_PLUS_PAD,   KEY_1_PAD,
   /* 0x50 */  KEY_2_PAD,      KEY_3_PAD,      KEY_0_PAD,      KEY_DEL_PAD, 
   /* 0x54 */  KEY_PRTSCR,     0,              KEY_BACKSLASH2, KEY_F11,
   /* 0x58 */  KEY_F12,        0,              0,              KEY_LWIN, 
   /* 0x5C */  KEY_RWIN,       KEY_MENU,       0,              0,
   /* 0x60 */  0,              0,              0,              0, 
   /* 0x64 */  0,              0,              0,              0,
   /* 0x68 */  0,              0,              0,              0, 
   /* 0x6C */  0,              0,              0,              0,
   /* 0x70 */  KEY_KANA,       0,              0,              KEY_ABNT_C1, 
   /* 0x74 */  0,              0,              0,              0,
   /* 0x78 */  0,              KEY_CONVERT,    0,              KEY_NOCONVERT, 
   /* 0x7C */  0,              KEY_YEN,       0,              0
};


/* lookup table for converting extended hardware codes into Allegro format */
static unsigned char hw_to_mycode_ex[128] = {
   /* 0x00 */  0,              KEY_ESC,        KEY_1,          KEY_2,
   /* 0x04 */  KEY_3,          KEY_4,          KEY_5,          KEY_6,
   /* 0x08 */  KEY_7,          KEY_8,          KEY_9,          KEY_0,
   /* 0x0C */  KEY_MINUS,      KEY_EQUALS,     KEY_BACKSPACE,  KEY_TAB,
   /* 0x10 */  KEY_CIRCUMFLEX, KEY_AT,         KEY_COLON2,     KEY_R,
   /* 0x14 */  KEY_KANJI,      KEY_Y,          KEY_U,          KEY_I,
   /* 0x18 */  KEY_O,          KEY_P,          KEY_OPENBRACE,  KEY_CLOSEBRACE,
   /* 0x1C */  KEY_ENTER_PAD,  KEY_RCONTROL,   KEY_A,          KEY_S,
   /* 0x20 */  KEY_D,          KEY_F,          KEY_G,          KEY_H,
   /* 0x24 */  KEY_J,          KEY_K,          KEY_L,          KEY_COLON,
   /* 0x28 */  KEY_QUOTE,      KEY_TILDE,      0,              KEY_BACKSLASH,
   /* 0x2C */  KEY_Z,          KEY_X,          KEY_C,          KEY_V,
   /* 0x30 */  KEY_B,          KEY_N,          KEY_M,          KEY_COMMA,
   /* 0x34 */  KEY_STOP,       KEY_SLASH_PAD,  0,              KEY_PRTSCR,
   /* 0x38 */  KEY_ALTGR,      KEY_SPACE,      KEY_CAPSLOCK,   KEY_F1,
   /* 0x3C */  KEY_F2,         KEY_F3,         KEY_F4,         KEY_F5,
   /* 0x40 */  KEY_F6,         KEY_F7,         KEY_F8,         KEY_F9,
   /* 0x44 */  KEY_F10,        KEY_NUMLOCK,    KEY_PAUSE,      KEY_HOME,
   /* 0x48 */  KEY_UP,         KEY_PGUP,       KEY_MINUS_PAD,  KEY_LEFT,
   /* 0x4C */  KEY_5_PAD,      KEY_RIGHT,      KEY_PLUS_PAD,   KEY_END,
   /* 0x50 */  KEY_DOWN,       KEY_PGDN,       KEY_INSERT,     KEY_DEL,
   /* 0x54 */  KEY_PRTSCR,     0,              KEY_BACKSLASH2, KEY_F11,
   /* 0x58 */  KEY_F12,        0,              0,              KEY_LWIN,
   /* 0x5C */  KEY_RWIN,       KEY_MENU,       0,              0,
   /* 0x60 */  0,              0,              0,              0,
   /* 0x64 */  0,              0,              0,              0,
   /* 0x68 */  0,              0,              0,              0,
   /* 0x6C */  0,              0,              0,              0,
   /* 0x70 */  0,              0,              0,              0,
   /* 0x74 */  0,              0,              0,              0,
   /* 0x78 */  0,              0,              0,              0,
   /* 0x7C */  0,              0,              0,              0
};


/* default mapping table for the US keyboard layout */
static unsigned short standard_key_ascii_table[KEY_MAX] = {
   /* start */       0,
   /* alphabet */    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
   /* numbers */     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, '`', '-', '=', 8, 9, '[', ']', 13, ';', '\'', '\\', '\\', ',', '.', '/', ' ',
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      '/', '*', '-', '+', '.', 13,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* capslock mapping table for the US keyboard layout */
static unsigned short standard_key_capslock_table[KEY_MAX] = {
   /* start */       0,
   /* alphabet */    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
   /* numbers */     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, '`', '-', '=', 8, 9, '[', ']', 13, ';', '\'', '\\', '\\', ',', '.', '/', ' ',
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      '/', '*', '-', '+', '.', 13,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* shifted mapping table for the US keyboard layout */
static unsigned short standard_key_shift_table[KEY_MAX] = {
   /* start */       0,
   /* alphabet */    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
   /* numbers */     ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, '~', '_', '+', 8, 9, '{', '}', 13, ':', '"', '|', '|', '<', '>', '?', ' ',
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      '/', '*', '-', '+', '.', 13,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* ctrl+key mapping table for the US keyboard layout */
static unsigned short standard_key_control_table[KEY_MAX] = {
   /* start */       0,
   /* alphabet */    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
   /* numbers */     2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, 2, 2, 2, 127, 127, 2, 2, 10, 2, 2, 2, 2, 2, 2, 2, 2,
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      2, 2, 2, 2, 2, 10,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* convert numeric pad scancodes into arrow codes */
static unsigned char numlock_table[10] = {
   KEY_INSERT, KEY_END,    KEY_DOWN,   KEY_PGDN,   KEY_LEFT,
   KEY_5_PAD,  KEY_RIGHT,  KEY_HOME,   KEY_UP,     KEY_PGUP
};


/* kb_wait_for_write_ready:
 *  Wait for the keyboard controller to set the ready-for-write bit.
 */
static __inline int
kb_wait_for_write_ready (void)
{
   int timeout = 4096;

   while ((timeout > 0) && (inportb(0x64) & 2)) timeout--;

   return (timeout > 0);
}


/* kb_wait_for_read_ready:
 *  Wait for the keyboard controller to set the ready-for-read bit.
 */
static __inline int
kb_wait_for_read_ready (void)
{
   int timeout = 16384;

   while ((timeout > 0) && (!(inportb(0x64) & 1))) timeout--;

   return (timeout > 0);
}


/* kb_send_data:
 *  Sends a byte to the keyboard controller. Returns 1 if all OK.
 */
static __inline int
kb_send_data (unsigned char data)
{
   int resends = 4;
   int timeout, temp;

   do {
      if (!kb_wait_for_write_ready())
         return 0;

      outportb(0x60, data);
      timeout = 4096;

      while (--timeout > 0) {
         if (!kb_wait_for_read_ready())
            return 0;

         temp = inportb(0x60);

         if (temp == 0xFA)
            return 1;

         if (temp == 0xFE)
            break;
      }
   } while ((resends-- > 0) && (timeout > 0));

   return 0;
}


static void
update_leds (int leds)
{
   if (leds_ok) {
      if (!in_a_terrupt)
         DISABLE();

      if (!kb_send_data(0xED)) {
         kb_send_data(0xF4);
         leds_ok = FALSE;
      } else if (!kb_send_data((leds >> 8) & 7)) {
         kb_send_data(0xF4);
         leds_ok = FALSE;
      }

      if (!in_a_terrupt)
         ENABLE();
   }
} ENDOFUNC(update_leds)


static void
inject_key (int scancode)
{
   unsigned short *table;

   if ((scancode >= KEY_0_PAD) && (scancode <= KEY_9_PAD)) {
      if (((key_shifts & KB_NUMLOCK_FLAG) != 0) == ((key_shifts & KB_SHIFT_FLAG) != 0)) {
         scancode = numlock_table[scancode - KEY_0_PAD];
      }
      table = standard_key_ascii_table;
   } else if (key_shifts & KB_CTRL_FLAG) {
      table = standard_key_control_table;
   } else if (key_shifts & KB_SHIFT_FLAG) {
      if (key_shifts & KB_CAPSLOCK_FLAG) {
         if (standard_key_ascii_table[scancode] == standard_key_capslock_table[scancode]) {
            table = standard_key_shift_table;
         } else {
            table = standard_key_ascii_table;
         }
      } else {
         table = standard_key_shift_table;
      }
   } else if (key_shifts & KB_CAPSLOCK_FLAG) {
      table = standard_key_capslock_table;
   } else {
      table = standard_key_ascii_table;
   }

   key_buffer.key[key_buffer.end++] = (scancode << 16) | table[scancode];

   if (key_buffer.end >= KEY_BUFFER_SIZE)
      key_buffer.end = 0;
   if (key_buffer.end == key_buffer.start) {
      key_buffer.start++;
      if (key_buffer.start >= KEY_BUFFER_SIZE)
         key_buffer.start = 0;
   }
} ENDOFUNC(inject_key)


static void
handle_code (int scancode, int keycode)
{
   in_a_terrupt++;

   if (keycode == 0) {            /* pause */
      inject_key(scancode);
      pc_key[KEY_PAUSE] ^= TRUE;
   } else if (scancode) {
      int flag;

      if (scancode >= KEY_MODIFIERS) {
         flag = modifier_table[scancode - KEY_MODIFIERS];
      } else {
         flag = 0;
      }
      if ((char)keycode < 0) {    /* release */
         pc_key[scancode] = FALSE;
         if (flag & KB_MODIFIERS) {
            key_shifts &= ~flag;
         }
      } else {                    /* keypress */
         pc_key[scancode] = TRUE;
         if (flag & KB_MODIFIERS) {
            key_shifts |= flag;
         }
         if (flag & KB_LED_FLAGS) {
            key_shifts ^= flag;
            update_leds(key_shifts);
         }
         if (scancode < KEY_MODIFIERS) {
            inject_key(scancode);
         }
      }
   }

   in_a_terrupt--;
} ENDOFUNC(handle_code)


static int
keyboard ()
{
   unsigned char temp, scancode;

   temp = inportb(0x60);

   if (temp <= 0xe1) {
      if (key_pause_loop) {
         if (!--key_pause_loop) handle_code(KEY_PAUSE, 0);
      } else
         switch (temp) {
            case 0xe0:
               key_enhanced = TRUE;
               break;
            case 0xe1:
               key_pause_loop = 5;
               break;
            default:
               if (key_enhanced) {
                  key_enhanced = FALSE;
                  scancode = hw_to_mycode_ex[temp & 0x7f];
               } else {
                  scancode = hw_to_mycode[temp & 0x7f];
               }
               handle_code(scancode, temp);
         }
   }

   if (((temp==0x4F)||(temp==0x53))&&(key_shifts&KB_CTRL_FLAG)&&(key_shifts&KB_ALT_FLAG)) {
      /* Hack alert:
       * only SIGINT (but not Ctrl-Break)
       * calls the destructors and will safely clean up
       */
       __asm("\n\
		movb	$0x79, %%al		\n\
		call	___djgpp_hw_exception	\n\
       ":::"%eax", "%ebx", "%ecx", "%edx", "%esi", "%edi", "memory");
   }

   __asm("\n\
		inb	$0x61, %%al	\n\
		movb	%%al, %%ah	\n\
		orb	$0x80, %%al	\n\
		outb	%%al, $0x61	\n\
		xchgb	%%al, %%ah	\n\
		outb	%%al, $0x61	\n\
		movb	$0x20, %%al	\n\
		outb	%%al, $0x20	\n\
   ":::"%eax");
   return 0;
} ENDOFUNC(keyboard)


int
pc_keypressed (void)
{
   return (key_buffer.start!=key_buffer.end);
}


int
pc_readkey (void)
{
   if (keyboard_installed) {
      int key;

      while (key_buffer.start == key_buffer.end) {
         __dpmi_yield();
      }

      DISABLE();
      key = key_buffer.key[key_buffer.start++];
      if (key_buffer.start >= KEY_BUFFER_SIZE)
         key_buffer.start = 0;
      ENABLE();

      return key;
   } else {
      return 0;
   }
}


int
pc_keydown (int code)
{
   return pc_key[code];
}


int
pc_keyshifts (void)
{
   return key_shifts;
}


void
pc_remove_keyb (void)
{
   if (keyboard_installed) {
      int s1, s2, s3;

      keyboard_installed = FALSE;
      pc_clexit(pc_remove_keyb);

      DISABLE();
      _farsetsel(__djgpp_dos_sel);
      _farnspokew(0x41c, _farnspeekw(0x41a));

      s1 = _farnspeekb(0x417) & 0x80;
      s2 = _farnspeekb(0x418) & 0xFC;
      s3 = _farnspeekb(0x496) & 0xF3;

      if (pc_key[KEY_RSHIFT])   { s1 |= 1; }
      if (pc_key[KEY_LSHIFT])   { s1 |= 2; }
      if (pc_key[KEY_LCONTROL]) { s2 |= 1; s1 |= 4; }
      if (pc_key[KEY_ALT])      { s1 |= 8; s2 |= 2; }
      if (pc_key[KEY_RCONTROL]) { s1 |= 4; s3 |= 4; }
      if (pc_key[KEY_ALTGR])    { s1 |= 8; s3 |= 8; }

      if (key_shifts & KB_SCROLOCK_FLAG) s1 |= 16;
      if (key_shifts & KB_NUMLOCK_FLAG)  s1 |= 32;
      if (key_shifts & KB_CAPSLOCK_FLAG) s1 |= 64;

      _farnspokeb(0x417, s1);
      _farnspokeb(0x418, s2);
      _farnspokeb(0x496, s3);
      update_leds(key_shifts);

      ENABLE();
      pc_remove_irq(KEYB_IRQ);
   }
}


int
pc_install_keyb (void)
{
   if (keyboard_installed || pc_install_irq(KEYB_IRQ, keyboard)) {
      return -1;
   } else {
      int s1, s2, s3;

      LOCKDATA(key_buffer);
      LOCKDATA(key_enhanced);
      LOCKDATA(key_pause_loop);
      LOCKDATA(key_shifts);
      LOCKDATA(leds_ok);
      LOCKDATA(in_a_terrupt);
      LOCKDATA(pc_key);
      LOCKDATA(modifier_table);
      LOCKDATA(hw_to_mycode);
      LOCKDATA(hw_to_mycode_ex);
      LOCKDATA(standard_key_ascii_table);
      LOCKDATA(standard_key_capslock_table);
      LOCKDATA(standard_key_shift_table);
      LOCKDATA(standard_key_control_table);
      LOCKDATA(numlock_table);
      LOCKFUNC(update_leds);
      LOCKFUNC(inject_key);
      LOCKFUNC(handle_code);
      LOCKFUNC(keyboard);

      DISABLE();
      _farsetsel(__djgpp_dos_sel);
      _farnspokew(0x41c, _farnspeekw(0x41a));

      key_shifts = 0;
      s1 = _farnspeekb(0x417);
      s2 = _farnspeekb(0x418);
      s3 = _farnspeekb(0x496);

      if (s1 & 1) { key_shifts |= KB_SHIFT_FLAG; pc_key[KEY_RSHIFT]   = TRUE; }
      if (s1 & 2) { key_shifts |= KB_SHIFT_FLAG; pc_key[KEY_LSHIFT]   = TRUE; }
      if (s2 & 1) { key_shifts |= KB_CTRL_FLAG;  pc_key[KEY_LCONTROL] = TRUE; }
      if (s2 & 2) { key_shifts |= KB_ALT_FLAG;   pc_key[KEY_ALT]      = TRUE; }
      if (s3 & 4) { key_shifts |= KB_CTRL_FLAG;  pc_key[KEY_RCONTROL] = TRUE; }
      if (s3 & 8) { key_shifts |= KB_ALT_FLAG;   pc_key[KEY_ALTGR]    = TRUE; }

      if (s1 & 16) key_shifts |= KB_SCROLOCK_FLAG;
      if (s1 & 32) key_shifts |= KB_NUMLOCK_FLAG;
      if (s1 & 64) key_shifts |= KB_CAPSLOCK_FLAG;
      update_leds(key_shifts);

      key_enhanced = key_pause_loop = 0;
      key_buffer.start = key_buffer.end = 0;
      ENABLE();

      pc_atexit(pc_remove_keyb);
      keyboard_installed = TRUE;
      return 0;
   }
}
