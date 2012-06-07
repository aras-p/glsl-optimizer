/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

package com.adobe.AGALOptimiser.agal {

import com.adobe.AGALOptimiser.Semantics;
import com.adobe.AGALOptimiser.error.Enforcer;
import com.adobe.AGALOptimiser.error.ErrorMessages;
import com.adobe.AGALOptimiser.nsinternal;
import com.adobe.AGALOptimiser.type.ArrayType;
import com.adobe.AGALOptimiser.type.ImageType;
import com.adobe.AGALOptimiser.type.MatrixType;
import com.adobe.AGALOptimiser.type.ScalarType;
import com.adobe.AGALOptimiser.type.Type;
import com.adobe.AGALOptimiser.type.VectorType;
import com.adobe.AGALOptimiser.utils.Tokenizer;

use namespace nsinternal;

public final class AgalParser
{
    // program scope state
    private var tokens_       : Vector.<String> = null;
    private var currentToken_ : int             = 0;
    private var theProgram_   : Program         = null;
    private var version_      : int             = 0;

    // procedure scope state
    private var theProcedure_ : Procedure       = null;
    
    public function AgalParser()
    {
    }

    private function cleanProcedureState() : void
    {
        theProcedure_ = null;
    }

    private function cleanState() : void
    {
        cleanProcedureState();

        tokens_       = null;
        currentToken_ = 0;
        theProgram_   = null;
        version_      = 0;
    }

    private function expect(expected : String,
                            seen : String = null) : void
    {
        if (seen == null)
            seen = getNextTokenAndAdvance();

        Enforcer.check(seen == expected, ErrorMessages.EXPECTED_TOKEN_NOT_SEEN + " was: " + seen);
    }

    private function getNextToken() : String
    {
        Enforcer.check((tokens_ != null) && (currentToken_ < tokens_.length),
                       "Token stream corrupt or exhausted during parse");

        return tokens_[currentToken_];
    }

    private function getNextTokenAndAdvance() : String
    {
        Enforcer.check((tokens_ != null) && (currentToken_ < tokens_.length),
                       ErrorMessages.TOKEN_STREAM_CORRUPT);

        return tokens_[currentToken_++];
    }

    // REVIEW: right now we're writing this assuming non-minimal form serialized Agal programs
    public function parse(stream : String) : Program
    {
        Enforcer.checkNull(stream);

        var theProgram : Program = null;

        try
        {
            var tokenizer : Tokenizer = new Tokenizer(/\n|-|\.|,|\(|\)|\[|\]|<|>/);

            tokens_ = tokenizer.tokenize(stream);
            currentToken_ = 0;

            parseProgram();
        }
        catch (e : Error)
        {
            theProgram_ = null;
            throw e;
        }
        finally
        {
            theProgram  = theProgram_;

            cleanState();
        }
        
        return theProgram;
    }
    
    private function parseArray() : Type
    {
        var dimension : int = int(getNextTokenAndAdvance());
        // ignore the x
        getNextTokenAndAdvance();
        var arrayType : Type = parseType();
        //ignore the ]
        getNextTokenAndAdvance();
        var array : ArrayType = new ArrayType(arrayType, dimension);
        return array;
    }

    private function parseBool() : Boolean
    {
        var token : String = getNextTokenAndAdvance();

        if (token == Constants.TRUE_TAG)
            return true;
        else if (token == Constants.FALSE_TAG)
            return false;
        else
            throw new Error(ErrorMessages.INVALID_VALUE);

        return false;
    }
    
    private function parseBools(channels : int) : Vector.<Boolean>
    {
        var values : Vector.<Boolean> = new Vector.<Boolean>(channels, true);

        for (var i : int = 0 ; i < channels ; ++i)
        {
            if (i > 0)
                expect(",");

            values[i] = parseBool();
        }

        return values;
    }

    private function parseBoolTypes() : Type
    {
        // we've already seen "bool"

        var token : String = getNextTokenAndAdvance();

        if (token == "-")
        {
            // we have a bool vector

            return new VectorType(new ScalarType(ScalarType.BOOL_TYPE),
                                  int(getNextTokenAndAdvance()));
        }
        else
        {
            rewindToken();
            return new ScalarType(ScalarType.BOOL_TYPE);
        }
    }

    private function parseDestinationRegister() : DestinationRegister
    {
        var regName  : String    = getNextTokenAndAdvance();
        var mask     : WriteMask;

        if (getNextTokenAndAdvance() == ".")
            mask = parseWriteMask();
        else
        {
            rewindToken();
            mask = new WriteMask();
        }

        var regRef   : Array     = parseRegisterReference(regName, mask, null);
        var register : Register  = regRef[0];
        var slotUsed : int       = regRef[1];

        return new DestinationRegister(register,
                                       mask,
                                       slotUsed);
    }

    private function parseFloat() : Number
    {
        var numString : String;
        var token     : String = getNextTokenAndAdvance();
        var sign      : Number = 1.0;

        if (token == "-")
        {
            sign = -1.0;
            token = getNextTokenAndAdvance();
        }

        numString = token;

        token = getNextTokenAndAdvance();

        if (token == ".")
        {
            numString += ".";
            numString += getNextTokenAndAdvance();
        }
        else
            rewindToken();

        return (sign * Number(numString));
    }

    private function parseFloats(channels : int) : Vector.<Number>
    {
        var values : Vector.<Number> = new Vector.<Number>(channels, true);

        for (var i : int = 0 ; i < channels ; ++i)
        {
            if (i > 0)
                expect(",");

            values[i] = parseFloat();
        }

        return values;
    }

    private function parseFloatTypes() : Type
    {
        var token : String = getNextTokenAndAdvance();

        if (token == "-")
        {
            // it's a vector or a matrix

            var dim1 : int = int(getNextTokenAndAdvance());

            token = getNextTokenAndAdvance();

            if (token == "-")
            {
                // it's a matrix
                var dim2 : int = int(getNextTokenAndAdvance());

                return new MatrixType(dim1, dim2);
            }
            else
            {
                rewindToken();
                return new VectorType(new ScalarType(ScalarType.FLOAT_TYPE),
                                      dim1);
            }
        }
        else
        {
            // it's a scalar
            rewindToken();
            return new ScalarType(ScalarType.FLOAT_TYPE);
        }
    }

    private function parseInstruction(opcode : Opcode) : Instruction
    {
        var dst            : DestinationRegister = null;
        var src0           : SourceRegister      = null;
        var src1           : SourceRegister      = null;

        var hasDestination : Boolean = Instruction.isDestinationRequired(opcode);
        var numSources     : int     = Instruction.expectedSourceCount(opcode);

        if (hasDestination)
            dst = parseDestinationRegister();

        if (numSources > 0)
        {
            if (hasDestination)
                expect(",");

            src0 = parseSourceRegister();
        }

        if (numSources == 2)
        {
            expect(",");
            src1 = parseSourceRegister();
        }

        return new Instruction(opcode, dst, src0, src1);
    }

    private function parseImageTypes() : Type
    {
        // we've already seen "image"

        expect("-");
        
        return new ImageType(int(getNextTokenAndAdvance()));
    }

    private function parseInt() : int
    {
        var token : String = getNextTokenAndAdvance();

        if (token == "-")
            return -int(getNextTokenAndAdvance());
        else
            return int(token);
    }

    private function parseInts(channels : int) : Vector.<int>
    {
        var values : Vector.<int> = new Vector.<int>(channels, true);

        for (var i : int = 0 ; i < channels ; ++i)
        {
            if (i > 0)
                expect(",");

            values[i] = parseInt();
        }

        return values;
    }

    private function parseIntTypes() : Type
    {
        // we've already seen "int"

        var token : String = getNextTokenAndAdvance();

        if (token == "-")
        {
            // we have an int vector

            return new VectorType(new ScalarType(ScalarType.INT_TYPE),
                                  int(getNextTokenAndAdvance()));
        }
        else
        {
            rewindToken();
            return new ScalarType(ScalarType.INT_TYPE);
        }
    }

    private function parseNumericalConstant(constantType : Type) : ConstantValue
    {
        var id           : int           = theProgram_.constants.length;
        var cv           : ConstantValue = null;

        expect("(");

        if (constantType.isScalarBool() || constantType.isVectorBool())
            theProgram_.constants.push(cv = new ConstantBooleanValue(id, constantType, parseBools(constantType.channelCount())));
        else if (constantType.isScalarFloat() || constantType.isVectorFloat())
            theProgram_.constants.push(cv = new ConstantFloatValue(id, constantType, parseFloats(constantType.channelCount())));
        else if (constantType.isScalarInt() || constantType.isVectorInt())
            theProgram_.constants.push(cv = new ConstantIntValue(id, constantType, parseInts(constantType.channelCount())));
        else if (constantType.isMatrix())
            theProgram_.constants.push(cv = new ConstantMatrixValue(id, constantType as MatrixType, parseFloats(constantType.channelCount())));
        else
            throw new Error(ErrorMessages.INVALID_VALUE);

        expect(")");

        return cv;
    }

    private function parseOperations() : void
    {
        expect(Constants.OPERATIONS_TAG);
        
        for (var token : String = getNextTokenAndAdvance() ; token != Constants.END_OPERATIONS_TAG ; token = getNextTokenAndAdvance())
        {
            var opcode         : Opcode  = Opcode.mnemonicToOpcode(token);
            
            Enforcer.check(!opcode.isSubroutineRelated(), ErrorMessages.AGAL_CONTROL_FLOW_RESTRICTION_V1);

            if (opcode.code == Opcode.SAMPLE)
                theProcedure_.operations.push(parseSample(opcode));
            else
                theProcedure_.operations.push(parseInstruction(opcode));
        }        
    }

    private function parseProcedure() : void
    {
        try
        {
            theProcedure_ = new Procedure(getNextTokenAndAdvance());

            parseOperations();

            expect(Constants.END_PROCEDURE_TAG);
        }
        catch (e : Error)
        {
            theProcedure_ = null;
            throw e;
        }
        finally
        {
            if (theProcedure_ != null)
            {
                theProgram_.procedures.push(theProcedure_);

                if ((theProgram_.category.code == ProgramCategory.FRAGMENT) && (theProcedure_.name == Constants.EVALUATE_FRAGMENT_TAG))
                    theProgram_.main = theProcedure_;
                else if ((theProgram_.category.code == ProgramCategory.VERTEX) && (theProcedure_.name == Constants.EVALUATE_VERTEX_TAG))
                    theProgram_.main = theProcedure_;
            }

            cleanProcedureState();
        }
    }

    private function parseProgram() : void
    {
        var token            : String          = getNextTokenAndAdvance();
        var programCategory  : ProgramCategory = null;
        var version          : int             = 0;
        var endProgramString : String          = Constants.END_TAG_PREFIX + token;

        switch (token)
        {
        case Constants.FRAGMENT_PROGRAM_TAG:

            programCategory = new ProgramCategory(ProgramCategory.FRAGMENT);
            break;

        case Constants.VERTEX_PROGRAM_TAG:

            programCategory = new ProgramCategory(ProgramCategory.VERTEX);
            break;

        default:

            throw new Error(ErrorMessages.ENUM_VALUE_INVALID);
            break;
        }

        expect(Constants.VERSION_TAG);
        expect("(");

        version = parseInt();

        expect(")");

        theProgram_ = new Program(programCategory, version);

        parseRegisters();
        
        // trace(theProgram_.serialize());

        for (token = getNextTokenAndAdvance() ; token != endProgramString ; token = getNextTokenAndAdvance())
        {
            if (token == Constants.PROCEDURE_TAG)
                parseProcedure();
            else
                throw new Error(ErrorMessages.EXPECTED_TOKEN_NOT_SEEN);
        }
    }

    private function parseRegister() : void
    {
        var type             : Type             = undefined;
        var name             : String           = undefined;
        var offsetInPhysical : int              = 0;
        var token            : String           = undefined;
        var slotCount        : int              = 1;
        var category         : RegisterCategory = undefined;
        var baseNumber       : int              = undefined;
        var newRegister      : Register         = undefined;

        type = parseType();

        name = getNextTokenAndAdvance();

        Enforcer.check(type.isScalarFloat() || type.isVectorFloat() || type.isMatrix() || type.isImage() || type.isArray(), "Unexpected register type");

        if (type.isScalarFloat() || (type.isVectorFloat() && (type.channelCount() < 4)))
        {
            var swizzleString : String = undefined;

            expect(".");
            
            swizzleString = getNextTokenAndAdvance();

            Enforcer.check(type.channelCount() == swizzleString.length, ErrorMessages.SWIZZLE_STRING_TYPE_MISMATCH);

            if (swizzleString.charAt(0) == "w")
                offsetInPhysical = 3;
            else
                offsetInPhysical = swizzleString.charCodeAt(0) - "x".charCodeAt(0); 

            // you can't offset more than 3 without leaving the register.
            Enforcer.check((offsetInPhysical >= 0) && (offsetInPhysical <= 3), "Unexpected channel code in swizzleString");
        }

        //2x2 matrices
        if (type.channelCount() == 4 && type.isMatrix())
            slotCount = 2;
        else if (type.channelCount() == 9)
            slotCount = 3;
        else if (type.channelCount() == 16)
            slotCount = 4;
        else if (type.isArray())
        {
            if ((type as ArrayType).elementType is MatrixType)
            {
                var matrix : MatrixType = (type as ArrayType).elementType as MatrixType;
                slotCount = (type as ArrayType).dimension * matrix.colDimension;
            }
            else
                slotCount = (type as ArrayType).dimension;
        }
        
        var programCategory : ProgramCategory = theProgram_.category;
        
        if(name == "op" || name == "oc") {
            category = new RegisterCategory(RegisterCategory.OUTPUT, programCategory);
        } else {
            baseNumber = int(name.slice(2));
            var s:String = name.slice(1, 2);
            
            if (s == "c")
                category = new RegisterCategory(RegisterCategory.CONSTANT, programCategory);
            else if (s == "t")
                category = new RegisterCategory(RegisterCategory.TEMPORARY, programCategory);
            else if (s == "o")
                category = new RegisterCategory(RegisterCategory.OUTPUT, programCategory);
            else if (s == "s")
                category = new RegisterCategory(RegisterCategory.SAMPLER, programCategory);
            else if (s == "a")
                category = new RegisterCategory(RegisterCategory.VERTEXATTRIBUTE, programCategory);
            else if (name.indexOf("v") == 0) {
                category   = new RegisterCategory(RegisterCategory.VARYING, programCategory);
                baseNumber = int(name.slice(1));
            } else {
                throw "FAIL! " + name
            }
        }

        // we're ready to make the basic register

        newRegister = new Register(category, baseNumber, type, slotCount, null, offsetInPhysical);

        
        token = getNextTokenAndAdvance();

        if (token == Constants.CONST_TAG)
            newRegister.valueInfo = parseNumericalConstant(type);
        else if (token == Constants.PARAMETER_TAG)
        {
            var paramValue : SimpleValue = parseSimpleValue(theProgram_.parameters.length, category, type);
            newRegister.valueInfo = paramValue;
            theProgram_.parameters.push(paramValue);
        }
        else if (token == Constants.TEXTURE_TAG)
            newRegister.valueInfo = parseTextureValue(type);
        else if (token == Constants.VERTEX_TAG)
            newRegister.valueInfo = parseVertexValue(type);
        else if (token == Constants.OUTPUT_TAG)
        {
            var outValue : SimpleValue = parseSimpleValue(theProgram_.outputs.length, category, type); 
            newRegister.valueInfo = outValue
            theProgram_.outputs.push(outValue);
        }
        else if (token == Constants.INTERPOLATED_TAG)
        {
            var varyValue : SimpleValue = parseSimpleValue(theProgram_.interpolateds.length, category, type);
            newRegister.valueInfo = varyValue;
            theProgram_.interpolateds.push(varyValue);
        }
        else
            rewindToken();

        // all done, now put it in the program

        theProgram_.registers.push(newRegister);
    }

    private function parseRegisterReference(ref : String,
                                            mask : WriteMask, 
                                            swizzle : Swizzle,
                                            indirectBaseNumber : int = 0) : Array
    {
        var catCode    : int;
        var baseNumber : int;
        var rname      : String = ref;
 
            if (ref == "vc" || ref =="fc")
                // get the source base number from its offset
                baseNumber = indirectBaseNumber;
            else    
                baseNumber = int(ref.slice(2));

            var newref:String = ref.slice(1, 2);

            if (ref == "op" || ref == "oc") 
                catCode = RegisterCategory.OUTPUT;
            else if (newref == "c") 
                catCode = RegisterCategory.CONSTANT;
            else if (newref == "t") 
                catCode = RegisterCategory.TEMPORARY;
            else if (newref == "s") 
                catCode = RegisterCategory.SAMPLER;
            else if (newref == "a") 
                catCode = RegisterCategory.VERTEXATTRIBUTE;
            else if (ref.indexOf("v") == 0) {
                catCode = RegisterCategory.VARYING;
                baseNumber = int(ref.slice(1));
            } else {
                throw new Error(ErrorMessages.INVALID_VALUE + "ref: " + ref);
            }

        for each (var r : Register in theProgram_.registers)
        {
            var isThisTheOne : Boolean;
            
            isThisTheOne = ((r.category.code == catCode) && 
                            (r.baseNumber <= baseNumber) && 
                            ((r.baseNumber + r.slotCount) > baseNumber) &&
                            ((mask == null) || r.isMaskCompatible(mask)) &&
                            ((swizzle == null) || r.isSwizzleCompatible(swizzle))) 
            if (isThisTheOne)
                return new Array(r, baseNumber - r.baseNumber);
        }

        throw new Error(ErrorMessages.REGISTER_MISSING);
        return null;
    }

    private function parseRegisters() : void
    {
        expect(Constants.REGISTERS_TAG);

        for (var token : String = getNextTokenAndAdvance() ; token != Constants.END_REGISTERS_TAG ; token = getNextTokenAndAdvance())
        {
            rewindToken();
            parseRegister();
        }
    }

    private function parseSample(opcode : Opcode) : SampleOperation
    {
        var dst  : DestinationRegister  = null;
        var src0 : SourceRegister       = null;
        var src1 : SampleSourceRegister = null;

        dst = parseDestinationRegister();
        
        expect(",");

        src0 = parseSourceRegister();

        expect(",");

        src1 = parseSampleSourceRegister();

        return new SampleOperation(dst, src0, src1);
    }

    private function parseSampleOptions() : SampleOptions
    {
        var sampleOptions : uint = 0;

        expect("<");

        while (SampleOptions.isSampleOption(getNextToken()) || getNextToken() == ",")
        {
            var option : String = getNextTokenAndAdvance();
            
            if(option == ",")
                continue;

            sampleOptions |= SampleOptions.sampleOptionToValue( option );
        }

        expect(">");

        return new SampleOptions( sampleOptions );
    }
    
    private function parseSampleSourceRegister() : SampleSourceRegister
    {
        var regRef : Array  = parseRegisterReference(getNextTokenAndAdvance(), null, new Swizzle());

        return new SampleSourceRegister(regRef[0], parseSampleOptions());
    }

    private function parseSemantics() : Semantics
    {
        var token : String = undefined;
        var id    : String = undefined;
        var index : int    = -1;

        expect(com.adobe.AGALOptimiser.Constants.SEMANTICS_TAG);
        expect("(");
        
        id = getNextTokenAndAdvance();

        token = getNextTokenAndAdvance();

        if (token == "[")
        {
            index = parseInt();
            expect("]");
            token = getNextTokenAndAdvance();
        }

        expect(")", token);

        return new Semantics(id, index);
    }

    private function parseSimpleValue(id : int,
                                      c : RegisterCategory,
                                      type : Type) : SimpleValue
    {
        var semantics : Semantics = null;
        var token     : String    = undefined;
        var name      : String    = undefined;

        expect("(");

        name = getNextTokenAndAdvance();

        token = getNextTokenAndAdvance();
        
        rewindToken();

        if (token == com.adobe.AGALOptimiser.Constants.SEMANTICS_TAG)
            semantics = parseSemantics();

        expect (")");
        return new SimpleValue(id, name, type, semantics, c);
    }

    private function parseSourceRegister() : SourceRegister
    {
        var regName            : String    = getNextTokenAndAdvance();
        var indirectRegName    : String    = null;
        var swizzle            : Swizzle   = null;
        var indirectSwizzle    : Swizzle   = null;
        var offset             : int       = 0;
        var indirectBaseNumber : int       = 0;
        var isDirect           : Boolean   = true;
        var indirectRegister   : Register  = null;
        var slotUsed           : int       = 0;
        
        var token : String = getNextTokenAndAdvance();
        if (token == "[")
        {
            indirectRegName = getNextTokenAndAdvance();
            if (getNextTokenAndAdvance() == ".")
                indirectSwizzle = parseSwizzle();
            else
            {
                rewindToken();
                indirectSwizzle = new Swizzle();
            }
            if (getNextTokenAndAdvance() == "+")
            {
                offset = int(getNextTokenAndAdvance());
                indirectBaseNumber = offset;
                //account for the "]"
                getNextTokenAndAdvance();
            }
            else
            {
                //account for the "]"
                rewindToken();
                getNextTokenAndAdvance();
            }
            if (getNextTokenAndAdvance() == "[")
            {
                slotUsed = int(getNextTokenAndAdvance());
                offset += slotUsed;
                //account for the "]"
                getNextTokenAndAdvance();
            }
            //there is no slotUsed Info
            else
                rewindToken();
            isDirect = false;
            
            var indirectRegRef : Array = parseRegisterReference(indirectRegName,
                                                                null,
                                                                indirectSwizzle);
            indirectRegister = indirectRegRef[0];
            
            if (getNextTokenAndAdvance() == ".")
                swizzle = parseSwizzle();
            else
            {
                rewindToken();
                swizzle = new Swizzle();
            }
        }
        
        else
        {
            rewindToken();
            if (getNextTokenAndAdvance() == ".")
                swizzle = parseSwizzle();
            else
            {
                rewindToken();
                swizzle = new Swizzle();
            }
        }
        var regRef   : Array     = parseRegisterReference(regName,
                                                          null,
                                                          swizzle,
                                                          indirectBaseNumber);
        var register : Register  = regRef[0];
        if(!indirectRegister)
            slotUsed      = regRef[1];
        
        return new SourceRegister(register, swizzle, isDirect, slotUsed, indirectRegister, indirectSwizzle, offset);
    }

    private function parseSwizzle() : Swizzle
    {
        var swizzleString : String        = getNextTokenAndAdvance();
        var swizzle       : Vector.<int> = new Vector.<int>(4, true);
        var i             : int;

        Enforcer.check((swizzleString.length > 0) && (swizzleString.length <= 4), "Swizzle string is 1 to 4 characters in length");
        
        for (i = 0 ; i < swizzleString.length ; ++i)
            swizzle[i] = stringToChannelIndex(swizzleString.charAt(i));

        for (; i < 4 ; ++i)
            swizzle[i] = swizzle[i - 1];

        return new Swizzle(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
    }

    private function parseTextureValue(type : Type) : TextureValue
    {
        var id        : int       = theProgram_.textures.length;
        var name      : String    = undefined;
        var sampler   : int       = undefined;
        var semantics : Semantics = null;
        var token     : String    = undefined;

        expect("(");

        name = getNextTokenAndAdvance();

        sampler = parseInt();

        token = getNextTokenAndAdvance();
        rewindToken();

        if (token == com.adobe.AGALOptimiser.Constants.SEMANTICS_TAG)
        {
            rewindToken();
            semantics = parseSemantics();
        }

        expect(")");

        return new TextureValue(id, sampler, name, type, semantics);
    }

    private function parseType() : Type
    {
        var token : String = getNextTokenAndAdvance();

        if (token == "[")
            return parseArray();
        else if (token == com.adobe.AGALOptimiser.type.Constants.BOOL_TYPE_STRING)
            return parseBoolTypes();
        else if (token == com.adobe.AGALOptimiser.type.Constants.FLOAT_TYPE_STRING)
            return parseFloatTypes();
        else if (token == com.adobe.AGALOptimiser.type.Constants.INT_TYPE_STRING)
            return parseIntTypes();
        else if (token == com.adobe.AGALOptimiser.type.Constants.IMAGE_TYPE_STRING)
            return parseImageTypes();
        else
            throw new Error(ErrorMessages.INVALID_VALUE);
    }
 
    private function parseVertexValue(type : Type) : VertexValue
    {
        var semantics : Semantics = null;
        var id        : int       = theProgram_.vertexBuffers.length;
        var mapIndex  : int       = undefined;
        var name      : String    = undefined;
        var token     : String    = undefined;

        expect("(");

        name = getNextTokenAndAdvance();

        mapIndex = parseInt();

        token = getNextTokenAndAdvance();
        rewindToken();

        if (token == com.adobe.AGALOptimiser.Constants.SEMANTICS_TAG)
            semantics = parseSemantics();
        
        expect(")");

        return new VertexValue(id, mapIndex, name, type, semantics);
    }

    private function parseWriteMask() : WriteMask
    {
        var maskValues   : Vector.<Boolean> = new Vector.<Boolean>(4, true);
        var token        : String           = getNextTokenAndAdvance();
        var lastIndex    : int              = -1;

        maskValues[0] = maskValues[1] = maskValues[2] = maskValues[3] = false;
        
        Enforcer.check((token.length > 0) && (token.length <= 4), ErrorMessages.WRITE_MASK_FORMAT_ERROR);

        for (var i : int = 0 ; i < token.length ; ++i)
        {
            var index : int = stringToChannelIndex(token.charAt(i));
            
            Enforcer.check(index > lastIndex, ErrorMessages.WRITE_MASK_FORMAT_ERROR);
            
            lastIndex = index;

            maskValues[index] = true;
        }
        
        return new WriteMask(maskValues[0], maskValues[1], maskValues[2], maskValues[3]);
    }

    private function rewindToken() : void
    {
        Enforcer.check(tokens_ && (currentToken_ > 0), ErrorMessages.TOKEN_STREAM_CORRUPT);

        --currentToken_;
    }

    private function stringToChannelIndex(c : String) : int
    {
        if (c == "x")
            return 0;
        else if (c == "y")
            return 1;
        else if (c == "z")
            return 2;
        else if (c == "w")
            return 3;

        throw new Error(ErrorMessages.INVALID_VALUE);
        return -1;
    }
    
    
    
} // AgalParser
} // com.adobe.AGALOptimiser.agal
