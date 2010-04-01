
proc main {argc argv} {
    if {1 != $argc} {
	puts stderr "syntax is: [info script] output.h"
	exit 1
    } 

    set fd [open [lindex $argv 0] w]
    puts $fd "
/*OpenGL primitive typedefs*/
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

typedef long GLintptr;
typedef long GLsizeiptr;
"

}
main $::argc $::argv
