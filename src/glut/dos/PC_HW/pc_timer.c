/*
 * PC/HW routine collection v0.1 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <pc.h>

#include "pc_hw.h"

#define TIMER_IRQ 0

#define MAX_TIMERS 8

#define PIT_FREQ 0x1234DD

#define unvolatile(__v, __t) __extension__ ({union { volatile __t __cp; __t __p; } __q; __q.__cp = __v; __q.__p;})

static int timer_installed;

typedef struct {
        volatile unsigned int counter, clock_ticks, freq;
        volatile PFUNC func;
        volatile void *parm;
} TIMER;

TIMER timer_main, timer_func[MAX_TIMERS];

static int timer ()
{
 int i;

 for (i=0;i<MAX_TIMERS;i++) {
     TIMER *t = &timer_func[i];
     if (t->func) {
        t->clock_ticks += t->counter;
        if (t->clock_ticks>=timer_main.counter) {
           t->clock_ticks -= timer_main.counter;
           t->func(unvolatile(t->parm, void *));
        }
     }
 }

 timer_main.clock_ticks += timer_main.counter;
 if (timer_main.clock_ticks>=0x10000) {
    timer_main.clock_ticks -= 0x10000;
    return 1;
 } else {
    outportb(0x20, 0x20);
    return 0;
 }
} ENDOFUNC(timer)

void pc_remove_timer (void)
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

static int install_timer (void)
{
 if (timer_installed||pc_install_irq(TIMER_IRQ, timer)) {
    return -1;
 } else {
    LOCKDATA(timer_func);
    LOCKDATA(timer_main);
    LOCKFUNC(timer);

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

static TIMER *find_slot (PFUNC func)
{
 int i;

 for (i=0;i<MAX_TIMERS;i++) {
     if (timer_func[i].func==func) {
        return &timer_func[i];
     }
 }
 for (i=0;i<MAX_TIMERS;i++) {
     if (!timer_func[i].func) {
        return &timer_func[i];
     }
 }

 return NULL;
}

int pc_install_int (PFUNC func, void *parm, unsigned int freq)
{
 int i;
 TIMER *t;

 if (!timer_installed) {
    if (install_timer()) {
       return -1;
    }
 }

 if ((t=find_slot(func))!=NULL) {
    unsigned int new_counter = PIT_FREQ / freq;

    DISABLE();

    t->func = func;
    t->parm = parm;
    t->freq = freq;
    t->clock_ticks = 0;

    if (new_counter < timer_main.counter) {
       for (i=0;i<MAX_TIMERS;i++) {
           if (timer_func[i].func) {
              timer_func[i].counter = new_counter * timer_func[i].freq / freq;
           }
       }
       outportb(0x43, 0x34);
       outportb(0x40, (unsigned char)new_counter);
       outportb(0x40, (unsigned char)(new_counter>>8));
       timer_main.clock_ticks = 0;
       timer_main.counter = new_counter;
       timer_main.freq = freq;
    } else {
       t->counter = PIT_FREQ * freq / (timer_main.freq * timer_main.freq);
    }

    ENABLE();

    return 0;
 }
 
 return -1;
}
