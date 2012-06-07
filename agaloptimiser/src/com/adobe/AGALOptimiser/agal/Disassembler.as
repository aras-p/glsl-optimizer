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

import com.adobe.AGALOptimiser.nsinternal;

import flash.utils.ByteArray;

use namespace nsinternal;

// Based on Sebastian's disassembler in agal.cpp
public class Disassembler
{
    internal var regtypes:String = new String( "actois__________ACTOISRP________" ); 
    internal var compstr:String  = new String( "xyzw" );

    // register types
    internal static const reg_attrib:int = 0x0;
    internal static const reg_const:int = 0x1;
    internal static const reg_temp:int = 0x2;
    internal static const reg_output:int = 0x3;
    internal static const reg_varying:int = 0x4;      
    internal static const reg_sampler:int = 0x5; // special for sampler, can not be used in generic source

    public function Disassembler()
    {
    }

    public function disassemble( binary : ByteArray ) : String
    {
        var result : String = new String();

        binary.position = 0;

        result += disassembleHeader( binary );
        
        while( binary.position != binary.length )
        {
            result += disassembleInstruction( binary );
        }

        return result;
    }

    private function disassembleHeader( binary : ByteArray ) : String
    {
        var result : String = new String();

        var shadertype_vertex : int = 0x0; 
        var shadertype_fragment : int = 0x1; 
        
        var versionTag : int = binary.readByte();

        var version : int = binary.readInt();
        var shaderTypeTag : int = binary.readByte();
        var shaderType : int = binary.readByte();
        
        if( shaderType == shadertype_fragment )
        {
            result += "fragment";
            regtag_ = "f";
        }
        else
        {
            result += "vertex";
            regtag_ = "v";
        }

        result += " program (agal " + version + ")\n";

        return result;
    }

    private function disassembleInstruction( binary : ByteArray ) : String
    {
        var result : String = new String();

        var opcodes : Vector.<OpcodeData> = getOpcodes();

        // TBD: Add indent
        var op : int = binary.readInt();

        var opdata : OpcodeData = opcodes[ op ];
        
        result += opdata.mnemonic_;
        result += " ";
        
        var destRegnum : int = binary.readShort();
        var destWritemask : int = binary.readByte(); 
        var destRegtype : int = binary.readByte(); 

        if ( !(opdata.flags_ & f_nodest) ) 
        {   
            result += regtag_; 
            result += regtypes.charAt( destRegtype );

            if ( destRegtype != reg_output ) // remove check when we support multiple output regs
                result += destRegnum; 
            if ( destWritemask != 0xf ) 
            {
                result += '.'; 
                if ( destWritemask & 1 ) result += 'x'; 
                if ( destWritemask & 2 ) result += 'y'; 
                if ( destWritemask & 4 ) result += 'z'; 
                if ( destWritemask & 8 ) result += 'w'; 
            }
            if ( opdata.srcaflags_ ) result += ", ";
        } 

        result += disassembleSourceReg( binary, opdata.srcaflags_ );

        if ( opdata.srcbflags_ ) 
        {
            result += ", "; 
        }      

        result += disassembleSourceReg( binary, opdata.srcbflags_ );

        result += "\n";

        return result;
    }
    
    private function getRegType( binary : ByteArray ) : int
    {
        var originalOffset : int = binary.position;
        binary.position += 4;

        var regtype : int = binary.readByte(); 

        binary.position = originalOffset;

        return regtype;
    }
    
    private function disassembleSourceReg( binary : ByteArray, flags : int ) : String
    {
        var result : String = new String();

        if( getRegType( binary ) == reg_sampler )
        {
            result += disassembleSamplerReg( binary, flags );
        }
        else
        {
            result += disassembleNonSamplerReg( binary, flags );
        }
        
        return result;
    }

