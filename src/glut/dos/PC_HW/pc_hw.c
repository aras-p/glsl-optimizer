/*
 * PC/HW routine collection v1.3 for DOS/DJGPP
 *
 *  Copyright (C) 2002 - Daniel Borca
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


static void __attribute__((destructor))
doexit (void)
{
   while (atexitcnt) atexittbl[--atexitcnt]();
}


int
pc_clexit (VFUNC f)
{
   int i;

   for (i = 0; i < atexitcnt; i++) {
      if (atexittbl[i] == f) {
         for (atexitcnt--; i < atexitcnt; i++) atexittbl[i] = atexittbl[i+1];
         atexittbl[i] = 0;
         return 0;
      }
   }
   return -1;
}


int
pc_atexit (VFUNC f)
{
   pc_clexit(f);
   if (atexitcnt < MAX_ATEXIT) {
      atexittbl[atexitcnt++] = f;
      return 0;
   }
   return -1;
}


/*
 * locked memory allocation
 */
void *
pc_malloc (size_t size)
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
static char outname[L_tmpnam];
static int h_out, h_outbak;
static char errname[L_tmpnam];
static int h_err, h_errbak;


int
pc_open_stdout (void)
{
   tmpnam(outname);

   if ((h_out=open(outname, O_WRONLY | O_CREAT | O_TEXT | O_TRUNC, S_IREAD | S_IWRITE)) > 0) {
      h_outbak = dup(STDOUT_FILENO);
      fflush(stdout);
      dup2(h_out, STDOUT_FILENO);
   }

   return h_out;
}


void
pc_close_stdout (void)
{
   FILE *f;
   char *line = alloca(512);

   if (h_out > 0) {
      dup2(h_outbak, STDOUT_FILENO);
      close(h_out);
      close(h_outbak);

      f = fopen(outname, "rt");
      while (fgets(line, 512, f)) {
         fputs(line, stdout);
      }
      fclose(f);

      remove(outname);
   }
}


int
pc_open_stderr (void)
{
   tmpnam(errname);

   if ((h_err=open(errname, O_WRONLY | O_CREAT | O_TEXT | O_TRUNC, S_IREAD | S_IWRITE)) > 0) {
      h_errbak = dup(STDERR_FILENO);
      fflush(stderr);
      dup2(h_err, STDERR_FILENO);
   }

   return h_err;
}


void
pc_close_stderr (void)
{
   FILE *f;
   char *line = alloca(512);

   if (h_err > 0) {
      dup2(h_errbak, STDERR_FILENO);
      close(h_err);
      close(h_errbak);

      f = fopen(errname, "rt");
      while (fgets(line, 512, f)) {
         fputs(line, stderr);
      }
      fclose(f);

      remove(errname);
   }
}
