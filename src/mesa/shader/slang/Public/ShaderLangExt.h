//
//Copyright (C) 2002-2004  3Dlabs Inc. Ltd.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.
//
#ifndef _SHADERLANG_EXTENSION_INCLUDED_
#define _SHADERLANG_EXTENSION_INCLUDED_

#include "ShaderLang.h"

//
// This is the platform independent interface between an OGL driver
// and the shading language compiler/linker.
//

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum  {
    EReserved = 0,
    EFixed,
    ERecommended,
    EFloating
} ShPriority;

typedef enum  {
    ESymbol = 0,
    EFloatConst, 
    EFloatConstPtr, 
    EIntConst, 
    EIntConstPtr, 
    EBoolConst, 
    EBoolConstPtr 
} ShDataType;

// this definition will eventually go once we move to the new linker interface in the driver
typedef enum {
    EVirtualBinding,
    EPhysicalBinding
} ShVirtualPhysicalBinding;

typedef struct {
    int size;         // out - total physical size for the binding in floats - P10
    int location;     // in-out - virtualLocation for all cases
    int functionPriority; // out - used for giving priorities to function bindings
    int proEpilogue;  // out - essentially a bool defining whether its a prologue/epilogue or not, 1 means it is
    int builtInName;  // out - basically a bool value, 0 means not a builtInName, 1 means its a builtInName
    int arraySize;    // out - size of the array in units of its type - if the binding is for an array
    ShPriority bindingPriority;  // in-out - EFixed, ERecommended, EFloating
    ShDataType bindingDataType;  // in-out - whether its a symbol name or a constant value
    ShBasicType hardwareDataType; // out - bool are loaded as floats on the hardware
    ShBasicType userDataType;    // out - mat3 -> mat3, ivec2 -> ivec2, vec2 -> vec2
    ShBasicType basicUserType;   // out - mat3 -> float, ivec2 ->int, vec2 -> float
    int sizeOfType;   // out - for vec3 -> 3, for float -> 1, for mat3 -> 3, mat3[10] -> 3
    int matrix;       // out - essentially a boolean, 0 means vector, 1 means matrix
    union { // in-out
        char* name;   
        float floatVal;
        float* floatValPtr;
        int intVal;
        int* intValPtr; 
    };
    // A pointer to ShP10PhysicalBinding or ShP20PhysicalBinding
    void* targetDependentData; // in-out
} ShBindingExt;

//
// to specify the type of binding
//
typedef enum { 
    EAttribute, 
    EUniform, 
    EVarying, 
    EFunction, 
    EConstant, 
    EFunctionRelocation, 
    EArbVertexLocal,
    EArbVertexEnv,
    EArbFragmentLocal,
    EArbFragmentEnv,
    EState,
    ENoBindingType } ShBindingType;

typedef struct {
    // a pointer to ShBindingExt
    ShBindingExt* pBinding;
    int numOfBindings;
    ShBindingType type;
} ShBindingTableExt;

typedef struct {
    ShBindingTableExt *bindingTable;
    int numOfBindingTables;
} ShBindingList;

SH_IMPORT_EXPORT ShHandle ShConstructBindings();
SH_IMPORT_EXPORT ShHandle ShConstructLibrary();

SH_IMPORT_EXPORT ShHandle ShAddBinding(ShHandle bindingHandle, 
                                       ShBindingExt* binding, 
                                       ShBindingType type);
SH_IMPORT_EXPORT ShHandle ShAddBindingTable(ShHandle bindingHandle, 
                                       ShBindingTableExt *bindingTable, 
                                       ShBindingType type);

SH_IMPORT_EXPORT int ShLinkExt(
    const ShHandle,               // linker object
    const ShHandle h[],           // compiler objects to link together
    const int numHandles);

SH_IMPORT_EXPORT ShBindingList* ShGetBindingList(const ShHandle linkerHandle); 
SH_IMPORT_EXPORT ShBindingTableExt* ShGetBindingTable(const ShHandle linkerHandle, ShBindingType type); 
SH_IMPORT_EXPORT int ShGetUniformLocationExt(const ShHandle linkerHandle, const char* name);
SH_IMPORT_EXPORT int ShSetFixedAttributeBindingsExt(const ShHandle, const ShBindingTableExt*);
SH_IMPORT_EXPORT int ShGetUniformLocationExt2(const ShHandle handle, const char* name, int* location, int* offset);
SH_IMPORT_EXPORT int ShSetVirtualLocation(const ShHandle handle, ShBindingType type, int bindingIndex, int virtualLocation);
SH_IMPORT_EXPORT int ShGetVirtualLocation(const ShHandle handle, ShBindingType type, int bindingIndex, int *virtualLocation);
// 
// To get the bindings object from the linker object (after the link is done so that 
// bindings can be added to the list)
//
SH_IMPORT_EXPORT ShHandle ShGetBindings(const ShHandle linkerHandle);
SH_IMPORT_EXPORT int ShAddLibraryCode(ShHandle library, const char* name, const ShHandle objectCodes[], const int numHandles);

/*****************************************************************************
  This code is used by the new shared linker
 *****************************************************************************/
//
// Each programmable unit has a UnitExecutable.  Targets may subclass
// and append to this as desired.
//
typedef struct {
    int name;         // name of unit to which executable is targeted
    int entry;          // a target specific entry point
    int count;          // size of executable
    const void* code;   // read-only code
} ShUnitExecutable;

//
// The "void*" returned from ShGetExecutable() will be an ShExecutable
//
typedef struct {
    int count;  // count of unit executables
    ShUnitExecutable* executables;
} ShExecutable;

SH_IMPORT_EXPORT ShExecutable* ShGetExecutableExt(const ShHandle linkerHandle);

typedef struct {
    int numThread;
    int stackSpacePerThread;
    int visBufferValidity; // essenatially a boolean
    int shaderFragTerminationStatus; // essentially a boolean
} ShDeviceInfo;

#ifdef __cplusplus
    }
#endif

#endif // _SHADERLANG_EXTENSION_INCLUDED_
