//
//Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
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

#include "ParseHelper.h"
#include "Include/InitializeParseContext.h"
#include "osinclude.h"
#include <stdarg.h>
///////////////////////////////////////////////////////////////////////
//
// Sub- vector and matrix fields
//
////////////////////////////////////////////////////////////////////////

//
// Look at a '.' field selector string and change it into offsets
// for a vector.
//
bool TParseContext::parseVectorFields(const TString& compString, int vecSize, TVectorFields& fields, int line)
{
    fields.num = (int) compString.size();
    if (fields.num > 4) {
        error(line, "illegal vector field selection", compString.c_str(), "");
        return false;
    }

    enum {
        exyzw,
        ergba,
        estpq
    } fieldSet[4];

    for (int i = 0; i < fields.num; ++i) {
        switch (compString[i])  {
        case 'x': 
            fields.offsets[i] = 0;
            fieldSet[i] = exyzw;
            break;
        case 'r': 
            fields.offsets[i] = 0;
            fieldSet[i] = ergba;
            break;
        case 's':
            fields.offsets[i] = 0;
            fieldSet[i] = estpq;
            break;
        case 'y': 
            fields.offsets[i] = 1;
            fieldSet[i] = exyzw;
            break;
        case 'g': 
            fields.offsets[i] = 1;
            fieldSet[i] = ergba;
            break;
        case 't':
            fields.offsets[i] = 1;
            fieldSet[i] = estpq;
            break;
        case 'z': 
            fields.offsets[i] = 2;
            fieldSet[i] = exyzw;
            break;
        case 'b': 
            fields.offsets[i] = 2;
            fieldSet[i] = ergba;
            break;
        case 'p':
            fields.offsets[i] = 2;
            fieldSet[i] = estpq;
            break;
        
        case 'w': 
            fields.offsets[i] = 3;
            fieldSet[i] = exyzw;
            break;
        case 'a': 
            fields.offsets[i] = 3;
            fieldSet[i] = ergba;
            break;
        case 'q':
            fields.offsets[i] = 3;
            fieldSet[i] = estpq;
            break;
        default:
            error(line, "illegal vector field selection", compString.c_str(), "");
            return false;
        }
    }

    for (int i = 0; i < fields.num; ++i) {
        if (fields.offsets[i] >= vecSize) {
        error(line, "vector field selection out of range",  compString.c_str(), "");
        return false;
    }

        if (i > 0) {
            if (fieldSet[i] != fieldSet[i-1]) {
                error(line, "illegal - vector component fields not from the same set", compString.c_str(), "");
                return false;
            }
        }
    }

    return true;
}


