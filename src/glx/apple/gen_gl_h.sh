#!/bin/bash

INFILE=$1
OUTFILE=$2

generate_macros() {
    grep gl.*ProcPtr /System/Library/Frameworks/OpenGL.framework/Headers/gl{,ext}.h | sed 's:^.*\(gl.*Ptr\).*$:\1:' | sort -u | perl -ne 'chomp($_); $s = "PFN".uc($_); $s =~ s/PROCPTR/PROC/; print "#define ".$_." ".$s."\n"'
}

generate_function_pointers() {
    { 
      echo "#define GL_GLEXT_FUNCTION_POINTERS 1"
      echo "#define GL_GLEXT_LEGACY 1"
      generate_macros
      echo '#include "/System/Library/Frameworks/OpenGL.framework/Headers/gl.h"'
    } | ${CC:-gcc} -E - | grep typedef.*PFN
}

cat ${INFILE} | while IFS= read LINE ; do
  case $LINE in
    "@CGL_MESA_COMPAT_MACROS@")
      generate_macros
    ;;
    "@CGL_MESA_FUNCTION_POINTERS@")
      if ! grep -q GL_GLEXT_PROTOTYPES /System/Library/Frameworks/OpenGL.framework/Headers/gl.h ; then
        generate_function_pointers
      fi
    ;;
    *)
      printf "${LINE}\n"
    ;;
  esac
done > ${OUTFILE}