    private function disassembleSamplerReg( binary : ByteArray, flags : int ) : String
    {
        var specialstr : Vector.< String > = new Vector.<String >();
        specialstr.push( "centroid", "single", "depth");

        var dimstr : Vector.< String > = new Vector.<String >();
        dimstr.push( "2d","cube","3d" );

        var filterstr : Vector.< String > = new Vector.<String >();
        filterstr.push( "nearest","linear" );

        var mipstr : Vector.< String > = new Vector.<String >();
        mipstr.push( "nomip", "mipnearest", "miplinear" );

        var wrapstr : Vector.< String > = new Vector.<String >();
        wrapstr.push( "clamp", "repeat" );

        var result : String = new String();

        var regnum : int = binary.readShort();
        var reserved0 : int = binary.readShort();
        var regtype : int = binary.readByte(); 
        var t0 : int = binary.readByte();

        var dim : int = t0 & 0xf;
        
        t0 = binary.readByte();

        var special : int = ( t0 >> 4 ) & 0xf;
        var wrap : int = t0 & 0xf;
        
        t0 = binary.readByte();

        var mipmap : int = ( t0 >> 4 ) & 0xf;
        var filter : int = t0 & 0xf;

        if( flags )
        {
            result += regtag_ + "s" + regnum + " <" + dimstr[dim] + " " + filterstr[filter] + " " + mipstr[mipmap] + " " + wrapstr[wrap];

            for ( var i : int =0; i < specialstr.length; i++ )
                if ( (special >> i ) & 1 )
                    result += ", " + specialstr[ i ]; 
            result +='>';
        }
        
        return result;
    }
    
    private function disassembleNonSamplerReg( binary : ByteArray, flags : int ) : String
    {
        var swizzle_identity : int = 0xe4; 

        var result : String = new String();

        var regnum : int = binary.readShort();
        var indexoffset : int = binary.readByte(); 
        var swizzle : int = binary.readByte(); 
        var regtype : int = binary.readByte(); 
        var indextype : int = binary.readByte(); 
        var indexselect : int = binary.readByte(); 
        var indirectflag : int = binary.readByte(); 

        if( flags )
        {
            if ( indirectflag ) 
                result += regtag_ + regtypes.charAt(regtype) + "[" + regtag_ + regtypes.charAt(indextype) + regnum + "." + compstr.charAt( indexselect ) + "+" + indexoffset + "]";
            else 
                result += regtag_ + regtypes.charAt( regtype ) + regnum; 

            if ( swizzle == swizzle_identity && (flags & fs_vector) ) 
                return result; 

            result += '.';
            result += compstr.charAt( swizzle & 3);
            result += compstr.charAt( (swizzle>>2)&3);
            result += compstr.charAt( (swizzle>>4)&3);
            result += compstr.charAt( (swizzle>>6)&3);
        }
        
        return result;
    }
        

    // flags for opcode and destination
    internal static const f_nodest:int                         = 0x01; 
    internal static const f_incnest:int                = 0x02; 
    internal static const f_decnest:int                = 0x04; 
    internal static const f_fragonly:int               = 0x08; 
    //internal static const int8_t f_useme             = 0x10;
    internal static const f_outxyz:int                         = 0x20;
    internal static const f_flow:int                   = 0x40; 
    //internal static const int8_t f_useme             = 0x80;         
    internal static const f_notimpl:int                = 0xff; // opcode not implemented

    // source flags
    internal static const fs_scalar:int                = 0x01;
    internal static const fs_vector:int                = 0x02;
    internal static const fs_constscalar:int   = 0x04;
    internal static const fs_sampler:int               = 0x08;
    internal static const fs_matrix3:int               = 0x10; 
    internal static const fs_matrix4:int               = 0x20; 
    internal static const fs_noindirect:int    = 0x40; 