//
// Look at a '.' field selector string and change it into offsets
// for a matrix.
//
bool TParseContext::parseMatrixFields(const TString& compString, int matSize, TMatrixFields& fields, int line)
{
    fields.wholeRow = false;
    fields.wholeCol = false;
    fields.row = -1;
    fields.col = -1;

    if (compString.size() != 2) {
        error(line, "illegal length of matrix field selection", compString.c_str(), "");
        return false;
    }

    if (compString[0] == '_') {
        if (compString[1] < '0' || compString[1] > '3') {
            error(line, "illegal matrix field selection", compString.c_str(), "");
            return false;
        }
        fields.wholeCol = true;
        fields.col = compString[1] - '0';
    } else if (compString[1] == '_') {
        if (compString[0] < '0' || compString[0] > '3') {
            error(line, "illegal matrix field selection", compString.c_str(), "");
            return false;
        }
        fields.wholeRow = true;
        fields.row = compString[0] - '0';
    } else {
        if (compString[0] < '0' || compString[0] > '3' ||
            compString[1] < '0' || compString[1] > '3') {
            error(line, "illegal matrix field selection", compString.c_str(), "");
            return false;
        }
        fields.row = compString[0] - '0';
        fields.col = compString[1] - '0';
    }

    if (fields.row >= matSize || fields.col >= matSize) {
        error(line, "matrix field selection out of range", compString.c_str(), "");
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////
//
// Errors
//
////////////////////////////////////////////////////////////////////////

//
// Track whether errors have occurred.
//
void TParseContext::recover()
{
    recoveredFromError = true;
}

//
// Used by flex/bison to output all syntax and parsing errors.
//
void C_DECL TParseContext::error(TSourceLoc nLine, const char *szReason, const char *szToken, 
                                 const char *szExtraInfoFormat, ...)
{
    char szExtraInfo[400];
    va_list marker;
    
    va_start(marker, szExtraInfoFormat);
    
    _vsnprintf(szExtraInfo, sizeof(szExtraInfo), szExtraInfoFormat, marker);
    
    /* VC++ format: file(linenum) : error #: 'token' : extrainfo */
    infoSink.info.prefix(EPrefixError);
    infoSink.info.location(nLine);
    infoSink.info << "'" << szToken <<  "' : " << szReason << " " << szExtraInfo << "\n";
    
    va_end(marker);

    ++numErrors;
}

//
// Same error message for all places assignments don't work.
//
void TParseContext::assignError(int line, const char* op, TString left, TString right)
{
    error(line, "", op, "cannot convert from '%s' to '%s'",
          right.c_str(), left.c_str());
}

//
// Same error message for all places unary operations don't work.
//
void TParseContext::unaryOpError(int line, char* op, TString operand)
{
   error(line, " wrong operand type", op, 
          "no operation '%s' exists that takes an operand of type %s (or there is no acceptable conversion)",
          op, operand.c_str());
}

//
// Same error message for all binary operations don't work.
//
void TParseContext::binaryOpError(int line, char* op, TString left, TString right)
{
    error(line, " wrong operand types ", op, 
            "no operation '%s' exists that takes a left-hand operand of type '%s' and "
            "a right operand of type '%s' (or there is no acceptable conversion)", 
            op, left.c_str(), right.c_str());
}

//
// Both test and if necessary, spit out an error, to see if the node is really
// an l-value that can be operated on this way.
//
// Returns true if the was an error.
//
bool TParseContext::lValueErrorCheck(int line, char* op, TIntermTyped* node)
{
    TIntermSymbol* symNode = node->getAsSymbolNode();
    TIntermBinary* binaryNode = node->getAsBinaryNode();

    if (binaryNode) {
        bool errorReturn;

        switch(binaryNode->getOp()) {
        case EOpIndexDirect:
        case EOpIndexIndirect:
        case EOpIndexDirectStruct:
            return lValueErrorCheck(line, op, binaryNode->getLeft());
        case EOpVectorSwizzle:
            errorReturn = lValueErrorCheck(line, op, binaryNode->getLeft());
            if (!errorReturn) {
                int offset[4] = {0,0,0,0};

                TIntermTyped* rightNode = binaryNode->getRight();
                TIntermAggregate *aggrNode = rightNode->getAsAggregate();
                
                for (TIntermSequence::iterator p = aggrNode->getSequence().begin(); 
                                               p != aggrNode->getSequence().end(); p++) {
                    int value = (*p)->getAsTyped()->getAsConstantUnion()->getUnionArrayPointer()->iConst;
                    offset[value]++;     
                    if (offset[value] > 1) {
                        error(line, " l-value of swizzle cannot have duplicate components", op, "", "");

                        return true;
                    }
                }
            } 

            return errorReturn;
        default: 
            break;
        }
        error(line, " l-value required", op, "", "");

        return true;
    }


    const char* symbol = 0;
    if (symNode != 0)
        symbol = symNode->getSymbol().c_str();

    char* message = 0;
    switch (node->getQualifier()) {
    case EvqConst:          message = "can't modify a const";        break;
    case EvqConstReadOnly:  message = "can't modify a const";        break;
    case EvqAttribute:      message = "can't modify an attribute";   break;
    case EvqUniform:        message = "can't modify a uniform";      break;
    case EvqVaryingIn:      message = "can't modify a varying";      break;
    case EvqInput:          message = "can't modify an input";       break;
    case EvqFace:           message = "can't modify gl_FrontFace";   break;
    case EvqFragCoord:      message = "can't modify gl_FragCoord";   break;
    default:

        //
        // Type that can't be written to?
        //
        switch (node->getBasicType()) {
        case EbtSampler1D:
        case EbtSampler2D:
        case EbtSampler3D:
        case EbtSamplerCube:
        case EbtSampler1DShadow:
        case EbtSampler2DShadow:
            message = "can't modify a sampler";
            break;
        case EbtVoid:
            message = "can't modify void";
            break;
        default: 
            break;
        }
    }

    if (message == 0 && binaryNode == 0 && symNode == 0) {
        error(line, " l-value required", op, "", "");

        return true;
    }


    //
    // Everything else is okay, no error.
    //
    if (message == 0)
        return false;

    //
    // If we get here, we have an error and a message.
    //
    if (symNode)
        error(line, " l-value required", op, "\"%s\" (%s)", symbol, message);
    else
        error(line, " l-value required", op, "(%s)", message);

    return true;
}

//
// Both test, and if necessary spit out an error, to see if the node is really
// a constant.
//
// Returns true if the was an error.
//
bool TParseContext::constErrorCheck(TIntermTyped* node)
{
    if (node->getQualifier() == EvqConst)
        return false;

    error(node->getLine(), "constant expression required", "", "");

    return true;
}

//
// Both test, and if necessary spit out an error, to see if the node is really
// an integer.
//
// Returns true if the was an error.
//
bool TParseContext::integerErrorCheck(TIntermTyped* node, char* token)
{
    if (node->getBasicType() == EbtInt && node->getNominalSize() == 1)
        return false;

    error(node->getLine(), "integer expression required", token, "");

    return true;
}

//
// Both test, and if necessary spit out an error, to see if we are currently
// globally scoped.
//
// Returns true if the was an error.
//
bool TParseContext::globalErrorCheck(int line, bool global, char* token)
{
    if (global)
        return false;

    error(line, "only allowed at global scope", token, "");

    return true;
}

//
// For now, keep it simple:  if it starts "gl_", it's reserved, independent
// of scope.  Except, if the symbol table is at the built-in push-level,
// which is when we are parsing built-ins.
//
// Returns true if there was an error.
//
bool TParseContext::reservedErrorCheck(int line, const TString& identifier)
{
    if (symbolTable.atBuiltInLevel() ||
        identifier.substr(0, 3) != TString("gl_"))
        return false;

    error(line, "reserved built-in name", "gl_", "");

    return true;        
}

//
// Make sure there is enough data provided to the constructor to build
// something of the type of the constructor.  Also returns the type of
// the constructor.
//
// Returns true if there was an error in construction.
//
bool TParseContext::constructorErrorCheck(int line, TIntermNode* node, TFunction& function, TOperator op, TType* type)
{
    switch(op) {
    case EOpConstructInt:     *type = TType(EbtInt);                               break;
    case EOpConstructBool:    *type = TType(EbtBool);                              break;
    case EOpConstructFloat:   *type = TType(EbtFloat);                             break;
    case EOpConstructVec2:    *type = TType(EbtFloat, EvqTemporary, 2);            break;
    case EOpConstructVec3:    *type = TType(EbtFloat, EvqTemporary, 3);            break;
    case EOpConstructVec4:    *type = TType(EbtFloat, EvqTemporary, 4);            break;
    case EOpConstructBVec2:   *type = TType(EbtBool,  EvqTemporary, 2);            break;
    case EOpConstructBVec3:   *type = TType(EbtBool,  EvqTemporary, 3);            break;
    case EOpConstructBVec4:   *type = TType(EbtBool,  EvqTemporary, 4);            break;
    case EOpConstructIVec2:   *type = TType(EbtInt,   EvqTemporary, 2);            break;
    case EOpConstructIVec3:   *type = TType(EbtInt,   EvqTemporary, 3);            break;
    case EOpConstructIVec4:   *type = TType(EbtInt,   EvqTemporary, 4);            break;
    case EOpConstructMat2:    *type = TType(EbtFloat, EvqTemporary, 2, true);      break;
    case EOpConstructMat3:    *type = TType(EbtFloat, EvqTemporary, 3, true);      break;
    case EOpConstructMat4:    *type = TType(EbtFloat, EvqTemporary, 4, true);      break;
    case EOpConstructStruct:  *type = TType(function.getReturnType().getStruct(), function.getReturnType().getTypeName()); break;
    default:
        error(line, "expected constructor", "Internal Error", "");
        return true;
    }

    bool constructingMatrix = false;
    switch(op) {
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
        constructingMatrix = true;
        break;
    default: 
        break;
    }

    //
    // Note: It's okay to have too many components available, but not okay to have unused
    // arguments.  'full' will go to true when enough args have been seen.  If we loop
    // again, there is an extra argument, so 'overfull' will become true.
    //

    int size = 0;
    bool constType = true;
    bool full = false;
    bool overFull = false;
    bool matrixInMatrix = false;
    for (int i = 0; i < function.getParamCount(); ++i) {
        size += function[i].type->getInstanceSize();
        if (constructingMatrix && function[i].type->isMatrix())
            matrixInMatrix = true;
        if (full)
            overFull = true;
        if (op != EOpConstructStruct && size >= type->getInstanceSize())
            full = true;
        if (function[i].type->getQualifier() != EvqConst)
            constType = false;
    }
    
    if (constType)
        type->changeQualifier(EvqConst);

    if (matrixInMatrix) {
        error(line, "constructing matrix from matrix", "constructor", "(reserved)");
        return true;
    }

    if (overFull) {
        error(line, "too many arguments", "constructor", "");
        return true;
    }

    if (size != 1 && size < type->getInstanceSize() || (size < 1) && op == EOpConstructStruct) {
        error(line, "not enough data provided for construction", "constructor", "");
        return true;
    }

    TIntermTyped* typed = node->getAsTyped();
    if (typed == 0) {
        error(line, "constructor argument does not have a type", "constructor", "");
        return true;
    }
    if (op != EOpConstructStruct && IsSampler(typed->getBasicType())) {
        error(line, "cannot convert a sampler", "constructor", "");
        return true;
    }
    if (typed->getBasicType() == EbtVoid) {
        error(line, "cannot convert a void", "constructor", "");
        return true;
    }

    return false;
}

// This function checks to see if a void variable has been declared and raise an error message for such a case
//
// returns true in case of an error
//
bool TParseContext::voidErrorCheck(int line, const TString& identifier, const TPublicType& pubType)
{
    if (pubType.type == EbtVoid) {
        error(line, "illegal use of type 'void'", identifier.c_str(), "");
        return true;
    } 

    return false;
}

// This function checks to see if the node (for the expression) contains a scalar boolean expression or not
//
// returns true in case of an error
//
bool TParseContext::boolErrorCheck(int line, const TIntermTyped* type)
{
    if (type->getBasicType() != EbtBool || type->isArray() || type->isMatrix() || type->isVector()) {
        error(line, "boolean expression expected", "", "");
        return true;
    } 

    return false;
}

// This function checks to see if the node (for the expression) contains a scalar boolean expression or not
//
// returns true in case of an error
//
bool TParseContext::boolErrorCheck(int line, const TPublicType& pType)
{
    if (pType.type != EbtBool || pType.array || pType.matrix || (pType.size > 1)) {
        error(line, "boolean expression expected", "", "");
        return true;
    } 

    return false;
}

bool TParseContext::samplerErrorCheck(int line, const TPublicType& pType, const char* reason)
{
    if (pType.type == EbtStruct) {
        if (containsSampler(*pType.userDef)) {
            error(line, reason, TType::getBasicString(pType.type), "(structure contains a sampler)");
        
            return true;
        }
        
        return false;
    } else if (IsSampler(pType.type)) {
        error(line, reason, TType::getBasicString(pType.type), "");

        return true;
    }

    return false;
}

bool TParseContext::structQualifierErrorCheck(int line, const TPublicType& pType)
{
    if ((pType.qualifier == EvqVaryingIn || pType.qualifier == EvqVaryingOut || pType.qualifier == EvqAttribute) &&
        pType.type == EbtStruct) {
        error(line, "cannot be used with a structure", getQualifierString(pType.qualifier), "");
        
        return true;
    }

    if (pType.qualifier != EvqUniform && samplerErrorCheck(line, pType, "samplers must be uniform"))
        return true;

    return false;
}

bool TParseContext::parameterSamplerErrorCheck(int line, TQualifier qualifier, const TType& type)
{
    if ((qualifier == EvqOut || qualifier == EvqInOut) && 
             type.getBasicType() != EbtStruct && IsSampler(type.getBasicType())) {
        error(line, "samplers cannot be output parameters", type.getBasicString(), "");
        return true;
    }

    return false;
}

bool TParseContext::containsSampler(TType& type)
{
    if (IsSampler(type.getBasicType()))
        return true;

    if (type.getBasicType() == EbtStruct) {
        TTypeList& structure = *type.getStruct();
        for (unsigned int i = 0; i < structure.size(); ++i) {
            if (containsSampler(*structure[i].type))
                return true;
        }
    }

    return false;
}

bool TParseContext::insertBuiltInArrayAtGlobalLevel()
{
    TString *name = NewPoolTString("gl_TexCoord");
    TSymbol* symbol = symbolTable.find(*name);
    if (!symbol) {
        error(0, "INTERNAL ERROR finding symbol", name->c_str(), "");
        return true;
    }
    TVariable* variable = static_cast<TVariable*>(symbol);

    TVariable* newVariable = new TVariable(name, variable->getType());

    if (! symbolTable.insert(*newVariable)) {
        delete newVariable;
        error(0, "INTERNAL ERROR inserting new symbol", name->c_str(), "");
        return true;
    }

    return false;
}

//
// Do all the semantic checking for declaring an array, with and 
// without a size, and make the right changes to the symbol table.
//
// size == 0 means no specified size.
//
// Returns true if there was an error.
//
bool TParseContext::arrayErrorCheck(int line, TString& identifier, TPublicType type, TIntermTyped* size)
{
    //
    // Don't check for reserved word use until after we know it's not in the symbol table,
    // because reserved arrays can be redeclared.
    //

    //
    // Can the type be an array?
    //
    if (type.array || type.qualifier == EvqAttribute || type.qualifier == EvqConst) {
        error(line, "cannot declare arrays of this type", TType(type).getCompleteString().c_str(), "");
        return true;
    }
    type.array = true;

    //
    // size will be 0 if there is no size declared, otherwise it contains the size
    // declared.
    //
    TIntermConstantUnion* constant = 0;
    if (size) {
        constant = size->getAsConstantUnion();
        if (constant == 0 || constant->getBasicType() != EbtInt || constant->getUnionArrayPointer()->iConst <= 0) {
            error(line, "array size must be a positive integer", identifier.c_str(), "");
            return true;
        }
    }

    bool builtIn = false; 
    bool sameScope = false;
    TSymbol* symbol = symbolTable.find(identifier, &builtIn, &sameScope);
    if (symbol == 0 || !sameScope) {
        if (reservedErrorCheck(line, identifier))
            return true;
        
        TVariable* variable = new TVariable(&identifier, TType(type));

        if (size)
            variable->getType().setArraySize(constant->getUnionArrayPointer()->iConst);

        if (! symbolTable.insert(*variable)) {
            delete variable;
            error(line, "INTERNAL ERROR inserting new symbol", identifier.c_str(), "");
            return true;
        }
    } else {
        if (! symbol->isVariable()) {
            error(line, "variable expected", identifier.c_str(), "");
            return true;
        }

        TVariable* variable = static_cast<TVariable*>(symbol);
        if (! variable->getType().isArray()) {
            error(line, "redeclaring non-array as array", identifier.c_str(), "");
            return true;
        }
        if (variable->getType().getArraySize() > 0) {
            error(line, "redeclaration of array with size", identifier.c_str(), "");
            return true;
        }
        
        if (variable->getType() != TType(type)) {
            error(line, "redeclaration of array with a different type", identifier.c_str(), "");
            return true;
        }

        TType* t = variable->getArrayInformationType();
        while (t != 0) {
            if (t->getMaxArraySize() > constant->getUnionArrayPointer()->iConst) {
                error(line, "higher index value already used for the array", identifier.c_str(), "");
                return true;
            }
            t->setArraySize(constant->getUnionArrayPointer()->iConst);
            t = t->getArrayInformationType();
        }

        if (size)
            variable->getType().setArraySize(constant->getUnionArrayPointer()->iConst);
    } 

    if (voidErrorCheck(line, identifier, type))
        return true;

    return false;
}

bool TParseContext::arraySetMaxSize(TIntermSymbol *node, TType* type, int size, bool updateFlag, TSourceLoc line)
{
    bool builtIn = false;
    TSymbol* symbol = symbolTable.find(node->getSymbol(), &builtIn);
    if (symbol == 0) {
        error(line, " undeclared identifier", node->getSymbol().c_str(), "");
        return true;
    }
    TVariable* variable = static_cast<TVariable*>(symbol);

    type->setArrayInformationType(variable->getArrayInformationType());
    variable->updateArrayInformationType(type);

    // we dont want to update the maxArraySize when this flag is not set, we just want to include this 
    // node type in the chain of node types so that its updated when a higher maxArraySize comes in.
    if (!updateFlag)
        return false;

    size++;
    variable->getType().setMaxArraySize(size);
    type->setMaxArraySize(size);
    TType* tt = type;

    while(tt->getArrayInformationType() != 0) {
        tt = tt->getArrayInformationType();
        tt->setMaxArraySize(size);
    }

    return false;
}

//
// Do semantic checking for a variable declaration that has no initializer,
// and update the symbol table.
//
// Returns true if there was an error.
//
bool TParseContext::nonInitErrorCheck(int line, TString& identifier, TPublicType& type)
{
    if (reservedErrorCheck(line, identifier))
        recover();

    //
    // Make the qualifier make sense, error is issued in a little bit.
    //
    bool constError = false;
    if (type.qualifier == EvqConst) {
        type.qualifier = EvqTemporary;
        constError = true;
    }

    TVariable* variable = new TVariable(&identifier, TType(type));

    if (! symbolTable.insert(*variable)) {
        error(line, "redefinition", variable->getName().c_str(), "");
        delete variable;
        return true;
    }
    if (constError) {
        error(line, "variables with qualifier 'const' must be initialized", identifier.c_str(), "");
        return true;
    }

    if (voidErrorCheck(line, identifier, type))
        return true;

    return false;
}

bool TParseContext::paramErrorCheck(int line, TQualifier qualifier, TQualifier paramQualifier, TType* type)
{    
    if (qualifier != EvqConst && qualifier != EvqTemporary) {
        error(line, "qualifier not allowed on function parameter", getQualifierString(qualifier), "");
        return true;
    }
    if (qualifier == EvqConst && paramQualifier != EvqIn) {
        error(line, "qualifier not allowed with ", getQualifierString(qualifier), getQualifierString(paramQualifier));
        return true;
    }

    if (qualifier == EvqConst)
        type->changeQualifier(EvqConstReadOnly);
    else
        type->changeQualifier(paramQualifier);

    return false;
}

/////////////////////////////////////////////////////////////////////////////////
//
// Non-Errors.
//
/////////////////////////////////////////////////////////////////////////////////

//
// Look up a function name in the symbol table, and make sure it is a function.
//
// Return the function symbol if found, otherwise 0.
//
const TFunction* TParseContext::findFunction(int line, TFunction* call, bool *builtIn)
{
    const TSymbol* symbol = symbolTable.find(call->getMangledName(), builtIn);

    if (symbol == 0) {        
        error(line, "no matching overloaded function found", call->getName().c_str(), "");
        return 0;
    }

    if (! symbol->isFunction()) {
        error(line, "function name expected", call->getName().c_str(), "");
        return 0;
    }
    
    const TFunction* function = static_cast<const TFunction*>(symbol);
    
    return function;
}
//
// Initializers show up in several places in the grammar.  Have one set of
// code to handle them here.
//
bool TParseContext::executeInitializer(TSourceLoc line, TString& identifier, TPublicType& pType, 
                                       TIntermTyped* initializer, TIntermNode*& intermNode)
{
    if (reservedErrorCheck(line, identifier))
        return true;

    if (voidErrorCheck(line, identifier, pType))
        return true;

    //
    // add variable to symbol table
    //
    TVariable* variable = new TVariable(&identifier, TType(pType));
    if (! symbolTable.insert(*variable)) {
        error(line, "redefinition", variable->getName().c_str(), "");
        return true;
        // don't delete variable, it's used by error recovery, and the pool 
        // pop will take care of the memory
    }

    //
    // identifier must be of type constant, a global, or a temporary
    //
    TQualifier qualifier = variable->getType().getQualifier();
    if ((qualifier != EvqTemporary) && (qualifier != EvqGlobal) && (qualifier != EvqConst)) {
        error(line, " cannot initialize this type of qualifier ", variable->getType().getQualifierString(), "");
        return true;
    }
    //
    // test for and propagate constant
    //

    if (qualifier == EvqConst) {
        if (qualifier != initializer->getType().getQualifier()) {
            error(line, " assigning non-constant to", "=", "'%s'", variable->getType().getCompleteString().c_str());
            variable->getType().changeQualifier(EvqTemporary);
            return true;
        }
        if (TType(pType) != initializer->getType()) {
            error(line, " non-matching types for const initializer ", 
                variable->getType().getQualifierString(), "");
            variable->getType().changeQualifier(EvqTemporary);
            return true;
        }
        if (initializer->getAsConstantUnion()) { 
            constUnion* unionArray = variable->getConstPointer();            

            if (pType.size == 1 && TType(pType).getBasicType() != EbtStruct) {
                switch (pType.type ) {
                case EbtInt:
                    unionArray->iConst = (initializer->getAsConstantUnion()->getUnionArrayPointer())[0].iConst;
                    break;
                case EbtFloat:
                    unionArray->fConst = (initializer->getAsConstantUnion()->getUnionArrayPointer())[0].fConst;
                    break;
                case EbtBool:
                    unionArray->bConst = (initializer->getAsConstantUnion()->getUnionArrayPointer())[0].bConst;
                    break;
                default:
                    error(line, " cannot initialize constant of this type", "", "");
                    return true;
                }
            } else {
                variable->shareConstPointer(initializer->getAsConstantUnion()->getUnionArrayPointer());
            }
        } else if (initializer->getAsAggregate()) {
            bool returnVal = false;
            constUnion* unionArray = variable->getConstPointer();
            if (initializer->getAsAggregate()->getSequence().size() == 1 && initializer->getAsAggregate()->getSequence()[0]->getAsTyped()->getAsConstantUnion())  {
                returnVal = intermediate.parseConstTree(line, initializer, unionArray, initializer->getAsAggregate()->getOp(), symbolTable,  variable->getType(), true);
            }
            else {
                returnVal = intermediate.parseConstTree(line, initializer, unionArray, initializer->getAsAggregate()->getOp(), symbolTable, variable->getType());
            }
            intermNode = 0;
            constUnion *arrayUnion = unionArray;
            if (returnVal) {
                arrayUnion = 0;
                variable->getType().changeQualifier(EvqTemporary);
            } 
            return returnVal;
        } else if (initializer->getAsSymbolNode()) {
            const TSymbol* symbol = symbolTable.find(initializer->getAsSymbolNode()->getSymbol());
            const TVariable* tVar = static_cast<const TVariable*>(symbol);

            constUnion* constArray = tVar->getConstPointer();
            variable->shareConstPointer(constArray);
        } else {
            error(line, " assigning non-constant to", "=", "'%s'", variable->getType().getCompleteString().c_str());
            variable->getType().changeQualifier(EvqTemporary);
            return true;
        }
    }
 
    if (qualifier != EvqConst) {
        TIntermSymbol* intermSymbol = intermediate.addSymbol(variable->getUniqueId(), variable->getName(), variable->getType(), line);
        intermNode = intermediate.addAssign(EOpAssign, intermSymbol, initializer, line);
        if (intermNode == 0) {
            assignError(line, "=", intermSymbol->getCompleteString(), initializer->getCompleteString());
            return true;
        }
    } else 
        intermNode = 0;

    return false;
}

//
// This method checks to see if the given aggregate node has all its children nodes as constants
// This method does not test if structure members are constant
//
bool TParseContext::canNodeBeRemoved(TIntermNode* childNode)
{
    TIntermAggregate *aggrNode = childNode->getAsAggregate();
    if (!aggrNode)
        return false;

    if (!aggrNode->isConstructor() || aggrNode->getOp() == EOpConstructStruct)
        return false;

    bool allConstant = true;

    // check if all the child nodes are constants so that they can be inserted into 
    // the parent node
    if (aggrNode) {
        TIntermSequence &childSequenceVector = aggrNode->getSequence() ;
        for (TIntermSequence::iterator p = childSequenceVector.begin(); 
                                    p != childSequenceVector.end(); p++) {
            if (!(*p)->getAsTyped()->getAsConstantUnion())
                return false;
        }
    }

    return allConstant;
}

// This function is used to test for the correctness of the parameters passed to various constructor functions
// and also convert them to the right datatype if it is allowed and required. 
//
// Returns 0 for an error or the constructed node (aggregate or typed) for no error.
//
TIntermTyped* TParseContext::addConstructor(TIntermNode* node, TType* type, TOperator op, TFunction* fnCall, TSourceLoc line)
{
    if (node == 0)
        return 0;

    TIntermAggregate* aggrNode = node->getAsAggregate();
    
    TTypeList::iterator list;
    TTypeList* structure = 0;  // Store the information (vector) about the return type of the structure.
    if (op == EOpConstructStruct) {
        const TType& ttype = fnCall->getReturnType();
        structure = ttype.getStruct();
        list = (*structure).begin();
    }

    bool singleArg;
    if (aggrNode) {
        if (aggrNode->getOp() != EOpNull || aggrNode->getSequence().size() == 1)
            singleArg = true;
        else
            singleArg = false;
    } else
        singleArg = true;

    TIntermTyped *newNode;
    if (singleArg) {
        if (op == EOpConstructStruct) { 
            // If structure constructor is being called for only one parameter inside the structure,
            // we need to call constructStruct function once.
            if (structure->size() != 1) {
                error(line, "Number of constructor parameters does not match the number of structure fields", "constructor", "");
                
                return 0;
            } else
                return constructStruct(node, (*list).type, 1, node->getLine(), false);
        } else {
            newNode =  constructBuiltIn(type, op, node, node->getLine(), false);
            if (newNode && newNode->getAsAggregate()) {
                if (canNodeBeRemoved(newNode->getAsAggregate()->getSequence()[0])) {
                    TIntermAggregate* returnAggNode = newNode->getAsAggregate()->getSequence()[0]->getAsAggregate();
                    newNode = intermediate.removeChildNode(newNode, type, returnAggNode);
                }
            }
            return newNode;
        }
    }
    
    //
    // Handle list of arguments.
    //
    TIntermSequence &sequenceVector = aggrNode->getSequence() ;    // Stores the information about the parameter to the constructor
    // if the structure constructor contains more than one parameter, then construct
    // each parameter
    if (op == EOpConstructStruct) {
        if (structure->size() != sequenceVector.size()) { // If the number of parameters to the constructor does not match the expected number of parameters
            error(line, "Number of constructor parameters does not match the number of structure fields", "constructor", "");
            
            return 0;
        }
    }
    
    int paramCount = 0;  // keeps a track of the constructor parameter number being checked    
    
    // for each parameter to the constructor call, check to see if the right type is passed or convert them 
    // to the right type if possible (and allowed).
    // for structure constructors, just check if the right type is passed, no conversion is allowed.
    
    for (TIntermSequence::iterator p = sequenceVector.begin(); 
                                   p != sequenceVector.end(); p++, paramCount++) {
        bool move = false;
        if (op == EOpConstructStruct) {
            newNode = constructStruct(*p, (list[paramCount]).type, paramCount+1, node->getLine(), true);
            if (newNode)
                move = true;
        } else {
            newNode = constructBuiltIn(type, op, *p, node->getLine(), true);

            if (newNode) {
                if (canNodeBeRemoved(newNode))
                    intermediate.removeChildNode(sequenceVector, *type, paramCount, p, newNode->getAsAggregate());
                else
                    move = true;    
            } 
        }
        if (move) {
            sequenceVector.erase(p); 
            sequenceVector.insert(p, newNode);
        }
    }

    return intermediate.setAggregateOperator(aggrNode, op, line);
}

// Function for constructor implementation. Calls addUnaryMath with appropriate EOp value
// for the parameter to the constructor (passed to this function). Essentially, it converts
// the parameter types correctly. If a constructor expects an int (like ivec2) and is passed a 
// float, then float is converted to int.
//
// Returns 0 for an error or the constructed node.
//
TIntermTyped* TParseContext::constructBuiltIn(TType* type, TOperator op, TIntermNode* node, TSourceLoc line, bool subset)
{
    TIntermTyped* newNode;
    TOperator basicOp;

    //
    // First, convert types as needed.
    //
    switch (op) {
    case EOpConstructVec2:
    case EOpConstructVec3:
    case EOpConstructVec4:
    case EOpConstructMat2:
    case EOpConstructMat3:
    case EOpConstructMat4:
    case EOpConstructFloat:
        basicOp = EOpConstructFloat;
        break;

    case EOpConstructIVec2:
    case EOpConstructIVec3:
    case EOpConstructIVec4:
    case EOpConstructInt:
        basicOp = EOpConstructInt;
        break;

    case EOpConstructBVec2:
    case EOpConstructBVec3:
    case EOpConstructBVec4:
    case EOpConstructBool:
        basicOp = EOpConstructBool;
        break;

    default:
        error(line, "unsupported construction", "", "");
        recover();

        return 0;
    }
    newNode = intermediate.addUnaryMath(basicOp, node, node->getLine(), symbolTable);
    if (newNode == 0) {
        error(line, "can't convert", "constructor", "");
        return 0;
    }

    //
    // Now, if there still isn't an operation to do the construction, and we need one, add one.
    //
    
    // Otherwise, skip out early.
    if (subset || newNode != node && newNode->getType() == *type)
        return newNode;

    // setAggregateOperator will insert a new node for the constructor, as needed.
    return intermediate.setAggregateOperator(newNode, op, line);
}

// This function tests for the type of the parameters to the structures constructors. Raises
// an error message if the expected type does not match the parameter passed to the constructor.
//
// Returns 0 for an error or the input node itself if the expected and the given parameter types match.
//
TIntermTyped* TParseContext::constructStruct(TIntermNode* node, TType* type, int paramCount, TSourceLoc line, bool subset)
{
    if (*type == node->getAsTyped()->getType()) {
        if (subset)
            return node->getAsTyped();
        else
            return intermediate.setAggregateOperator(node->getAsTyped(), EOpConstructStruct, line);
    } else {
        error(line, "", "constructor", "cannot convert parameter %d from '%s' to '%s'", paramCount,
                node->getAsTyped()->getType().getBasicString(), type->getBasicString());
        recover();
    }

    return 0;
}

//
// This function returns the tree representation for the vector field(s) being accessed from contant vector.
// If only one component of vector is accessed (v.x or v[0] where v is a contant vector), then a contant node is
// returned, else an aggregate node is returned (for v.xy). The input to this function could either be the symbol
// node or it could be the intermediate tree representation of accessing fields in a constant structure or column of 
// a constant matrix.
//
TIntermTyped* TParseContext::addConstVectorNode(TVectorFields& fields, TIntermTyped* node, TSourceLoc line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();
    TIntermAggregate* aggregateNode = node->getAsAggregate();

    constUnion *unionArray;
    if (tempConstantNode) {
        unionArray = tempConstantNode->getUnionArrayPointer();

        if (!unionArray) {  // this error message should never be raised
            infoSink.info.message(EPrefixInternalError, "constUnion not initialized in addConstVectorNode function", line);
            recover();

            return node;
        }
    } else if (aggregateNode) { // if an aggregate node is present, the value has to be taken from the parse tree 
        // for a case like vec(4).xz
        unionArray = new constUnion[aggregateNode->getType().getInstanceSize()];                        
    
        bool returnVal = false;
        if (aggregateNode->getAsAggregate()->getSequence().size() == 1 && aggregateNode->getAsAggregate()->getSequence()[0]->getAsTyped()->getAsConstantUnion())  {
            returnVal = intermediate.parseConstTree(line, aggregateNode, unionArray, aggregateNode->getOp(), symbolTable,  aggregateNode->getType(), true);
        }
        else {
            returnVal = intermediate.parseConstTree(line, aggregateNode, unionArray, aggregateNode->getOp(), symbolTable, aggregateNode->getType());
        }

        if (returnVal)
            return 0;

    } else { // The node has to be either a symbol node or an aggregate node or a tempConstant node, else, its an error
        error(line, "No aggregate or constant union node available", "Internal Error", "");
        recover();

        return 0;
    }

    constUnion* constArray = new constUnion[fields.num];

    for (int i = 0; i < fields.num; i++) {
        if (fields.offsets[i] >= node->getType().getInstanceSize()) {
            error(line, "", "[", "vector field selection out of range '%d'", fields.offsets[i]);
            recover();
            fields.offsets[i] = 0;
        }
        
        constArray[i] = unionArray[fields.offsets[i]];

    } 
    typedNode = intermediate.addConstantUnion(constArray, node->getType(), line);
    return typedNode;
}

//
// This function returns the column being accessed from a constant matrix. The values are retrieved from
// the symbol table and parse-tree is built for a vector (each column of a matrix is a vector). The input 
// to the function could either be a symbol node (m[0] where m is a constant matrix)that represents a 
// constant matrix or it could be the tree representation of the constant matrix (s.m1[0] where s is a constant structure)
//
TIntermTyped* TParseContext::addConstMatrixNode(int index, TIntermTyped* node, TSourceLoc line)
{
    TIntermTyped* typedNode;
    TIntermConstantUnion* tempConstantNode = node->getAsConstantUnion();
    TIntermAggregate* aggregateNode = node->getAsAggregate();

    if (index >= node->getType().getNominalSize()) {
        error(line, "", "[", "matrix field selection out of range '%d'", index);
        recover();
        index = 0;
    }

    if (tempConstantNode) {
         constUnion* unionArray = tempConstantNode->getUnionArrayPointer();
         int size = tempConstantNode->getType().getNominalSize();
         typedNode = intermediate.addConstantUnion(&unionArray[size*index], tempConstantNode->getType(), line);
    } else if (aggregateNode) {
        // for a case like mat4(5)[0]
        constUnion* unionArray = new constUnion[aggregateNode->getType().getInstanceSize()];                        
        int size = aggregateNode->getType().getNominalSize();
    
        bool returnVal = false;
        if (aggregateNode->getAsAggregate()->getSequence().size() == 1 && aggregateNode->getAsAggregate()->getSequence()[0]->getAsTyped()->getAsConstantUnion())  {
            returnVal = intermediate.parseConstTree(line, aggregateNode, unionArray, aggregateNode->getOp(), symbolTable,  aggregateNode->getType(), true);
        }
        else {
            returnVal = intermediate.parseConstTree(line, aggregateNode, unionArray, aggregateNode->getOp(), symbolTable, aggregateNode->getType());
        }

        if (!returnVal)
            typedNode = intermediate.addConstantUnion(&unionArray[size*index], aggregateNode->getType(), line);
        else 
            return 0;

    } else {
        error(line, "No Aggregate or Constant Union node available", "Internal Error", "");
        recover();

        return 0;
    }

    return typedNode;
}

//
// This function returns the value of a particular field inside a constant structure from the symbol table. 
// If there is an embedded/nested struct, it appropriately calls addConstStructNested or addConstStructFromAggr
// function and returns the parse-tree with the values of the embedded/nested struct.
//
TIntermTyped* TParseContext::addConstStruct(TString& identifier, TIntermTyped* node, TSourceLoc line)
{
    TTypeList* fields = node->getType().getStruct();
    TIntermTyped *typedNode;
    int instanceSize = 0;
    unsigned int index = 0;
    TIntermConstantUnion *tempConstantNode = node->getAsConstantUnion();
    TIntermAggregate* aggregateNode = node->getAsAggregate();

    for ( index = 0; index < fields->size(); ++index) {
        if ((*fields)[index].type->getFieldName() == identifier) {
            break;
        } else {
            if ((*fields)[index].type->getStruct())
                //?? We should actually be calling getStructSize() function and not setStructSize. This problem occurs in case
                // of nested/embedded structs.                
                instanceSize += (*fields)[index].type->setStructSize((*fields)[index].type->getStruct());
            else
                instanceSize += (*fields)[index].type->getInstanceSize();
        }
    }

    if (tempConstantNode) {
         constUnion* constArray = tempConstantNode->getUnionArrayPointer();

         typedNode = intermediate.addConstantUnion(constArray+instanceSize, tempConstantNode->getType(), line); // type will be changed in the calling function
    } else if (aggregateNode) {
        // for a case like constStruct(1,v3).i where structure fields is int i and vec3 v3.

        constUnion* unionArray = new constUnion[aggregateNode->getType().getStructSize()];                        
    
        bool returnVal = false;
        if (aggregateNode->getAsAggregate()->getSequence().size() == 1 && aggregateNode->getAsAggregate()->getSequence()[0]->getAsTyped()->getAsConstantUnion())  {
            returnVal = intermediate.parseConstTree(line, aggregateNode, unionArray, aggregateNode->getOp(), symbolTable,  aggregateNode->getType(), true);
        }
        else {
            returnVal = intermediate.parseConstTree(line, aggregateNode, unionArray, aggregateNode->getOp(), symbolTable, aggregateNode->getType());
        }

        if (!returnVal)
            typedNode = intermediate.addConstantUnion(unionArray+instanceSize, aggregateNode->getType(), line);
        else
            return 0;

    } else {
        error(line, "No Aggregate or Constant Union node available", "Internal Error", "");
        recover();

        return 0;
    }

    return typedNode;
}

//
// Initialize all supported extensions to disable
//
void TParseContext::initializeExtensionBehavior()
{
    //
    // example code: extensionBehavior["test"] = EDisable; // where "test" is the name of 
    // supported extension
    //
}

OS_TLSIndex GlobalParseContextIndex = OS_INVALID_TLS_INDEX;

bool InitializeParseContextIndex()
{
    if (GlobalParseContextIndex != OS_INVALID_TLS_INDEX) {
        assert(0 && "InitializeParseContextIndex(): Parse Context already initalised");
        return false;
    }

    //
    // Allocate a TLS index.
    //
    GlobalParseContextIndex = OS_AllocTLSIndex();
    
    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "InitializeParseContextIndex(): Parse Context already initalised");
        return false;
    }

    return true;
}

