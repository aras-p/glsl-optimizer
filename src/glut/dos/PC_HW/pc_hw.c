/*
 * PC/HW routine collection v1.2 for DOS/DJGPP
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
static int h_out, h_outbak, h_err, h_errbak;

int pc_open_stdout (void)
{
 if ((h_out=open(tmpnam(NULL), O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_TEMPORARY, S_IRUSR | S_IWUSR)) >= 0) {
    if ((h_outbak=dup(1)) != -1) {
       fflush(stdout);
       if (dup2(h_out, 1) != -1) {
          return 0;
       }
       close(h_outbak);
    }
    close(h_out);
 }
 return (h_out = -1);
}

void pc_close_stdout (void)
{
 FILE *f;
 char *line = alloca(512);

 if (h_out >= 0) {
    dup2(h_outbak, 1);
    close(h_outbak);

    if ((f=fdopen(h_out, "r")) != NULL) {
       fseek(f, 0, SEEK_SET);
       while (fgets(line, 512, f)) {
             fputs(line, stdout);
       }
       fclose(f);
    } else {
       close(h_out);
    }
 }
}

int pc_open_stderr (void)
{
 if ((h_err=open(tmpnam(NULL), O_WRONLY | O_CREAT | O_TRUNC | O_TEXT | O_TEMPORARY, S_IRUSR | S_IWUSR)) >= 0) {
    if ((h_errbak=dup(2)) != -1) {
       fflush(stderr);
       if (dup2(h_err, 2) != -1) {
          return 0;
       }
       close(h_errbak);
    }
    close(h_err);
 }
 return (h_err = -1);
}

void pc_close_stderr (void)
{
 FILE *f;
 char *line = alloca(512);

 if (h_err >= 0) {
    dup2(h_errbak, 2);
    close(h_errbak);

    if ((f=fdopen(h_err, "r")) != NULL) {
       fseek(f, 0, SEEK_SET);
       while (fgets(line, 512, f)) {
             fputs(line, stderr);
       }
       fclose(f);
    } else {
       close(h_err);
    }
 }
}
