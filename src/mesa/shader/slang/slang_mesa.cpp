/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "slang_mesa.h"
#include "Initialisation.h"
#include "Include/Common.h"
#include "Include/ShHandle.h"
#include "Public/ShaderLang.h"


class TGenericCompiler: public TCompiler
{
public:
	TGenericCompiler (EShLanguage l, int dOptions): TCompiler(l, infoSink), debugOptions(dOptions)
	{
	}
public:
	virtual bool compile (TIntermNode *root)
	{
		haveValidObjectCode = true;
		return haveValidObjectCode;
	}
	TInfoSink infoSink;
	int debugOptions;
};

TCompiler *ConstructCompiler (EShLanguage language, int debugOptions)
{
	return new TGenericCompiler (language, debugOptions);
}

void DeleteCompiler (TCompiler *compiler)
{
	delete compiler;
}

class TGenericLinker: public TLinker
{
public:
	TGenericLinker (EShExecutable e, int dOptions): TLinker(e, infoSink), debugOptions(dOptions)
	{
	}
public:
	bool link (TCompilerList &, TUniformMap *)
	{
		return true;
	}
	void getAttributeBindings (ShBindingTable const **t) const
	{
	}
	TInfoSink infoSink;
	int debugOptions;
};

TShHandleBase *ConstructLinker (EShExecutable executable, int debugOptions)
{
	return new TGenericLinker (executable, debugOptions);
}

void DeleteLinker (TShHandleBase *linker)
{
	delete linker;
}

class TUniformLinkedMap: public TUniformMap
{
public:
	TUniformLinkedMap()
	{
	}
public:
	virtual int getLocation (const char *name)
	{
		return 0;
	}
};

TUniformMap *ConstructUniformMap ()
{
	return new TUniformLinkedMap;
}

void DeleteUniformMap (TUniformMap *map)
{
	delete map;
}


namespace std
{

void _Xran ()
{
	/* XXX fix this under Linux */
	/*_THROW(out_of_range, "invalid string position");*/
}

void _Xlen ()
{
	/* XXX fix this under Linux */
	/*_THROW(length_error, "string too long");*/
}

}


/* these functions link with extern "C" */

int _mesa_isalnum (char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

int _glslang_3dlabs_InitProcess ()
{
	return InitProcess () ? 1 : 0;
}

int _glslang_3dlabs_ShInitialize ()
{
	return ShInitialize () ? 1 : 0;
}