bool InitializeGlobalParseContext()
{
    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "InitializeGlobalParseContext(): Parse Context index not initalised");
        return false;
    }

    TThreadParseContext *lpParseContext = static_cast<TThreadParseContext *>(OS_GetTLSValue(GlobalParseContextIndex));
    if (lpParseContext != 0) {
        assert(0 && "InitializeParseContextIndex(): Parse Context already initalised");
        return false;
    }

    TThreadParseContext *lpThreadData = new TThreadParseContext();
    if (lpThreadData == 0) {
        assert(0 && "InitializeGlobalParseContext(): Unable to create thread parse context");
        return false;
    }

    lpThreadData->lpGlobalParseContext = 0;
    OS_SetTLSValue(GlobalParseContextIndex, lpThreadData);

    return true;
}

TParseContextPointer& GetGlobalParseContext()
{
    //
    // Minimal error checking for speed
    //

    TThreadParseContext *lpParseContext = static_cast<TThreadParseContext *>(OS_GetTLSValue(GlobalParseContextIndex));

    return lpParseContext->lpGlobalParseContext;
}

bool FreeParseContext()
{
    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "FreeParseContext(): Parse Context index not initalised");
        return false;
    }

    TThreadParseContext *lpParseContext = static_cast<TThreadParseContext *>(OS_GetTLSValue(GlobalParseContextIndex));
    if (lpParseContext)
        delete lpParseContext;

    return true;
}

bool FreeParseContextIndex()
{
    OS_TLSIndex tlsiIndex = GlobalParseContextIndex;

    if (GlobalParseContextIndex == OS_INVALID_TLS_INDEX) {
        assert(0 && "FreeParseContextIndex(): Parse Context index not initalised");
        return false;
    }

    GlobalParseContextIndex = OS_INVALID_TLS_INDEX;

    return OS_FreeTLSIndex(tlsiIndex);
}
