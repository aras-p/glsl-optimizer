/*
 * PC/HW routine collection v1.5 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Daniel Borca
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <pc.h>
#include <string.h>

#include "pc_hw.h"

#define TIMER_IRQ 0

#define MAX_TIMERS 8

#define PIT_FREQ 0x1234DD

#define ADJUST(timer, basefreq) timer.counter = PIT_FREQ * timer.freq / SQR(basefreq)

#define unvolatile(__v, __t) __extension__ ({union { volatile __t __cp; __t __p; } __q; __q.__cp = __v; __q.__p;})

static int timer_installed;

typedef struct {
   volatile unsigned int counter, clock_ticks, freq;
   volatile PFUNC func;
   volatile void *parm;
} TIMER;

static TIMER timer_main, timer_func[MAX_TIMERS];


/* Desc: main timer callback
 *
 * In  : -
 * Out : 0 to bypass BIOS, 1 to chain to BIOS
 *
 * Note: -
 */
static int
timer ()
{
   int i;

   for (i = 0; i < MAX_TIMERS; i++) {
      TIMER *t = &timer_func[i];
      if (t->func) {
         t->clock_ticks += t->counter;
         if (t->clock_ticks >= timer_main.counter) {
            t->clock_ticks -= timer_main.counter;
            t->func(unvolatile(t->parm, void *));
         }
      }
   }

   timer_main.clock_ticks += timer_main.counter;
   if (timer_main.clock_ticks >= 0x10000) {
      timer_main.clock_ticks -= 0x10000;
      return 1;
   } else {
      outportb(0x20, 0x20);
      return 0;
   }
} ENDOFUNC(timer)


/* Desc: uninstall timer engine
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
void
pc_remove_timer (void)
{
   if (timer_installed) {
      timer_installed = FALSE;
      pc_clexit(pc_remove_timer);

      DISABLE();
      outportb(0x43, 0x34);
      outportb(0x40, 0);
      outportb(0x40, 0);
      ENABLE();

      pc_remove_irq(TIMER_IRQ);
   }
}


/* Desc: remove timerfunc
 *
 * In  : timerfunc id
 * Out : 0 if success
 *
 * Note: tries to relax the main timer whenever possible
 */
int
pc_remove_int (int fid)
{
   int i;
   unsigned int freq = 0;

   /* are we installed? */
   if (!timer_installed) {
      return -1;
   }

   /* sanity check */
   if ((fid < 0) || (fid >= MAX_TIMERS) || (timer_func[fid].func == NULL)) {
      return -1;
   }
   timer_func[fid].func = NULL;

   /* scan for maximum frequency */
   for (i = 0; i < MAX_TIMERS; i++) {
      TIMER *t = &timer_func[i];
      if (t->func) {
         if (freq < t->freq) {
            freq = t->freq;
         }
      }
   }

   /* if there are no callbacks left, cleanup */
   if (!freq) {
      pc_remove_timer();
      return 0;
   }

   /* if we just lowered the maximum frequency, try to relax the timer engine */
   if (freq < timer_main.freq) {
      unsigned int new_counter = PIT_FREQ / freq;

      DISABLE();

      for (i = 0; i < MAX_TIMERS; i++) {
         if (timer_func[i].func) {
            ADJUST(timer_func[i], freq);
         }
      }

      outportb(0x43, 0x34);
      outportb(0x40, (unsigned char)new_counter);
      outportb(0x40, (unsigned char)(new_counter>>8));
      timer_main.clock_ticks = 0;
      timer_main.counter = new_counter;
      timer_main.freq = freq;

      ENABLE();
   }
 
   return 0;
} ENDOFUNC(pc_remove_int)


/* Desc: adjust timerfunc
 *
 * In  : timerfunc id, new frequency (Hz)
 * Out : 0 if success
 *
 * Note: might change the main timer frequency
 */
