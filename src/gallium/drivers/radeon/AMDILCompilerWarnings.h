//===-- AMDILCompilerWarnings.h - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
#ifndef _AMDIL_COMPILER_WARNINGS_H_
#define _AMDIL_COMPILER_WARNINGS_H_
/// Compiler backend generated warnings that might cause
/// issues with compilation. These warnings become errors if
/// -Werror is specified on the command line.
namespace amd {

#define LIMIT_BARRIER 0
#define BAD_BARRIER_OPT 1
#define RECOVERABLE_ERROR 2
#define NUM_WARN_MESSAGES 3
    /// All warnings must be prefixed with the W token or they might be
    /// treated as errors.
    static const char *CompilerWarningMessage[NUM_WARN_MESSAGES] =
    {
        "W000:Barrier caused limited groupsize",
        "W001:Dangerous Barrier Opt Detected! ",
        "W002:Recoverable BE Error Detected!  "

    };
}

#endif // _AMDIL_COMPILER_WARNINGS_H_
