
/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <GL/glut.h>
#include "glutint.h"
#include "glutState.h"

void
__glutWarning(char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Warning in %s: ",
    gState.programName ? gState.programName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
}

/* CENTRY */
void APIENTRY 
glutReportErrors(void)
{
  GLenum error;

  while ((error = glGetError()) != GL_NO_ERROR)
    __glutWarning("GL error: %s", gluErrorString(error));
}
/* ENDCENTRY */

void
__glutFatalError(char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Fatal Error in %s: ",
    gState.programName ? gState.programName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
  exit(1);
}

void
__glutFatalUsage(char *format,...)
{
  va_list args;

  va_start(args, format);
  fprintf(stderr, "GLUT: Fatal API Usage in %s: ",
    gState.programName ? gState.programName : "(unamed)");
  vfprintf(stderr, format, args);
  va_end(args);
  putc('\n', stderr);
  abort();
}