int
pc_adjust_int (int fid, unsigned int freq)
{
   int i;

   /* are we installed? */
   if (!timer_installed) {
      return -1;
   }

   /* sanity check */
   if ((fid < 0) || (fid >= MAX_TIMERS) || (timer_func[fid].func == NULL)) {
      return -1;
   }
   timer_func[fid].freq = freq;

   /* scan for maximum frequency */
   freq = 0;
   for (i = 0; i < MAX_TIMERS; i++) {
      TIMER *t = &timer_func[i];
      if (t->func) {
         if (freq < t->freq) {
            freq = t->freq;
         }
      }
   }

   /* update main timer / sons to match highest frequency */
   DISABLE();

   /* using '>' is correct still (and avoids updating
    * the HW timer too often), but doesn't relax the timer!
    */
   if (freq != timer_main.freq) {
      unsigned int new_counter = PIT_FREQ / freq;

      for (i = 0; i < MAX_TIMERS; i++) {
         if (timer_func[i].func) {
            ADJUST(timer_func[i], freq);
         }
      }

      outportb(0x43, 0x34);
      outportb(0x40, (unsigned char)new_counter);
      outportb(0x40, (unsigned char)(new_counter>>8));
      timer_main.clock_ticks = 0;
      timer_main.counter = new_counter;
      timer_main.freq = freq;
   } else {
      ADJUST(timer_func[fid], timer_main.freq);
   }

   ENABLE();

   return 0;
} ENDOFUNC(pc_adjust_int)


/* Desc: install timer engine
 *
 * In  : -
 * Out : 0 for success
 *
 * Note: initial frequency is 18.2 Hz
 */
static int
install_timer (void)
{
   if (timer_installed || pc_install_irq(TIMER_IRQ, timer)) {
      return -1;
   } else {
      memset(timer_func, 0, sizeof(timer_func));

      LOCKDATA(timer_func);
      LOCKDATA(timer_main);
      LOCKFUNC(timer);
      LOCKFUNC(pc_adjust_int);
      LOCKFUNC(pc_remove_int);

      timer_main.counter = 0x10000;

      DISABLE();
      outportb(0x43, 0x34);
      outportb(0x40, 0);
      outportb(0x40, 0);
      timer_main.clock_ticks = 0;
      ENABLE();

      pc_atexit(pc_remove_timer);
      timer_installed = TRUE;
      return 0;
   }
}


/* Desc: install timerfunc
 *
 * In  : callback function, opaque pointer to be passed to callee, freq (Hz)
 * Out : timerfunc id (0 .. MAX_TIMERS-1)
 *
 * Note: returns -1 if error
 */
int
pc_install_int (PFUNC func, void *parm, unsigned int freq)
{
   int i;
   TIMER *t = NULL;

   /* ensure the timer engine is set up */
   if (!timer_installed) {
      if (install_timer()) {
         return -1;
      }
   }

   /* find an empty slot */
   for (i = 0; i < MAX_TIMERS; i++) {
       if (!timer_func[i].func) {
          t = &timer_func[i];
          break;
       }
   }
   if (t == NULL) {
      return -1;
   }

   DISABLE();

   t->func = func;
   t->parm = parm;
   t->freq = freq;
   t->clock_ticks = 0;

   /* update main timer / sons to match highest frequency */
   if (freq > timer_main.freq) {
      unsigned int new_counter = PIT_FREQ / freq;

      for (i = 0; i < MAX_TIMERS; i++) {
         if (timer_func[i].func) {
            ADJUST(timer_func[i], freq);
         }
      }

      outportb(0x43, 0x34);
      outportb(0x40, (unsigned char)new_counter);
      outportb(0x40, (unsigned char)(new_counter>>8));
      timer_main.clock_ticks = 0;
      timer_main.counter = new_counter;
      timer_main.freq = freq;
   } else {
      /* t == &timer_func[i] */
      ADJUST(timer_func[i], timer_main.freq);
   }

   i = t - timer_func;

   ENABLE();

   return i;
}