    public function getOpcodes() : Vector.<OpcodeData>
    {
        var result : Vector.<OpcodeData> = new Vector.<OpcodeData>();

        result.push(
            new OpcodeData( "mov", 0x00, 0, fs_vector, 0 ),
            new OpcodeData( "add", 0x01, 0, fs_vector, fs_vector ),
            new OpcodeData( "sub", 0x02, 0, fs_vector, fs_vector ),
            new OpcodeData( "mul", 0x03, 0, fs_vector, fs_vector ),
            new OpcodeData( "div", 0x04, 0, fs_vector, fs_vector ),
            new OpcodeData( "rcp", 0x05, 0, fs_vector, 0 ),
            new OpcodeData( "min", 0x06, 0, fs_vector, fs_vector ),
            new OpcodeData( "max", 0x07, 0, fs_vector, fs_vector ),
            new OpcodeData( "frc", 0x08, 0, fs_vector, 0 ),
            new OpcodeData( "sqt", 0x09, 0, fs_vector, 0 ),
            new OpcodeData( "rsq", 0x0a, 0, fs_vector, 0 ),
            new OpcodeData( "pow", 0x0b, 0, fs_vector, fs_vector ),
            new OpcodeData( "log", 0x0c, 0, fs_vector, 0 ),
            new OpcodeData( "exp", 0x0d, 0, fs_vector, 0 ),
            new OpcodeData( "nrm", 0x0e, f_outxyz, fs_vector, 0 ),
            new OpcodeData( "sin", 0x0f, 0, fs_vector, 0 ),
            new OpcodeData( "cos", 0x10, 0, fs_vector, 0 ),
            new OpcodeData( "crs", 0x11, 0, fs_vector, fs_vector ),
            new OpcodeData( "dp3", 0x12, 0, fs_vector, fs_vector ),
            new OpcodeData( "dp4", 0x13, 0, fs_vector, fs_vector ),
            new OpcodeData( "abs", 0x14, 0, fs_vector, 0 ),
            new OpcodeData( "neg", 0x15, 0, fs_vector, 0 ),
            new OpcodeData( "sat", 0x16, 0, fs_vector, 0 ),
            new OpcodeData( "m33", 0x17, f_outxyz, fs_vector, fs_matrix3 ),
            new OpcodeData( "m44", 0x18, 0, fs_vector, fs_matrix4 ),
            new OpcodeData( "m43", 0x19, f_outxyz, fs_vector, fs_matrix3 ),
            new OpcodeData( "ifz", 0x1a, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, 0 ),
            new OpcodeData( "inz", 0x1b, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, 0 ),
            new OpcodeData( "ife", 0x1c, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar ),
            new OpcodeData( "ine", 0x1d, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar ),
            new OpcodeData( "ifg", 0x1e, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar ),
            new OpcodeData( "ifl", 0x1f, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar ),
            new OpcodeData( "ieg", 0x20, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar ),
            new OpcodeData( "iel", 0x21, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar ),
            new OpcodeData( "els", 0x22, f_notimpl | f_nodest | f_incnest | f_decnest | f_flow, 0, 0 ),
            new OpcodeData( "eif", 0x23, f_notimpl | f_nodest | f_decnest | f_flow, 0, 0 ),
            new OpcodeData( "rep", 0x24, f_notimpl | f_nodest | f_incnest | f_flow, fs_constscalar, 0 ),
            new OpcodeData( "erp", 0x25, f_notimpl | f_nodest | f_decnest | f_flow, 0, 0 ),
            new OpcodeData( "brk", 0x26, f_notimpl | f_nodest | f_flow, 0, 0 ),
            new OpcodeData( "kil", 0x27, f_nodest | f_fragonly, fs_scalar, 0 ),
            new OpcodeData( "tex", 0x28, f_fragonly, fs_vector | fs_noindirect, fs_sampler | fs_noindirect ),
            new OpcodeData( "sge", 0x29, 0, fs_vector, fs_vector ),
            new OpcodeData( "slt", 0x2a, 0, fs_vector, fs_vector ),
            new OpcodeData( "sgn", 0x2b, f_notimpl, fs_vector, 0)
        );

        return result;
    }

    private var regtag_ : String;
}

}

{
        
class OpcodeData
{
    public var mnemonic_ : String;
    public var opcode_ : int;
    public var flags_ : int;
    public var srcaflags_ : int;
    public var srcbflags_ : int;
    
    public function OpcodeData( mnemonic : String, opcode : int, flags : int, srcaflags : int, srcbflags : int )
    {
        mnemonic_ = mnemonic;
        opcode_ = opcode;
        flags_ = flags;
        srcaflags_ = srcaflags;
        srcbflags_ = srcbflags;
    }
}

}


