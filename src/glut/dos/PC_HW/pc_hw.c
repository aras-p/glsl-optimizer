/*
 * PC/HW routine collection v1.0 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>
#include <fcntl.h>
#include <sys/stat.h> /* for mode definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pc_hw.h"

/*
 * atexit
 */
#define MAX_ATEXIT 32

static volatile int atexitcnt;
static VFUNC atexittbl[MAX_ATEXIT];

static void __attribute__((destructor)) doexit (void)
{
 while (atexitcnt) atexittbl[--atexitcnt]();
}

int pc_clexit (VFUNC f)
{
 int i;

 for (i=0;i<atexitcnt;i++) {
     if (atexittbl[i]==f) {
        for (atexitcnt--;i<atexitcnt;i++) atexittbl[i] = atexittbl[i+1];
        atexittbl[i] = 0;
        return 0;
     }
 }
 return -1;
}

int pc_atexit (VFUNC f)
{
 pc_clexit(f);
 if (atexitcnt<MAX_ATEXIT) {
    atexittbl[atexitcnt++] = f;
    return 0;
 }
 return -1;
}

/*
 * locked memory allocation
 */
void *pc_malloc (size_t size)
{
 void *p = malloc(size);

 if (p) {
    if (_go32_dpmi_lock_data(p, size)) {
       free(p);
       return NULL;
    }
 }

 return p;
}

/*
 * standard redirection
 */
static char errname[L_tmpnam];
static char outname[L_tmpnam];

static int h_out, h_outbak;
static int h_err, h_errbak;

void pc_open_stderr (void)
{
 if (tmpnam(errname)) {
    h_err = open(errname, O_WRONLY |/* O_BINARY |*/ O_CREAT | O_TRUNC,
                          S_IREAD | S_IWRITE);
    h_errbak = dup(2);
    fflush(stderr);
    dup2(h_err, 2);
 }
}

void pc_close_stderr (void)
{
 FILE *f;
 char *line = alloca(512);
 
 dup2(h_errbak, 2);
 close(h_err);
 close(h_errbak);
 
 if ((f=fopen(errname, "r"))!=NULL) {
    while (fgets(line, 512, f)) {
          fputs(line, stderr);
    }
    fclose(f);
 }

 remove(errname);
}

void pc_open_stdout (void)
{
 if (tmpnam(outname)) {
    h_out = open(outname, O_WRONLY |/* O_BINARY |*/ O_CREAT | O_TRUNC,
                          S_IREAD | S_IWRITE);
    h_outbak = dup(1);
    fflush(stdout);
    dup2(h_out, 1);
 }
}

void pc_close_stdout (void)
{
 FILE *f;
 char *line = alloca(512);
 
 dup2(h_outbak, 1);
 close(h_out);
 close(h_outbak);
 
 if ((f=fopen(outname, "r"))!=NULL) {
    while (fgets(line, 512, f)) {
          fputs(line, stdout);
    }
    fclose(f);
 }

 remove(outname);
}
