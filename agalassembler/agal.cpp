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

#define VALIDATEASSERT PLAYERASSERT(0);
//#define VALIDATEASSERT


#include <string.h> 

#include "agal.h"
#include "parray.h" 
#include "MiniFlashString.h"
#include "MiniFlashAssert.h" 


using namespace::AGAL;

inline uint8_t SwizzleToMask ( uint8_t swiz ) {
	return (1<<((swiz>>0)&3)) | (1<<((swiz>>2)&3)) | (1<<((swiz>>4)&3)) | (1<<((swiz>>6)&3));
}

inline uint8_t SwizzleToMaskWithLimit ( uint8_t swiz, uint8_t limit ) {
	uint8_t r = 0;
	if ( limit & 1 ) r |= (1<<((swiz>>0)&3)); 
	if ( limit & 2 ) r |= (1<<((swiz>>2)&3)); 
	if ( limit & 4 ) r |= (1<<((swiz>>4)&3)); 
	if ( limit & 8 ) r |= (1<<((swiz>>6)&3)); 
	return r;
}

static bool CheckRegisterRange ( uint8_t regtype, uint16_t regnum, uint8_t shadertype ) {
	switch ( regtype ) {
		case reg_sampler:
			if ( regnum >= AGAL::max_sampler_count ) {				
				VALIDATEASSERT;
				return false;
			}
			break; 
		case reg_varying:
			if ( regnum >= AGAL::max_varying_count ) {				
				VALIDATEASSERT;
				return false;
			}
			break; 
		case reg_attrib:
			if ( regnum >= AGAL::max_attrib_count ) {				
				VALIDATEASSERT;
				return false;
			}
			break; 
		case reg_const:
			if ( (shadertype == shadertype_vertex && regnum >= AGAL::max_vertex_const_count) || 
				 (shadertype == shadertype_fragment && regnum >= AGAL::max_fragment_const_count) ) {				
				VALIDATEASSERT;
				return false;
			}
			break; 
		case reg_output:
			if ( regnum != 0 ) {				
				VALIDATEASSERT;
				return false;
			}
			break;
		case reg_temp:
			if ( regnum >= AGAL::max_temp_count ) {				
				VALIDATEASSERT;
				return false;
			}
			break; 
		case reg_int_temp:
		case reg_int_const:		
		case reg_int_addr:
		case reg_int_inline:
			break; 
		default:
			VALIDATEASSERT;
			return false;
	}
	return true; 
}

static bool VerifyAGALDestReg( const AGAL::Destination &d, uint8_t shadertype, bool allowint ) {
	if ( !CheckRegisterRange(d.regtype, d.regnum, shadertype_vertex ) ) {
		VALIDATEASSERT;
		return false; 
	}

	// If any of our reserved bits are set, fail
	// If allowint is true, our regtype could be one of our internal registers so our mask is different
	int regtypeMask = allowint ? 0xE0 : 0xF0;
	if (( d.writemask & 0xF0 ) || ( d.regtype & regtypeMask )) {		
		VALIDATEASSERT;
		return false; 
	}

	switch ( d.regtype ) {
		case reg_sampler:						
			VALIDATEASSERT;
			return false;			
			break; 
		case reg_varying:
			if ( shadertype != shadertype_vertex ) {				
				VALIDATEASSERT;
				return false;
			}
			break; 
		case reg_attrib:						
			VALIDATEASSERT;
			return false;			
			break; 					
		case reg_const:			
			VALIDATEASSERT;
			return false;			
			break; 							
		case reg_int_addr:
		case reg_int_temp:
		case reg_int_inline:
			if ( !allowint ) {
				VALIDATEASSERT; // we should have failed our kAGALReservedBitsShouldBeZero check above
				return false; 
			}
			break;
		case reg_output:		
			if ( shadertype == shadertype_fragment ) {
				if ( d.writemask != 0xf ) {					
					VALIDATEASSERT;
					return false;		
				}
			}
			break;
		case reg_temp:
			break; 
		default:			
			VALIDATEASSERT;
			return false;
	}
	if ( d.writemask == 0 ) {		
		VALIDATEASSERT;
		return false;
	}

	return true; 
}

static bool CheckSampler ( const AGAL::Sampler &sa ) {
	switch ( sa.dim ) {
		case AGAL::dim_2d:
			break; 			
		case AGAL::dim_cube:			
			if ( sa.wrap != AGAL::wrap_clamp ) {				
				VALIDATEASSERT;
				return false; 
			}
			break; 
		default:			
			VALIDATEASSERT;
			return false; 
	}
	switch ( sa.filter ) {
		case AGAL::filter_linear:
		case AGAL::filter_nearest:
			break;
		default:			
			VALIDATEASSERT;
			return false; 
	}
	switch ( sa.mipmap ) {
		case AGAL::mipmap_none:
		case AGAL::mipmap_nearest:
		case AGAL::mipmap_linear:
			break; 
		default:			
			VALIDATEASSERT;
			return false; 
	}
	switch ( sa.wrap ) {
		case AGAL::wrap_clamp:
		case AGAL::wrap_repeat:
			break;
		default:			
			VALIDATEASSERT;
			return false; 
	}
	
	// no special flags are supported in this release
	if ( sa.special ) {
	//if ( (sa.special & ~(AGAL::special_depth|AGAL::special_centroid|AGAL::special_single))!=0 ) {		
		VALIDATEASSERT;
		return false; 
	}
	return true;
}

static bool VerifyAGALSourceReg ( const AGAL::Source &s, uint8_t flags, uint8_t shadertype, bool allowint, const uint8_t *tempwritten ) {
	if ( flags==0 ) { // empty
		if ( s.packed != 0 ) {			
			VALIDATEASSERT;
			return false; 
		} else {
			return true;
		}
	}

	if ( s.regtype == reg_sampler ) {		
		if ( !CheckRegisterRange(s.regtype, s.regnum, shadertype ) ) {
			VALIDATEASSERT;
			return false; 	
		}
		if ( shadertype != shadertype_fragment ) {			
			VALIDATEASSERT;
			return false;
		}		
		if ( (flags&fs_sampler)==0 ) {			
			VALIDATEASSERT;
			return false;
		}
		AGAL::Sampler sa; 
		sa.packed = s.packed; 
		return CheckSampler ( sa );
	} 
	else {
		if ( flags&fs_sampler ) {			
			VALIDATEASSERT;
			return false;
		}
	}
	
	// this does not check AGAL code but our opmap table
	//PLAYERASSERT( (flags&(fs_vector|fs_scalar|fs_sampler|fs_matrix3|fs_matrix4))!=0 );

	// If any of our reserved bits are set, fail
	// If allowint is true, our regtype could be one of our internal registers so our mask is different
	int regtypeMask = allowint ? 0xE0 : 0xF0;
	if (( s.indirectflag & 0x7F ) || ( s.indexselect & 0xFC ) || ( s.indextype & regtypeMask ) || ( s.regtype & regtypeMask )) {		
		VALIDATEASSERT;
		return false; 
	}

	if ( s.indirectflag  ) {
		if ( shadertype!=shadertype_vertex ) {			
			VALIDATEASSERT;
			return false; 
		}
		if ( s.regtype != reg_const ) {			
			VALIDATEASSERT;
			return false; 
		}
		if ( flags & fs_noindirect ) {			
			VALIDATEASSERT;
			return false; 
		}

		if ( !CheckRegisterRange(s.indextype, s.regnum, shadertype_vertex ) ) {
			VALIDATEASSERT;
			return false; 
		}

		switch ( s.indextype ) {
			case reg_attrib:
			case reg_const:
				break;
			case reg_temp:
				if ( !tempwritten[s.regnum] ) {					
					VALIDATEASSERT;
					return false;					
				} else {
					if ( tempwritten[s.regnum]!=destwrite_xyzw ) { // don't bother if all are valid
						// check partial read												
						uint8_t needvalidmask = s.indirectflag?1<<s.indexselect:SwizzleToMask ( s.swizzle );
						if ( (tempwritten[s.regnum] & needvalidmask) != needvalidmask ) {							
							VALIDATEASSERT;
							return false; 
						}
					}
				}
				break; 
			case reg_int_temp:				
			case reg_int_addr:
				if (allowint) break;
				VALIDATEASSERT; // we should have failed our kAGALReservedBitsShouldBeZero check above
				return false; 
			default:				
				VALIDATEASSERT;
				return false; 			
		}
	} else {
		if ( s.indexoffset!=0 || s.indexselect!=0 || s.indextype!=0 ) {			
			VALIDATEASSERT;
			return false; 
		}	

		if ( !CheckRegisterRange(s.regtype, s.regnum, shadertype ) ) {
			VALIDATEASSERT;
			return false; 
		}
		int nmore = 0; 	
		if ( flags & fs_matrix3 ) nmore = 2; 
		if ( flags & fs_matrix4 ) nmore = 3; 		
		for ( int i=0; i<nmore; i++ ) {
			if ( !CheckRegisterRange(s.regtype, uint16_t(s.regnum+i+1), shadertype ) ) {
				VALIDATEASSERT;
				return false; 
			}
		}		
		if ( flags & fs_scalar ) {
			if ( GetScalarInput (s) == -1 ) {
				VALIDATEASSERT;				
				return false; 
			}
		}

		switch ( s.regtype ) {
			case reg_sampler:
				VALIDATEASSERT; // already checked
				break; 
			case reg_varying:
				if ( shadertype != shadertype_fragment ) {					
					VALIDATEASSERT;
					return false;
				}
				break; 
			case reg_attrib:
				if ( shadertype != shadertype_vertex ) {					
					VALIDATEASSERT;
					return false;
				}
				break; 					
			case reg_output:				
				VALIDATEASSERT;
				return false;
				break;
			case reg_const:
				break; 
			case reg_temp:
				if ( !tempwritten[s.regnum] ) {					
					VALIDATEASSERT;
					return false;					
				} else {
					if ( tempwritten[s.regnum]!=destwrite_xyzw ) { // don't bother if all are valid
						// check partial read
						uint8_t needvalidmask = SwizzleToMask ( s.swizzle );
						if ( (tempwritten[s.regnum] & needvalidmask) != needvalidmask ) {							
							VALIDATEASSERT;
							return false; 
						}
					}
				}
				break; 			
			case reg_int_temp:
			case reg_int_const:			
			case reg_int_addr:
			case reg_int_inline:
				if ( allowint ) break;
				VALIDATEASSERT; // we should have failed our kAGALReservedBitsShouldBeZero check above				
				return false; 
			default:				
				VALIDATEASSERT;
				return false;
		}
	}

	return true; 
}


bool AGAL::Validate ( const char *bytecode, size_t bytecodelen, bool allowinternal ) {			
	if ( !bytecode ) return false; 
	if ( bytecodelen < sizeof(AGAL::Header)+sizeof(AGAL::Opcode) ) {		
		VALIDATEASSERT;
		return false; 
	}
	const AGAL::Header *aheader = (const AGAL::Header *)bytecode; 
	if ( aheader->magic != AGAL::magic ) {		
		VALIDATEASSERT;
		return false; 
	}
	if ( aheader->version > AGAL::current_version ) {		
		VALIDATEASSERT;
		return false; 
	}
	if ( aheader->shadertypeopcode != AGAL::shadertype_opcode ) {		
		VALIDATEASSERT;
		return false; 
	}		
	if ( aheader->shadertype != AGAL::shadertype_fragment && aheader->shadertype != AGAL::shadertype_vertex ) {		
		VALIDATEASSERT;
		return false; 
	}		

	AGAL::Sampler samplerused[AGAL::max_sampler_count] = {0};
	uint8_t tempwritten[AGAL::max_temp_count] = {0}; 
	unsigned int readpos = sizeof(AGAL::Header); 	
	unsigned int tokenpos = 1; 
	int nest = 0; 
	while ( readpos <= bytecodelen-sizeof(AGAL::Opcode) ) {		
		const AGAL::Opcode *op = (const AGAL::Opcode*)(bytecode+readpos);
		if ( op->op >= (int) AGAL::opmap_max ) {			
			VALIDATEASSERT;
			return false; 
		}		
		if ( opmap[op->op].flags == f_notimpl ) {			
			VALIDATEASSERT;
			return false; 
		}					
		if ( (opmap[op->op].flags & f_fragonly)!=0 && (aheader->shadertype != AGAL::shadertype_fragment) ) {			
			VALIDATEASSERT;
			return false; 
		}
		if ( (opmap[op->op].flags & f_decnest)!=0 ) nest--; 
		if ( (opmap[op->op].flags & f_incnest)!=0 ) nest++; // else does both, so do inc/dec before check
		if ( nest < 0 ) {			
			VALIDATEASSERT;
			return false; 
		}
		if ( nest > max_nesting ) {			
			VALIDATEASSERT;
			return false; 
		}		
				
		if ( !VerifyAGALSourceReg( op->sourcea, AGAL::opmap[op->op].srcaflags, aheader->shadertype, allowinternal, tempwritten ) ) {		
			VALIDATEASSERT;
			return false; 
		}		
		if ( !VerifyAGALSourceReg( op->sourceb, AGAL::opmap[op->op].srcbflags, aheader->shadertype, allowinternal, tempwritten ) ) {
			VALIDATEASSERT;
			return false; 
		}
		if ( AGAL::opmap[op->op].srcaflags ) {
			if ( AGAL::opmap[op->op].srcbflags ) {
				if ( op->sourcea.regtype == AGAL::reg_const && op->sourceb.regtype == AGAL::reg_const ) {					
					VALIDATEASSERT;
					return false; 
				}
				if ( opmap[op->op].flags & f_secsrcnoswizzle ) {
					if ( op->sourceb.swizzle != swizzle_identity ) {						
						VALIDATEASSERT;
						return false; 
					}
				}
			} else {
				// if we are in a post - unpack pass, this is ok: for example 
				// "div reg, reg, const" -> "rcp temp, const; mul reg, reg, temp" 
				if ( !allowinternal && op->sourcea.regtype == AGAL::reg_const && op->op != op_mov ) {					
					VALIDATEASSERT;
					return false; 
				}
			}
		}
		if ( op->op != op_tex ) {
			if ( AGAL::opmap[op->op].srcaflags && AGAL::opmap[op->op].srcbflags ) {
				if ( op->sourcea.indirectflag && op->sourceb.indirectflag ) {					
					VALIDATEASSERT;
					return false; 
				}
			}
		}

		if ( (opmap[op->op].flags & f_nodest)!=0 ) {
			if ( op->dest.packed!=0 ) {				
				VALIDATEASSERT;
				return false; 
			}
		} else {
			if ( !VerifyAGALDestReg( op->dest, aheader->shadertype, allowinternal ) ) {
				VALIDATEASSERT;
				return false; 
			}
			if ( opmap[op->op].flags & f_outxyz ) {
				if ( op->dest.writemask != 7 ) { // must be exactly xyz?
					VALIDATEASSERT;
					return false; 
				}
			}
			if ( op->dest.regtype == AGAL::reg_temp ) 
				tempwritten[op->dest.regnum] |= op->dest.writemask; 
		}

		// extra check, samplers must use same filter/type if multiply referenced		
		if ( op->op == AGAL::op_tex ) {
			if ( samplerused[op->sampler.regnum].regtype != AGAL::reg_sampler ) { // first use
				samplerused[op->sampler.regnum] = op->sampler; 
			} else {
				if ( samplerused[op->sampler.regnum].packed != op->sampler.packed ) {					
					VALIDATEASSERT;
					return false; 
				}
			}
		}

		if ( tokenpos > AGAL::max_tokens && !allowinternal ) {			
			VALIDATEASSERT;
			return false; 
		}			
		tokenpos++;
		readpos += sizeof(AGAL::Opcode); 
	}

	return true; 
}

static void DescribeAddBatch ( AGAL::Description * dest, int start, int len ) {
	if ( dest->numbatches >= AGAL::max_const_batch_count ) {
		// merge with last one 
		uint16_t end = (uint16_t)(start+len); 
		dest->constbatches[AGAL::max_const_batch_count-1].len = end - dest->constbatches[AGAL::max_const_batch_count-1].start;
	} else {
		// add batch - could merge some on small gaps?
		dest->constbatches[dest->numbatches].start = (uint16_t)start;
		dest->constbatches[dest->numbatches].len = (uint16_t)len;
		dest->numbatches++;
	}
}

static void DescribeUpdateSource ( const AGAL::Source &s, AGAL::Description * dest, uint32_t srcCount ) {
	uint8_t v, rt;
	if ( s.indirectflag && s.regtype != AGAL::reg_sampler ) {
		rt = s.indextype; 
		v = 1<<s.indexselect;
		dest->hasindirect = true;			
	} else {
		rt = s.regtype;
		v = SwizzleToMask ( s.swizzle ); 		
	}

	switch ( rt ) {
		case AGAL::reg_varying:
			for (uint32_t i = 0; i < srcCount; i++)
				dest->read_varying[s.regnum + i] |= v;
			break; 
		case AGAL::reg_attrib:
			for (uint32_t i = 0; i < srcCount; i++)
				dest->read_attribute[s.regnum + i] |= v;
			break; 
		case AGAL::reg_const:
			for (uint32_t i = 0; i < srcCount; i++)
				dest->read_const[s.regnum + i] |= v;
			break; 
		case AGAL::reg_temp:
			//PLAYERASSERT ( (dest->readwrite_temp[s.regnum] & v) == v );
			for (uint32_t i = 0; i < srcCount; i++)
				dest->readwrite_temp[s.regnum + i] |= v;
			break; 
		case AGAL::reg_int_const:
			for (uint32_t i = 0; i < srcCount; i++)
				dest->read_int_const[s.regnum + i] |= v;
			break;
		case AGAL::reg_int_temp:
			for (uint32_t i = 0; i < srcCount; i++)
				dest->readwrite_int_temp[s.regnum + i] |= v;
			break;
		case AGAL::reg_int_addr:
			dest->readwrite_int_addr |= v;
			break;
	}
}

void AGAL::Describe ( const char *bytecode, size_t bytecodelen, Description *dest ) {	
	PLAYERASSERT ( dest && Validate ( bytecode, bytecodelen, true ) ); 

	memset ( dest, 0, sizeof(Description) );

	const AGAL::Header *aheader = (const AGAL::Header *)bytecode; 
	dest->shadertype = aheader->shadertype; 

	unsigned int readpos = sizeof(Header); 	
	while ( readpos <= bytecodelen-sizeof(Opcode) ) {
		const Opcode *op = (const Opcode*)(bytecode+readpos);
		
		if ( AGAL::opmap[op->op].srcaflags )
			DescribeUpdateSource ( op->sourcea, dest, 1 );
		if ( AGAL::opmap[op->op].srcbflags ) {
			int srcCount = 1;
			if ( op->op == op_m44)
				srcCount = 4;
			else if (op->op == op_m43 || op->op == op_m33 )
				srcCount = 3;

			DescribeUpdateSource ( op->sourceb, dest, srcCount );
		}

		if ( !(opmap[op->op].flags & f_nodest) ) {
			switch ( op->dest.regtype ) { 
				case reg_output: 
					dest->write_output |= op->dest.writemask; 
					break; 
				case reg_varying:
					dest->write_varying[op->dest.regnum] |= op->dest.writemask; 
					break; 
				case reg_temp:
					dest->readwrite_temp[op->dest.regnum] |= op->dest.writemask; 
					break; 
				case reg_int_temp:
					dest->readwrite_int_temp[op->dest.regnum] |= op->dest.writemask; 
					break; 
				case reg_int_addr:
					dest->readwrite_int_addr |= op->dest.writemask; 
					break; 
			}
		}		

		// special opcode counting
		switch ( op->op ) {
			case op_sin:
			case op_cos:
				dest->hassincos = true; 
				break;
			case op_sge:
			case op_slt: 
			case op_sgn:
			case op_seq:
			case op_sne:
				dest->haspredicated = true; 
				break;
			case op_kil: 
				dest->numkil++;
				dest->haspredicated = true; 
				break;
			case op_tex:
				dest->numtex++;
				dest->samplers[op->sampler.regnum] = op->sampler; 			
				break;
		}	

		dest->numinstructions++; 
		readpos += sizeof(Opcode); 
	}
	if ( dest->hasindirect ) // flag all const regs for potential read
		memset ( dest->read_const, 0xf, sizeof ( dest->read_const ) );

	// mark batches 
	int n = dest->shadertype == shadertype_fragment?max_fragment_const_count:max_vertex_const_count;
	int batchstart = -1;
	int batchlen = 0;
	dest->numbatches = 0; 
	for ( int i=0; i<n; i++ ) {
		if ( dest->read_const[i] ) {
			if ( batchstart==-1 ) {
				batchstart=i;
				batchlen=1;
			} else {
				batchlen++;
			}
		} else {
			if ( batchstart!=-1 ) {
				DescribeAddBatch ( dest, batchstart, batchlen );
				batchstart=-1;				
			}
		}
	}
	if ( batchstart!=-1 ) 
		DescribeAddBatch ( dest, batchstart, batchlen );
}

bool AGAL::ValidateLinkage ( const Description *frag, const Description *vert ) {
	PLAYERASSERT ( frag && vert );

	if ( frag->shadertype != shadertype_fragment ) { 		
		return false; 
	}
	if ( vert->shadertype != shadertype_vertex ) { 		
		return false; 
	}

	// check varying read/write
	for ( int i=0; i<max_varying_count; i++ ) {
		if ( (frag->read_varying[i] & vert->write_varying[i]) != frag->read_varying[i] ) {			
			return false;
		}
		if ( vert->write_varying[i]!=0 && vert->write_varying[i]!=destwrite_xyzw ) {			
			return false;
		}
	}
	// check outputs
	if ( frag->write_output != destwrite_xyzw ) {		
		return false;
	}
	if ( vert->write_output != destwrite_xyzw ) {		
		return false;
	}
	// attributes must check stream setup during draw validation

	return true; 
}


bool AGAL::IsSameRegister ( const Destination &d, const Source &s ) {
	return s.regtype==d.regtype && s.regnum==d.regnum; 
}

int AGAL::GetScalarInput ( const Source &s ) {
	switch ( s.swizzle ) {
		case swizzle_all_x: return 0; 
		case swizzle_all_y: return 1;
		case swizzle_all_z: return 2;
		case swizzle_all_w: return 3; 
		default: return -1; 
	}
}

int AGAL::GetSwizzledChannel ( const Source &s, int basechannel ) {	
	return (s.swizzle>>(basechannel<<1))&3; 
}

int AGAL::GetScalarOutput ( const Destination &d ) {
	PLAYERASSERT ( d.writemask < destwrite_xyzw ); 	
	switch ( d.writemask ) {
		case 1: return 0; 
		case 2: return 1;
		case 4: return 2;
		case 8: return 3; 
		default: return -1; 
	}
}

bool AGAL::IsLinearSwizzle ( const Source &s ) {
	return ((s.swizzle>>0)&3)<=((s.swizzle>>2)&3) && 
		   ((s.swizzle>>2)&3)<=((s.swizzle>>4)&3) && 
		   ((s.swizzle>>4)&3)<=((s.swizzle>>6)&3);
}

uint8_t AGAL::ReplicateLastComponent ( uint8_t swiz ) {
	return (swiz&0x3f)|((swiz<<2)&0xc0); 
}

uint8_t AGAL::ReplicateSwizzle ( uint8_t c ) {
	PLAYERASSERT ( c<=3 ); 
	return (c<<6)|(c<<4)|(c<<2)|(c); 
}

// Given a destwrite mask, extract source component and replicate it across all source components
uint8_t AGAL::ExtractAndReplicateSwizzle ( uint8_t srcSwizzle, uint8_t deskwriteMask ) {
	uint8_t c = 0;
	switch ( deskwriteMask ) {
		case 1: c = srcSwizzle >> 0; break; 
		case 2: c = srcSwizzle >> 2; break;
		case 4: c = srcSwizzle >> 4; break;
		case 8: c = srcSwizzle >> 6; break;
		default:
			PLAYERASSERT(0);
	}

	c = c & 0x3;
	return (c<<6)|(c<<4)|(c<<2)|(c); 
}

bool AGAL::Unpack ( const char *bytecode, size_t bytecodelen, uint32_t flags, char **newbytecode, size_t *newbytecodelen, uint16_t pass ) {
	PLAYERASSERT ( newbytecode && newbytecodelen && flags );
		
	PLAYERASSERT ( Validate ( bytecode, bytecodelen, pass!=0 ) ); 
	PLAYERASSERT ( pass<AGAL::max_temp_count ); 
	if ( pass>=AGAL::max_temp_count ) return false;
	//const AGAL::Header *aheader = (const AGAL::Header *)bytecode; 	
	PArray<char> outbuf; 
	outbuf.AppendArray ( bytecode, sizeof(Header) ); 
	unsigned int readpos = sizeof(Header); 	
	Source curaddrsrc; 
	curaddrsrc.packed = ~uint64_t(0); 

	while ( readpos <= bytecodelen-sizeof(Opcode) ) {
		const Opcode *op = (const Opcode*)(bytecode+readpos);
		bool justcopy = true; 
		// unpack addr if required		
		if ( flags & unpack_addr ) {	
			if ( op->dest.regnum == curaddrsrc.regnum &&
				op->dest.regtype == curaddrsrc.regtype &&
				(op->dest.writemask & (1<<(curaddrsrc.swizzle&3)))!=0 ) {
				// invalidate curent address register source, it's being written to 				
				curaddrsrc.packed = ~uint64_t(0); 
			}
			const Source *sind = 0; 			
			if ( (opmap[op->op].srcaflags&(~fs_sampler))!=0 && op->sourcea.indirectflag ) sind = &op->sourcea;									
			else if ( (opmap[op->op].srcbflags&(~fs_sampler))!=0 && op->sourceb.indirectflag ) sind = &op->sourceb;									
			if ( sind && sind->indextype!=reg_int_addr ) {								
				// turn: op dest.mask, a, regtype[indextype(regnum).indexselect+indexoffset].swizzle
				// to: 
				// mov addr.x, indextype(regnum).indexselect
				// op dest.mask, a, regtype[addr.x + indexoffset].swizzle								
				Opcode writeop; 				
				writeop.op = op_mov; 
				writeop.dest.Set ( reg_int_addr, 0, 1 );
				writeop.sourcea.Set ( sind->indextype, sind->regnum, ReplicateSwizzle(sind->indexselect) );
				writeop.sourceb.packed = 0;
				if ( curaddrsrc.packed != writeop.sourcea.packed ) {
					outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
					curaddrsrc.packed = writeop.sourcea.packed;
				}
				writeop = *op; 
				Source writesource = *sind; 
				writesource.indextype = reg_int_addr;
				writesource.indexselect = 0;
				writesource.regnum = 0; 
				if ( sind==&op->sourcea ) writeop.sourcea = writesource;
				else writeop.sourceb = writesource;							
				outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
				justcopy = false; 
			}
		}

		// D3D complains if source a is used with indirect mode so split it into a mov
		// followed by the op.
		if ( justcopy && ( opmap[op->op].srcbflags && flags & unpack_srca_indirect ) ) {
			const Source *sind = 0; 			
			if ( (opmap[op->op].srcaflags&(~fs_sampler))!=0 && op->sourcea.indirectflag ) sind = &op->sourcea;									
			if (sind) {
				// indirect move
				Opcode writeop; 				
				writeop.op = op_mov; 
				writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw );
				writeop.sourcea.packed = op->sourcea.packed;
				writeop.sourceb.packed = 0;
				outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );

				// direct op
				writeop = *op; 
				writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
				outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
				justcopy = false;            
			}
		}

		// Two operand opcode with both source operands attribute registers
		if ( justcopy && (flags & unpack_only_one_attrib_reg) && op->sourcea.regtype == reg_attrib && 
				op->sourceb.regtype == reg_attrib && opmap[op->op].srcbflags ) {
			// mov, then op
			Opcode writeop;					
			writeop.op = op_mov;
			writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); 
			writeop.sourcea = op->sourcea;
			writeop.sourceb.packed = 0;
			outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
			writeop = *op;
			writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
			outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
			justcopy = false;            
        }

		// Two operand opcode with both source operands varying registers
		if ( justcopy && (flags & unpack_only_one_varying_reg) && op->sourcea.regtype == reg_varying && 
				op->sourceb.regtype == reg_varying && opmap[op->op].srcbflags ) {
			// mov, then op
			Opcode writeop;					
			writeop.op = op_mov;
			writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); 
			writeop.sourcea = op->sourcea;
			writeop.sourceb.packed = 0;
			outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
			writeop = *op;
			writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
			outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
			justcopy = false;            
        }

		// Only mov ops are supported to fragment output.  
		// No swizzle allowed on the mov.
		if ( justcopy && ( flags & unpack_frag_output_limited ) && op->dest.regtype == reg_output ) {
			if ( op->op != op_mov || op->sourcea.swizzle != swizzle_identity ) {
				// op then mov
				Opcode writeop;					
				writeop = *op;
				writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); 
				outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
				writeop = *op;
				writeop.op = op_mov;
				writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
				writeop.sourceb.packed = 0;
				outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
				justcopy = false;            
			}
		}

		// If our op only supports one source component but we are using multiple, split 
		// it into the component parts for each destination component.
		// We don't change .x, .y, .z, .w, or .xxxx, .yyyy, .zzzz, .wwww for source
		if ( justcopy && ( flags & unpack_one_component ) && ( GetScalarInput(op->sourcea) == -1 || GetScalarInput(op->sourceb) == -1 ) ) {
			switch ( op->op ) {
				case op_sqt:
				case op_rcp: 
				case op_rsq: 
				case op_pow: 
				case op_log: 
				case op_exp: 
				{
					justcopy = false;
					Opcode writeop = *op;
					static uint8_t destmask[4] = {destwrite_x, destwrite_y, destwrite_z, destwrite_w};
					for (int i = 0; i < 4; i++) {
						if ( op->dest.writemask & destmask[i] ) {
							writeop.dest.Set( op->dest.regtype, op->dest.regnum, destmask[i] );
							writeop.sourcea.Set( op->sourcea.regtype, op->sourcea.regnum, ExtractAndReplicateSwizzle( op->sourcea.swizzle, destmask[i] ) );
							if (op->sourceb.packed)
								writeop.sourceb.Set( op->sourceb.regtype, op->sourceb.regnum, ExtractAndReplicateSwizzle( op->sourceb.swizzle, destmask[i] ) );
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						}
					}
					break;
				}
				default:
					;
			}
		}

		if ( justcopy ) {
			switch ( op->op ) {
				case op_kil:
					if ( flags & unpack_kill ) {
						if ( op->sourcea.regtype != reg_int_temp ) {
							Opcode writeop; 
							writeop.op = op_mov; // mov to all in temp
							writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw );
							PLAYERASSERT (( GetScalarInput(op->sourcea)!=-1 ));
							writeop.sourcea = op->sourcea; 							
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							writeop.op = op_kil;						
							writeop.dest.packed = 0;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_all_x ); // swizzle is ignored
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							justcopy = false;
						}
					}
					break; 
				case op_sat:
					if ( flags & unpack_sat ) {
						// mov our constant into a temp in case sourca is a const (2 const failure on d3d)
						Opcode writeop; 
						writeop.op = op_mov;
						writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw );
						writeop.sourcea.Set ( reg_int_const, 0, swizzle_all_y ); //1
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						// sat turns into x = max ( 0, min ( x, 1 ) )
						writeop.op = op_min;
						writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw );
						writeop.sourcea = op->sourcea;
						writeop.sourceb.Set ( reg_int_temp, 0, swizzle_identity ); //from mov above
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						writeop.op = op_max;
						writeop.dest = op->dest;
						writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
						writeop.sourceb.Set ( reg_int_const, 0, swizzle_all_x ); //0
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
					}
					break;
				case op_nrm:
					if ( flags & unpack_nrm ) {
						// nrm unpacks into x * rsq ( dot3 (x,x) )
						PLAYERASSERT ( (flags & unpack_nrm_need_temp) == 0 ); // will be ok though
						Opcode writeop;					
						writeop.op = op_dp3;
						writeop.dest.Set ( reg_int_temp, pass, destwrite_x ); // only x
						writeop.sourcea = op->sourcea;
						writeop.sourceb = op->sourcea;
						writeop.sourcea.swizzle = ReplicateLastComponent ( writeop.sourcea.swizzle ); 
						writeop.sourceb.swizzle = ReplicateLastComponent ( writeop.sourceb.swizzle ); 
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						writeop.op = op_rsq;
						writeop.dest.Set ( reg_int_temp, pass, destwrite_x ); // only x
						writeop.sourcea.Set ( reg_int_temp, pass, swizzle_all_x );
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						writeop.op = op_mul;
						writeop.dest = op->dest;
						writeop.sourcea.Set ( reg_int_temp, pass, swizzle_all_x );
						writeop.sourceb = op->sourcea; 
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
					} else if ( flags & unpack_nrm_need_temp ) {
						if ( IsSameRegister ( op->dest, op->sourcea ) ) {
							// mov, then nrm
							Opcode writeop;					
							writeop.op = op_mov;
							writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); 
							writeop.sourcea = op->sourcea;
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							writeop.op = op_nrm;
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							justcopy = false;
						}
					}

					// D3D: "Dest writemask for NRM must be .xyzw (default) or .xyz"
					// OpenGL: silent failure.  Wants source to be .xyzz (not default or .xyzw)
					if ( justcopy && ( flags & unpack_nrm_destmask_must_be_xyz ) && ( op->dest.writemask < destwrite_xyz ) ) {
						Opcode writeop;					
						writeop.op = op_nrm;
						writeop.dest.Set ( reg_int_temp, pass, destwrite_xyz ); 
						writeop.sourcea = op->sourcea;
						writeop.sourcea.swizzle = swizzle_xyzz;
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						writeop.op = op_mov;
						writeop.dest = op->dest;
						writeop.sourcea.Set ( reg_int_temp, pass, op->sourcea.swizzle );
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
					}
					break; 
                case op_pow:
                    if ( flags & unpack_op_writes_to_temp ) {
                        if ( op->dest.regtype != reg_int_temp && op->dest.regtype != reg_temp ) {
							// pow to temp.xyz, then mov
							Opcode writeop;					
							writeop = *op; 
							writeop.dest.Set ( reg_int_temp, pass, op->dest.writemask );
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );

							writeop.op = op_mov; 
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							justcopy = false;
							break;
						}
                    }
					if (( flags & unpack_pow_diff_dest_sourceb ) && op->dest.regnum == op->sourceb.regnum && 
						op->dest.regtype == op->sourceb.regtype ) {
						Opcode writeop;					
						writeop = *op; 
						writeop.dest.Set ( reg_int_temp, pass, op->dest.writemask );
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );

						writeop.op = op_mov; 
						writeop.dest = op->dest;
						writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
						break;
					}
					break;
                case op_crs:
                    if ( flags & unpack_op_writes_to_temp ) {
                        if ( op->dest.regtype != reg_int_temp && op->dest.regtype != reg_temp ) {
							// crs to temp.xyz, then mov
							Opcode writeop;					
							writeop = *op; 
							writeop.dest.Set ( reg_int_temp, pass, op->dest.writemask );
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );

							writeop.op = op_mov; 
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							justcopy = false;
							break;
						}
                    }
					if ( flags & unpack_crs_nosrcswizzle ) {
						if ( op->sourcea.swizzle != swizzle_identity ) { 
							Opcode writeop;					
							writeop.op = op_mov;
							writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); // full writemask 
							writeop.sourcea = op->sourcea;
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );							
							writeop.op = op_crs;
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sourceb.packed = op->sourceb.packed;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );						
							justcopy = false;
							break;
						}
						else if ( op->sourceb.swizzle != swizzle_identity ) { 
							Opcode writeop;					
							writeop.op = op_mov;
							writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); // full writemask 
							writeop.sourcea = op->sourceb;
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );							
							writeop.op = op_crs;
							writeop.dest = op->dest;
							writeop.sourcea.packed = op->sourcea.packed;
							writeop.sourceb.Set ( reg_int_temp, pass, swizzle_identity );
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );						
							justcopy = false;
							break;
						}
					}
					if ( flags & unpack_crs_diff_dest_source ) {
						if (( op->dest.regnum == op->sourcea.regnum && op->dest.regtype == op->sourcea.regtype ) ||
							(op->dest.regnum == op->sourceb.regnum && op->dest.regtype == op->sourceb.regtype )) {
							Opcode writeop;					
							writeop = *op; 
							writeop.dest.Set ( reg_int_temp, pass, op->dest.writemask );
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );

							writeop.op = op_mov; 
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							justcopy = false;
							break;
						}
					}
                    break;
				case op_sqt:
					if ( flags & unpack_sqrt ) { // sqrt-> rcp/rsq
						Opcode writeop;					
						writeop.op = op_rcp;
						writeop.dest.Set ( reg_int_temp, pass, op->dest.writemask );  // write to N components?
						writeop.sourcea = op->sourcea;
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						writeop.op = op_rsq;
						writeop.dest = op->dest;
						writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
					}
					break;
				case op_div: 
					if ( flags & unpack_div ) { // div-> rcp/mul (add one newton iteration?) 
						Opcode writeop;					
						writeop.op = op_rcp;
						writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); 
						writeop.sourcea = op->sourceb;
						writeop.sourceb.packed = 0;
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						writeop.op = op_mul;
						writeop.dest = op->dest;
						writeop.sourcea = op->sourcea; 
						writeop.sourceb.Set ( reg_int_temp, pass, swizzle_identity );
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
					}
					break; 
				case op_neg: 
					if ( flags & unpack_neg ) { // sub ( 0, x )
						Opcode writeop;					
						writeop.op = op_sub;
						writeop.dest = op->dest;
						writeop.sourcea.Set ( reg_int_const, 0, swizzle_all_x ); //0
						writeop.sourceb = op->sourcea; 
						outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
						justcopy = false;
					}
					break; 
				case op_m44:
				case op_m43:
				case op_m33:
					{
						int nlines = op->op==op_m44?4:3;
						bool dounpack = (flags & unpack_matrix)!= 0; 
						if ( !dounpack && flags & unpack_matrix_destmasked ) { // if destination is masked, need to unpack 							
							if ( op->dest.writemask != (1<<nlines)-1 ) 
								dounpack = true; 
						}
						if ( dounpack ) { // dp4/dp3 to temp, then mov temp to dest							
							uint8_t lineop = op->op==op_m33?op_dp3:op_dp4;						
							for ( uint8_t i=0; i<nlines; i++ ) {
								Opcode writeop;				
								writeop.op = lineop;
								writeop.dest.Set ( reg_int_temp, pass, uint8_t(1<<i) );
								writeop.sourcea = op->sourcea; 
								writeop.sourceb = op->sourceb;
								if ( lineop==op_dp3 ) {
									writeop.sourcea.swizzle = ReplicateLastComponent ( writeop.sourcea.swizzle ); 
									writeop.sourceb.swizzle = ReplicateLastComponent ( writeop.sourceb.swizzle ); 
								}
								if ( writeop.sourceb.indirectflag ) writeop.sourceb.indexoffset+=i;
								else writeop.sourceb.regnum+=i;
								outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							}
							Opcode writeop;				
							writeop.op = op_mov;
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, op->op==op_m44 ? swizzle_identity : swizzle_xyzz ); 
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
							justcopy = false;
							break; 
						}
						if ( flags & unpack_matrix_diff_dest_source ) {
							bool dotempmatrix = false; 
							if ( op->dest.regnum == op->sourcea.regnum && op->dest.regtype == op->sourcea.regtype ) 
								dotempmatrix = true;
							else if ( op->dest.regtype == op->sourceb.regtype ) {								
								if ( op->dest.regnum >= op->sourceb.regnum && op->dest.regnum < op->sourceb.regnum+nlines ) 
									dotempmatrix = true;
							}						

							if ( dotempmatrix ) {
								Opcode writeop;					
								writeop = *op; 
								writeop.dest.Set ( reg_int_temp, pass, op->dest.writemask );
								outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );

								writeop.op = op_mov; 
								writeop.dest = op->dest;
								writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
								writeop.sourceb.packed = 0;
								outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );
								justcopy = false;
								break; 
							}
						}
					}
					break; 
				case op_tex:
					if ( flags & unpack_single_tex ) {
						if ( op->sampler.special & special_single ) { // only in pass 0
							if ( op->dest.writemask != 1 ) { // this is ok, x alone is always read correctly - and this is what we turn this into anyway
								Opcode writeop;					
								writeop.op = op_tex;
								writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); // full writemask so we are ok with tex writemask unpacks
								writeop.sourcea = op->sourcea;
								writeop.sampler = op->sampler;
								writeop.sampler.special &= ~special_single; 
								outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );							
								writeop.op = op_mov;
								writeop.dest = op->dest;
								writeop.sourcea.Set ( reg_int_temp, pass, swizzle_all_x );
								writeop.sourceb.packed = 0;
								outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );						
								justcopy = false;
								break; 
							}						
						}
					}
					if ( flags & unpack_tex_writemask ) {
						if ( op->dest.writemask != destwrite_xyzw ) { 
							Opcode writeop;					
							writeop.op = op_tex;
							writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); // full writemask 
							writeop.sourcea = op->sourcea;
							writeop.sampler = op->sampler;							
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );							
							writeop.op = op_mov;
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );						
							justcopy = false;
							break;
						}
					}
					if ( flags & unpack_tex_nosrcswizzle ) {
						if ( op->sourcea.swizzle != swizzle_identity ) { 
							Opcode writeop;					
							writeop.op = op_mov;
							writeop.dest.Set ( reg_int_temp, pass, destwrite_xyzw ); // full writemask 
							writeop.sourcea = op->sourcea;
							writeop.sourceb.packed = 0;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );							
							writeop.op = op_tex;
							writeop.dest = op->dest;
							writeop.sourcea.Set ( reg_int_temp, pass, swizzle_identity );
							writeop.sampler = op->sampler;
							outbuf.AppendArray ( (const char*)writeop.rawbytes, sizeof(Opcode) );						
							justcopy = false;
							break;
						}
					}
					break;
				default:
					break; 
			}
		}
		if ( justcopy ) outbuf.AppendArray ( bytecode+readpos, sizeof(Opcode) );
		readpos += sizeof(Opcode); 
	}
	
	PLAYERASSERT ( Validate ( outbuf.mem, outbuf.length, true ) );
	if ( bytecodelen != outbuf.length || memcmp ( outbuf.mem, bytecode, bytecodelen )!=0 ) {
		if ( !Validate ( outbuf.mem, outbuf.length, true ) ) {
			// this should not really happen. but rather keep the extra validate than run into trouble later
			*newbytecode = 0;
			*newbytecodelen = 0; 
			return false; 
		}
		// we have to run the same unpack again: some unpacks might have done something that requires
		// even more unpack! this process must converge - make sure about that in the unpack itself
		char *evennewerbytecode = 0; 
		size_t evennewerbytecodelen = 0;
		if ( !Unpack ( outbuf.mem, outbuf.length, flags, &evennewerbytecode, &evennewerbytecodelen, pass+1 ) ) {
			delete [] evennewerbytecode; 
			return false; 
		}
		if ( evennewerbytecodelen && evennewerbytecode ) {
			*newbytecode = evennewerbytecode;
			*newbytecodelen = evennewerbytecodelen;
		} else {			
			*newbytecode = new char[outbuf.length];
			memcpy ( *newbytecode, outbuf.mem, outbuf.length );
			*newbytecodelen = outbuf.length;
			#if defined(INCLUDE_AGAL_ASSEMBLER) && defined(_DEBUG)
			//FlashString t; 
			//Disassemble ( outbuf.mem, outbuf.length, &t );
			//PLAYER_C_OUTPUT (( "UNPACKED AGAL:\n" ));
			//PlayerOutLongString ( t.CStr() );							
			#endif
		}
	} else {
		*newbytecode = 0;
		*newbytecodelen = 0; 
	}

	return true; 
}





#ifdef INCLUDE_AGAL_ASSEMBLER

/// Assemble -------------------------------------------------------------------------------------------

static inline bool AssembleIsWhitespace ( char x ) {
	switch (x) {
		default: return false; 
		case ' ': case '\n': case '\t': case ';': return true; 
	}
}

static void AssemblePrintToken ( const char *source, unsigned int offset, unsigned int len ) {
	for ( unsigned int i=0; i<len; i++ ) putchar ( source[offset+i] );
}

static void AssembleGetLine ( const char *source, unsigned int &offset, unsigned int &len, unsigned int &linenum, int &errorcode ) {
	len = 0;
	if ( !source ) { errorcode=AGAL::asm_internal; return; }
	// front ws and comments
	skipagain: 
	while ( AssembleIsWhitespace(source[offset]) ) {
		if ( source[offset]=='\n' ) linenum++; 
		offset++; if ( !source[offset] ) return; 
	}
	// skip comments
	if ( source[offset]=='/' && source[offset+1]=='/' ) {
		while ( source[offset]!='\n' && source[offset]!=0 ) offset++; 	
		goto skipagain; 
	}
	// find end
	int desto = 0; 
	while ( source[offset+len] && source[offset+len]!='\n' ) {
		if ( source[offset+len]=='/' && source[offset+len+1]=='/' ) break; 
		len++; 		
	}	
}

static void AssembleGetToken ( const char *source, unsigned int &offset, unsigned int &len, unsigned int &tokenoffset, unsigned int &tokenlen, int &errorcode ) {
	tokenlen = 0; 
	while ( AssembleIsWhitespace(source[offset]) || source[offset]==',' || len==0 ) {
		if ( !source[offset] || len==0 ) { errorcode=AGAL::asm_notoken; return; };		
		offset++; len--;
	}
	tokenoffset = offset;
	int brackets = 0; 
	while ( !( (brackets==0 && (AssembleIsWhitespace(source[offset]) || source[offset]==',')) || source[offset]==0 || len==0) ) {
		if ( source[offset]=='<' || source[offset]=='(' || source[offset]=='[' ) brackets++; // crappy
		if ( source[offset]=='>' || source[offset]==')' || source[offset]==']' ) brackets--; 
		if ( brackets<0 ) { errorcode=AGAL::asm_mismatchedbrackets; return; };			
		offset++; len--; tokenlen++; 
	}
	if ( brackets!=0 ) errorcode=AGAL::asm_mismatchedbrackets; 			
}

static void AssembleGetSwizzle ( const char *source, unsigned int &offset, unsigned int &len, uint8_t &swizzle, int &errorcode ) {	
	if ( !len ) { swizzle = 0xe4; return; }
	if ( len < 2 || len > 5 || source[offset]!='.' ) { errorcode=asm_badswizzle; return; }
	swizzle = 0;
	int swi = 0; 
	while ( len > 1 ) {
		len--; offset++; 
		switch ( source[offset] ) {
			case 'x': case 'r': swizzle |= 0<<swi; break;
			case 'y': case 'g': swizzle |= 1<<swi; break;
			case 'z': case 'b': swizzle |= 2<<swi; break;
			case 'w': case 'a': swizzle |= 3<<swi; break;
			default: errorcode=asm_badswizzle; return;
		}
		swi+=2; 
	}
	// replicate last
	for ( int i=swi; i<8; i+=2 ) {
		swizzle |= ((swizzle>>(swi-2))&3)<<i; 
	}
}

static void AssembleGetWriteMask ( const char *source, unsigned int &offset, unsigned int &len, uint8_t &mask, int &errorcode ) {	
	if ( !len ) { mask = 0xf; return; }
	if ( len < 2 || source[offset]!='.' ) { errorcode=asm_badmask; return; }
	mask = 0; 
	while ( len > 1 ) {
		len--; offset++; 
		switch ( source[offset] ) {
			case 'x': case 'r': mask |= 1; break;
			case 'y': case 'g': mask |= 2; break;
			case 'z': case 'b': mask |= 4; break;
			case 'w': case 'a': mask |= 8; break;
			default: errorcode=asm_badmask; return;
		}
	}
}

static int AssembleGetNumber ( const char *source, unsigned int &offset, unsigned int &len ) {
	int r = 0; 	
	while ( source[offset]>='0' && source[offset]<='9' && len ) {
		r = r*10 + (source[offset]-'0');
		len--; offset++; 
	}
	return r; 
}

static void AssembleGetRegister ( const char *source, unsigned int &offset, unsigned int &len, uint8_t &regtype, uint16_t &regnum, int &errorcode ) {
	if ( len < 2 ) {
		errorcode = asm_badregname; return; }
	// Special case for output registers: 
	if (source[offset] == 'o' && (source[offset+1] == 'p' || source[offset+1] == 'c')) {
		// output register
		regtype = 3;
		regnum = 0;
		offset += 2;
		len -= 2;
		return;
	}
	
	// Other registers:
	//actois	
	char tag = source[offset];
	if ( !(tag=='v' || tag=='f') ) {
		errorcode = asm_badregname; return; }
	offset++; len--; 
	switch ( source[offset] ) {
		case 'a': regtype=0; break;
		case 'c': regtype=1; break;
		case 't': regtype=2; break;
		//case 'o': regtype=3; break;
		//case 'i': regtype=4; break;
		case 's': regtype=5; break;
		default: 
			if (tag == 'v') {
				// variable register
				regtype = 4;
				offset--; len++;
			} else {
				errorcode = asm_badregname;
				return;
			}
	}
	offset++; len--; 
	if (len == 0) { 
		errorcode = asm_badregname;
		return; }
	regnum = AssembleGetNumber(source,offset,len);
}

static void AssembleGetSourceReg ( uint8_t flags, AGAL::Source &dest, const char* source, unsigned int &offset, unsigned int &len, int &errorcode ) {
	dest.packed = 0; 
	if ( !flags ) return;
	unsigned int tokenoffset, tokenlen; 
	AssembleGetToken ( source, offset, len, tokenoffset, tokenlen, errorcode );		
	if ( errorcode!=asm_noerror ) return; 
	if ( tokenlen<2 ) { errorcode=asm_notoken; return; } 		
	#ifdef ASSEMBLER_VERBOSE
	fprintf (stderr,  "  SRC:" ); AssemblePrintToken ( source, tokenoffset, tokenlen ); fprintf (stderr,  "\n" );	
	#endif
	AssembleGetRegister ( source, tokenoffset, tokenlen, dest.regtype, dest.regnum, errorcode );
	if ( tokenlen ) {
		if ( source[tokenoffset]=='[' ) {
			#ifdef ASSEMBLER_VERBOSE
			fprintf (stderr,  "    INDIRECT\n" );
			#endif
			errorcode=asm_noerror; 
			if ( flags & AGAL::fs_noindirect ) {
				errorcode=asm_noindirect; 
				return; 
			}
			tokenoffset++; tokenlen--; 
			// find end
			unsigned int subtokenoffset = tokenoffset; 
			unsigned int subtokenlen = 0; 			
			unsigned int subtokenlen2 = 0;
			while ( source[subtokenoffset+subtokenlen2]!=']' ) {
				if ( source[subtokenoffset+subtokenlen2]=='+' || source[subtokenoffset+subtokenlen2]=='-' ) {
					subtokenlen = subtokenlen2;
				}
				if ( subtokenlen2 >= tokenlen ) {
					errorcode=asm_mismatchedbrackets; 
					return; 
				}
				subtokenlen2++; 
			}					
			if ( !subtokenlen ) subtokenlen = subtokenlen2;
			tokenoffset+=subtokenlen2+1;
			tokenlen-=subtokenlen2+1; 							
			subtokenlen2 -= subtokenlen;
			AGAL::Source tempind;  
			AssembleGetSourceReg ( AGAL::fs_noindirect, tempind, source, subtokenoffset, subtokenlen, errorcode );
			if ( errorcode!=asm_noerror ) return; 
			dest.indirectflag = 1<<7;
			dest.indexselect = tempind.swizzle&3; 			
			dest.indextype = tempind.regtype;
			dest.regnum = tempind.regnum;
			dest.indexoffset = 0; 
			if ( source[subtokenoffset]=='+' ) {
				subtokenoffset++; subtokenlen--; 
				dest.indexoffset = AssembleGetNumber ( source, subtokenoffset, subtokenlen2 ); 
			} else if ( source[subtokenoffset]=='-' ) {
				subtokenoffset++; subtokenlen--; 
				dest.indexoffset = -AssembleGetNumber ( source, subtokenoffset, subtokenlen2 ); 
			}
			#ifdef ASSEMBLER_VERBOSE
			fprintf (stderr,  "    OFFSET: %i\n", dest.indexoffset ); 
			#endif
			if ( errorcode!=asm_noerror ) return; 
		} 
	}
	if ( errorcode!=asm_noerror ) return; 
	AssembleGetSwizzle ( source, tokenoffset, tokenlen, dest.swizzle, errorcode );
	if ( errorcode!=asm_noerror ) return; 
	#ifdef ASSEMBLER_VERBOSE
	fprintf (stderr,  "    SWIZZLE: 0x%x\n", dest.swizzle ); 		
	#endif
}

void AssembleSetTexFlag ( const char *source, unsigned int offset, unsigned int len, AGAL::Sampler &dest, int &errorcode ) {
	if ( strcmp ( source, "2d" ) ) {
		dest.dim = AGAL::dim_2d;
	} else if ( strcmp ( source, "3d" ) ) {
		dest.dim = AGAL::dim_3d;
	} else if ( strcmp ( source, "cube" ) ) {
		dest.dim = AGAL::dim_cube;
	} else if ( strcmp ( source, "rect" ) ) {
		dest.dim = AGAL::dim_rect;
	} else if ( strcmp ( source, "clamp" ) ) {
		dest.wrap = AGAL::wrap_clamp;
	} else if ( strcmp ( source, "repeat" ) ) {
		dest.wrap = AGAL::wrap_repeat;
	} else if ( strcmp ( source, "nearest" ) ) {
		dest.filter = AGAL::filter_nearest;
	} else if ( strcmp ( source, "linear" ) ) {
		dest.filter = AGAL::filter_linear;
	} else if ( strcmp ( source, "anisotropic" ) ) {
		dest.filter = AGAL::filter_anisotropic;
	} else if ( strcmp ( source, "miplinear" ) ) {
		dest.mipmap = AGAL::mipmap_linear;
	} else if ( strcmp ( source, "nomip" ) ) {
		dest.mipmap = AGAL::mipmap_none;
	} else if ( strcmp ( source, "mipnearest" ) ) {
		dest.mipmap = AGAL::mipmap_nearest;
	} else if ( strcmp ( source, "centroid" ) ) {	
		dest.special |= AGAL::special_centroid;
	} else {
		errorcode = asm_badtexflag;
	}
}

int AGAL::Assemble ( const char *source, uint8_t shadertype, char **bytecode, size_t *bytecodelen ) {
	PArray<char> dest; 
	int errorcode = asm_noerror; 	
	unsigned int offset = 0, len = 0; 	
	unsigned int tokenoffset = 0, tokenlen = 0; 	
	unsigned int linenum = 0;
	while ( errorcode==asm_noerror ) {
		AssembleGetLine ( source, offset, len, linenum, errorcode );		
		if ( len==0 || errorcode!=asm_noerror ) break; // empty line is eof or too short	
		if ( len<9 ) { errorcode=asm_linetooshort; break; } 

		#ifdef ASSEMBLER_VERBOSE
		fprintf (stderr,  "%i: ",linenum ); AssemblePrintToken ( source, offset, len ); fprintf (stderr,  "\n" );
		#endif
		
		// get opcode 
		AssembleGetToken ( source, offset, len, tokenoffset, tokenlen, errorcode );		
		if ( errorcode!=asm_noerror ) break; 
		if ( tokenlen!=3 ) { errorcode=asm_notoken; break; } 		

		Opcode op; op.op = 0xffff; 
		for ( int i=0; i<opmap_max; i++ ) {
			if ( source[tokenoffset]==opmap[i].mnemonic[0] && source[tokenoffset+1]==opmap[i].mnemonic[1] && source[tokenoffset+2]==opmap[i].mnemonic[2] ) {
				op.op = i; 				
				#ifdef ASSEMBLER_VERBOSE
				fprintf (stderr,  "  OP=%s (0x%x)\n", opmap[i].mnemonic, i ); 
				#endif
				break; 
			}
		}
		if ( op.op==0xffff ) {
			errorcode = asm_invalidopcode; 
			break;
		}
		// get dest arg
		if ( !(opmap[op.op].flags & f_nodest) ) {
			AssembleGetToken ( source, offset, len, tokenoffset, tokenlen, errorcode );		
			if ( errorcode!=asm_noerror ) break; 
			if ( tokenlen<2 ) { errorcode=asm_notoken; break; } 	
			#ifdef ASSEMBLER_VERBOSE
			fprintf (stderr,  "  DEST:" ); AssemblePrintToken ( source, tokenoffset, tokenlen ); fprintf (stderr,  "\n" );
			#endif
			AssembleGetRegister ( source, tokenoffset, tokenlen, op.dest.regtype, op.dest.regnum, errorcode );
			if ( errorcode!=asm_noerror ) break; 
			AssembleGetWriteMask ( source, tokenoffset, tokenlen, op.dest.writemask, errorcode );
			if ( errorcode!=asm_noerror ) break; 
			#ifdef ASSEMBLER_VERBOSE
			fprintf (stderr,  "    MASK: 0x%x\n", op.dest.writemask ); 
			#endif
		} else {
			op.dest.packed = 0; 
		}

		AssembleGetSourceReg ( opmap[op.op].srcaflags, op.sourcea, source, offset, len, errorcode ); 
		AssembleGetSourceReg ( opmap[op.op].srcbflags, op.sourceb, source, offset, len, errorcode ); 

		if ( op.op == op_tex ) {
			AssembleGetToken ( source, offset, len, tokenoffset, tokenlen, errorcode );		
			if ( errorcode!=asm_noerror ) break; 
			if ( tokenlen<2 ) { errorcode=asm_notoken; break; } 	
			#ifdef ASSEMBLER_VERBOSE
			fprintf (stderr,  "  TEXFLAGS:" ); AssemblePrintToken ( source, tokenoffset, tokenlen ); fprintf (stderr,  "\n" );
			#endif
			if ( source[tokenoffset] != '<' || source[tokenoffset+tokenlen-1] != '>' ) {
				errorcode=asm_mismatchedbrackets;
				break; 
			}
			tokenoffset++; 
			tokenlen-=2; 
			for ( ;; ) {
				unsigned int subtokenoffset = 0, subtokenlen = 0; 
				AssembleGetToken ( source, tokenoffset, tokenlen, subtokenoffset, subtokenlen, errorcode );		
				if ( subtokenlen ) {
					#ifdef ASSEMBLER_VERBOSE
					fprintf (stderr,  "    FLAG:" ); AssemblePrintToken ( source, subtokenoffset, subtokenlen ); fprintf (stderr,  "\n" );
					#endif
					AssembleSetTexFlag ( source, subtokenoffset, subtokenlen, op.sampler, errorcode );
					if ( errorcode!=asm_noerror ) break; 
				} else {
					errorcode = asm_noerror; 
					break;
				}
			}
			if ( errorcode!=asm_noerror ) break; 
		}

		if ( errorcode==asm_noerror ) {
			AssembleGetToken ( source, offset, len, tokenoffset, tokenlen, errorcode );		
			if ( tokenlen ) {
				errorcode = asm_extratoken; 
				break; 
			}
			errorcode=asm_noerror;
		}		
		dest.AppendArray ( (const char*)&op, sizeof(AGAL::Opcode) ); 
		offset+=len; 
	}

	if ( errorcode!=asm_noerror ) {
		*bytecode = 0; 
		*bytecodelen = 0;
	} else {
		*bytecodelen = dest.length + sizeof(AGAL::Header);
		*bytecode = new char[*bytecodelen];
		((AGAL::Header*)*bytecode)->magic = AGAL::magic;		
		((AGAL::Header*)*bytecode)->shadertypeopcode = AGAL::shadertype_opcode;
		((AGAL::Header*)*bytecode)->shadertype = shadertype;
		((AGAL::Header*)*bytecode)->version = AGAL::current_version;
		memcpy ( (*bytecode)+sizeof(AGAL::Header), dest.mem, dest.length ); 
	}

	#ifdef ASSEMBLER_VERBOSE
	fprintf (stderr,  "DONE: %i bytes (%d instructions), code=%s\n", *bytecodelen, (*bytecodelen - 7) / 24, asm_errorstrings[errorcode] );
	#endif

	return errorcode; 	
}

/// Disassemble -------------------------------------------------------------------------------------------

static const char regtypes[] = "actois__________-CT#--A_________"; 
static const char vertexCompstr[] = "xyzw";
static const char fragmentCompstr[] = "rgba"; 
static const char *dimstr[] = { "2d","cube","3d","rect" };
static const char *filterstr[] = { "nearest","linear","anisotropic" };
static const char *mipstr[] = { "nomip", "mipnearest", "miplinear" };
static const char *wrapstr[] = { "clamp", "repeat" };
static const char *specialstr[] = { "centroid", "single", "depth", 0 };

static void DisassembleSourceReg ( AGAL::Source s, uint8_t flags, char tag, FlashString *dest ) {	
	const char* compstr = tag == 'f' ? fragmentCompstr : vertexCompstr;
	
	if ( s.regtype==AGAL::reg_sampler ) {
		AGAL::Sampler sam; 
		sam.packed = s.packed; 
		dest->AppendFormat ( "%cs%d <%s, %s, %s, %s", tag, sam.regnum, dimstr[sam.dim], filterstr[sam.filter], mipstr[sam.mipmap], wrapstr[sam.wrap] ); 		
		if ( sam.lodbias ) dest->AppendFormat ( ", %f", float(sam.lodbias)*(1.0f/8.0f) );
		for ( unsigned int i=0; specialstr[i]; i++ )
			if ( (sam.special>>i)&1 ) dest->AppendFormat ( ", %s", specialstr[i] ); 
		dest->AppendChar ('>');
	} else {
		if (regtypes[s.regtype] == 'i') {
			if ( s.indirectflag )
				if (regtypes[s.indextype] == 'i')
					dest->AppendFormat ( "v[v%d.%c%+d]", s.regnum, compstr[s.indexselect], s.indexoffset );
				else
					dest->AppendFormat ( "v[%c%c%d.%c%+d]", tag, regtypes[s.indextype], s.regnum, compstr[s.indexselect], s.indexoffset );
			else 
				dest->AppendFormat ( "v%d", s.regnum ); 
		} else {
		if ( s.indirectflag ) 
				if (regtypes[s.indextype] == 'i')
					dest->AppendFormat ( "%c%c[v%d.%c%+d]", tag, regtypes[s.regtype], s.regnum, compstr[s.indexselect], s.indexoffset );
				else
			dest->AppendFormat ( "%c%c[%c%c%d.%c%+d]", tag, regtypes[s.regtype], tag, regtypes[s.indextype], s.regnum, compstr[s.indexselect], s.indexoffset ); 						
		else 
			dest->AppendFormat ( "%c%c%d", tag, regtypes[s.regtype], s.regnum ); 				
		}
		if ( s.swizzle == AGAL::swizzle_identity && (flags&fs_vector) ) return; 
		dest->AppendChar ( '.' ); 
		dest->AppendChar ( compstr[s.swizzle&3] );
		dest->AppendChar ( compstr[(s.swizzle>>2)&3] );
		dest->AppendChar ( compstr[(s.swizzle>>4)&3] );
		dest->AppendChar ( compstr[(s.swizzle>>6)&3] );
	}
}

void AGAL::Disassemble ( const char *bytecode, size_t bytecodelen, FlashString *dest ) {	
	//PLAYERASSERT ( Validate ( bytecode, bytecodelen, true ) );
	const AGAL::Header *aheader = (const AGAL::Header *)bytecode; 	
	char regtag; 	
	if ( aheader->shadertype == shadertype_fragment ) {
		dest->AppendString ( "//fragment" );
		regtag = 'f'; 
	} else {
		dest->AppendString ( "//vertex" );
		regtag = 'v'; 
	}
	dest->AppendFormat ( " program (agal %d)\n", aheader->version );
	unsigned int indent = 0;	
	unsigned int readpos = sizeof(Header); 		
	if ( bytecodelen < sizeof(Opcode) ) return; 
	while ( readpos <= bytecodelen-sizeof(Opcode) ) {
		int linestart = dest->length; 
		const Opcode *op = (const Opcode*)(bytecode+readpos);
		if ( opmap[op->op].flags & f_decnest ) indent--; 
		for ( unsigned int i=0; i<indent; i++ ) dest->AppendChar ( 9 ); 

		dest->AppendString ( opmap[op->op].mnemonic );
		dest->AppendChar ( ' ' ); 
		if ( !(opmap[op->op].flags & f_nodest) ) {			
			switch (regtypes[op->dest.regtype]) {
				case 'i':
					dest->AppendFormat ( "v%d", op->dest.regnum ); 
					break;
				case 'o':
					if (regtag == 'v' && op->dest.regnum == 0) {
						dest->AppendString ( "op" ); 
					} else if (regtag == 'f' && op->dest.regnum == 0) {
						dest->AppendString ( "oc" ); 
					} else {
						// Not a valid output register
						PLAYERASSERT(false);
					}
					break;
				default:
			dest->AppendFormat ( "%c%c%d", regtag, regtypes[op->dest.regtype], op->dest.regnum ); 
			}
			if ( op->dest.writemask != destwrite_xyzw ) {
				dest->AppendChar ( '.' ); 
				if ( op->dest.writemask&1 ) regtag == 'f' ? dest->AppendChar ('r') : dest->AppendChar ('x'); 
				if ( op->dest.writemask&2 ) regtag == 'f' ? dest->AppendChar ('g') : dest->AppendChar ('y'); 
				if ( op->dest.writemask&4 ) regtag == 'f' ? dest->AppendChar ('b') : dest->AppendChar ('z'); 
				if ( op->dest.writemask&8 ) regtag == 'f' ? dest->AppendChar ('a') : dest->AppendChar ('w'); 
			}
			if ( opmap[op->op].srcaflags ) dest->AppendString ( ", " ); 			
		} 
		if ( opmap[op->op].srcaflags ) {							
			DisassembleSourceReg ( op->sourcea, opmap[op->op].srcaflags, regtag, dest ); 		
			if ( opmap[op->op].srcbflags ) {
				dest->AppendString ( ", " ); 
				DisassembleSourceReg ( op->sourceb, opmap[op->op].srcbflags, regtag, dest ); 
			}		
		}
		dest->AppendChar ( '\n' );
		if ( opmap[op->op].flags & f_incnest ) indent++; 
		readpos += sizeof(Opcode); 
	}
	dest->AppendChar ( 0 ); 	
}

#endif






#ifdef INCLUDE_AGAL_DEPENDENCYGRAPH

static void AddLinks ( AGAL::Graph *graph, const AGAL::Source &s, uint8_t sflags, unsigned int token, uint8_t limitmask ) {	
	if ( !sflags ) return; 
	if ( s.indirectflag ) {
		// turn into only the relative, constants can be ignored
		PLAYERASSERT ( s.regtype == AGAL::reg_const );
		AGAL::Source s2;
		s2.packed = 0;
		s2.swizzle = AGAL::ReplicateSwizzle(s.indexselect);
		s2.regtype = s.indextype;
		s2.regnum = s.regnum; 
		AddLinks ( graph, s2, AGAL::fs_scalar, token, 1<<s.indexselect );
		return; 
	}
	// no more indirect - can discard input sinks
	if ( !(s.regtype == AGAL::reg_temp || s.regtype == AGAL::reg_int_temp || s.regtype == AGAL::reg_int_addr ) )
		return; 
	// matrix expand
	if ( sflags & (fs_matrix3|fs_matrix4) ) {	
		//limitmask = (sflags & fs_matrix3)?0x7:0xf; - no, not done in other steps either
		int n = (sflags & fs_matrix3)?3:4;
		sflags &= ~(fs_matrix3|fs_matrix4);
		sflags |= fs_vector;
		for ( int i=0; i<n; i++ ) {
			AGAL::Source s2;
			s2.packed = s.packed;
			s2.regnum += i; 			
			AddLinks ( graph, s2, sflags, token, 0xf );
		}
		return; 
	}
	// no more matrix opcodes 
	uint8_t readmask = SwizzleToMask ( s.swizzle ); 
	// find the links
	graph->edges.EnsureSpace(4); 	
	int searchback = 0; 
	
	for ( unsigned int i=0; i<4; i++ ) {
		if ( !(limitmask&(1<<i)) ) continue; 		
		unsigned int i2 = GetSwizzledChannel ( s, i );
		// back-find this and add link if needed. link target gets flagged for needing this output
		PLAYERASSERT(token>0); 		
		bool found = false; 
		for ( int t2=token-1; t2>=0; t2-- ) {			
			if ( graph->allnodes.mem[t2].op.dest.regnum == s.regnum &&
				 graph->allnodes.mem[t2].op.dest.regtype == s.regtype &&				 
				 (graph->allnodes.mem[t2].op.dest.writemask & (1<<i2)) ) {
				// found, check if we can extend the previous existing one, otherwise add it! (backwards, last one is most likely) 															
				for ( int j=graph->allnodes.mem[token].inputs.length-1; j>=((int)graph->allnodes.mem[token].inputs.length)-searchback; j-- ) {
					unsigned int ei = graph->allnodes.mem[token].inputs.mem[j];
					if ( graph->edges[ei].srcidx == t2 ) {
						graph->edges[ei].mask |= 1<<i2;
						found = true;
						break; 
					}
				}
				if  ( !found ) {
					graph->edges.mem[graph->edges.length].srcidx = t2;
					graph->edges.mem[graph->edges.length].dstidx = token;
					graph->edges.mem[graph->edges.length].mask = 1<<i2; 			
					graph->allnodes.mem[token].inputs.PushByValue ( graph->edges.length );
					graph->allnodes.mem[t2].outputs.PushByValue ( graph->edges.length );
					graph->edges.length++; 
					found = true;
					searchback++;
				}
				// mark in t2 as needed output
				graph->allnodes.mem[t2].writemask |= 1<<i; 
				break;
			}
		}		
		PLAYERASSERT ( found ); // read without write		
	}
}

static void StartRecMarkInput ( AGAL::Graph *graph, unsigned int token, uint8_t mask ) {
	if ( mask==0 ) return; 	
	if ( graph->allnodes.mem[token].flags & gf_recursing ) {
		PLAYERASSERT(0); 
		return; // working on it? circular? 
	}

	const AGAL::Opcode *op = &graph->allnodes.mem[token].op;
	graph->allnodes.mem[token].flags |= gf_recursing | gf_active; // in the works

	// track only inputs needed for destination
	uint8_t limitinput = (opmap[op->op].flags & f_horiz)?0xf:mask; 			
	AddLinks ( graph, op->sourcea, opmap[op->op].srcaflags, token, limitinput );						
	AddLinks ( graph, op->sourceb, opmap[op->op].srcbflags, token, limitinput );			

	// now follow the new links and recurse on them 
	for ( unsigned int i=0; i<graph->allnodes.mem[token].inputs.length; i++ ) {
		unsigned int ei = graph->allnodes.mem[token].inputs.mem[i];
		StartRecMarkInput ( graph, graph->edges[ei].srcidx, graph->edges[ei].mask ); 
	}

	graph->allnodes.mem[token].flags &= ~gf_recursing; // visited and alive
}

AGAL::Graph * AGAL::CreateDependencyGraph ( const char *bytecode, size_t bytecodelen, uint32_t flags ) {	
	PLAYERASSERT ( Validate ( bytecode, bytecodelen, true ) ); 	
	
	unsigned int ntokens = (bytecodelen-sizeof(Header))/sizeof(Opcode);	
	if ( !ntokens ) return 0; 
	const Header *aheader = (const AGAL::Header *)bytecode; 	
	const Opcode *ops = (const Opcode*)(bytecode+sizeof(Header));
	
	// init graph
	Graph *graph = new Graph; 

	graph->sink_kill_count = 0;
	memset ( graph->sink_varying_written, 0, sizeof ( graph->sink_varying_written ) );
	memset ( graph->sink_output_written, 0, sizeof ( graph->sink_output_written ) );
	
	graph->edges.EnsureSpace(ntokens*4);

	graph->allnodes.EnsureSpace(ntokens);
	graph->allnodes.length = ntokens; 
	memset ( graph->allnodes.mem, 0, sizeof(Node)*ntokens );
	for ( unsigned int i=0; i<ntokens; i++ ) {		
		graph->allnodes.mem[i].op = ops[i]; 
		graph->allnodes.mem[i].idx = (uint16_t)i;
	}

	// search backwards
	for ( int i=ntokens-1; i>=0; i-- ) {							
		uint8_t mask = 0; 
		if ( ops[i].op == op_kil ) {			
			mask = 1; // x only
			graph->sink_kill_count++; 
		} else {
			PLAYERASSERT (( (opmap[ops[i].op].flags & f_nodest) == 0 )); 
			switch ( ops[i].dest.regtype ) { 
				case reg_output: // sink what is written to 
					mask = ops[i].dest.writemask & ~ graph->sink_output_written[ops[i].dest.regnum];
					graph->sink_output_written[ops[i].dest.regnum] |= mask; 
					break; 
				case reg_varying: // sink what is written to 
					PLAYERASSERT ( aheader->shadertype == AGAL::shadertype_vertex );
					mask = ops[i].dest.writemask & ~ graph->sink_varying_written[ops[i].dest.regnum];
					graph->sink_varying_written[ops[i].dest.regnum] |= mask; 					
					break;
			}
		}	
		if ( mask ) {
			// if mask is 0, this instruction is not a starting point for the graph
			graph->sinks.PushByValue ( i );
			graph->allnodes.mem[i].writemask = mask; 

			StartRecMarkInput ( graph, i, mask );
			// PLAYER_C_OUTPUT (( "begin optimizer trail at token %d for %s%s%s%s\n", tokennum, (readmask&1)?"x":"", (readmask&1)?"y":"", (readmask&1)?"z":"", (readmask&1)?"w":"" )); 
			// mark opcode, then start backtracking
			//StartRecMarkInput ( op, readmask, bytecode, readpos, tokennum, opoutputused );
		}
	}

	return graph;	
}


void RecPrintNodes ( AGAL::Graph *graph, int idx, int level ) {
	for ( int i=0; i<level; i++ ) fprintf (stderr,  "  " );
	fprintf (stderr,  "%i: %s (%i out, %i in)\n", idx+1, AGAL::opmap[graph->allnodes.mem[idx].op.op].mnemonic, 
		graph->allnodes.mem[idx].outputs.length, graph->allnodes.mem[idx].inputs.length ); 
	for ( unsigned int i=0; i<graph->allnodes.mem[idx].inputs.length; i++ ) {
		int ei = graph->allnodes.mem[idx].inputs.mem[i];
		PLAYERASSERT ( graph->edges.mem[ei].dstidx == idx ); 
		RecPrintNodes ( graph, graph->edges.mem[ei].srcidx, level+1 );
	}

}

void AGAL::PrintGraph ( AGAL::Graph *graph ) {
	if ( !graph ) return; 
	for ( unsigned int i=0; i<graph->sinks.length; i++ ) {
		RecPrintNodes ( graph, graph->sinks.mem[i], 0 );
	}
	int ndead = 0;
	for ( unsigned int i=0; i<graph->allnodes.length; i++ ) {
		if ( graph->allnodes.mem[i].flags == 0 ) ndead++; 
	}
	if ( ndead ) fprintf (stderr,  "%i dead instructions\n", ndead );
}


#endif



#ifdef INCLUDE_AGAL_SIMPLEDEPENDENCY


// just find the input of this, and mark it 
static int16_t MarkInputSimple ( AGAL::SimpleDependencies *sd, const AGAL::Source &s, uint8_t sflags, unsigned int token, uint8_t limitmask ) {	
	if ( !sflags ) return sdsrc_constant; // easier to check than another value
	if ( s.indirectflag ) {
		// turn into only the relative, constants can be ignored
		PLAYERASSERT ( s.regtype == AGAL::reg_const );
		AGAL::Source s2;
		s2.packed = 0;
		s2.swizzle = AGAL::ReplicateSwizzle(s.indexselect);
		s2.regtype = s.indextype;
		s2.regnum = s.regnum; 
		//sd[token].flags |= sdf_relative; 
		MarkInputSimple ( sd, s2, AGAL::fs_scalar, token, 1<<s.indexselect );		
		// could return value, but harder to unpack
		return sdsrc_complex;
	}
	// no more indirect - can discard input sinks
	if ( !(s.regtype == AGAL::reg_temp || s.regtype == AGAL::reg_int_temp || s.regtype == AGAL::reg_int_addr ) )
		return sdsrc_constant; // non dependent input
	// matrix expand
	if ( sflags & (fs_matrix3|fs_matrix4) ) {	
		//limitmask = (sflags & fs_matrix3)?0x7:0xf; - no, not done in other steps either
		int n = (sflags & fs_matrix3)?3:4;
		sflags &= ~(fs_matrix3|fs_matrix4);
		sflags |= fs_vector;
		int16_t rval = sdsrc_constant;
		for ( int i=0; i<n; i++ ) {
			AGAL::Source s2;
			s2.packed = s.packed;
			s2.regnum += i; 			
			int16_t newval = MarkInputSimple ( sd, s2, sflags, token, 0xf );
			if ( newval != rval ) 
				rval = sdsrc_complex; 
		}				
		return rval;  // complex input dependency
	}
	// no more matrix opcodes 

	// those need to be written to in a single op to qualify for simple. 
	// all of those need to be acounted for 
	uint8_t readmask = SwizzleToMaskWithLimit ( s.swizzle, limitmask ); 
	uint8_t fullreadmask = readmask;

	int16_t rval = sdsrc_unknown; 
	for ( int t2=token-1; t2>=0; t2-- ) {		
		if ( sd[t2].op.dest.regnum == s.regnum &&
			 sd[t2].op.dest.regtype == s.regtype ) {
			uint8_t mask = sd[t2].op.dest.writemask & readmask;
			if ( mask ) {
				sd[t2].numreaders ++; 
				sd[t2].usedwritemask |= mask; 
				if ( (sd[t2].op.dest.writemask & fullreadmask) != fullreadmask ) 
					sd[t2].flags |= sdf_incompleteread;
				readmask &= ~mask; // clear bits			
				if ( !readmask ) {
					if ( rval == sdsrc_unknown ) rval = (int16_t)t2;
					break; 
				} else {
					rval = sdsrc_complex; 			
				}
			}
		}
	}
	PLAYERASSERT (( readmask == 0 && rval!=sdsrc_unknown )); // all acounted for 
	return rval; 
}


AGAL::SimpleDependencies * AGAL::CreateSimpleDependencies ( const char *bytecode, size_t bytecodelen ) {
	PLAYERASSERT ( Validate ( bytecode, bytecodelen, true ) ); 	
	
	int ntokens = (bytecodelen-sizeof(Header))/sizeof(Opcode);	
	if ( !ntokens ) return 0; 
	const Header *aheader = (const AGAL::Header *)bytecode; 	
	const Opcode *ops = (const Opcode*)(bytecode+sizeof(Header));

	AGAL::SimpleDependencies *sd = new AGAL::SimpleDependencies[ntokens]; 
	memset ( sd, 0, sizeof ( SimpleDependencies ) * ntokens ); 

	uint8_t sink_varying_written[AGAL::max_varying_count] = {0};
	uint8_t sink_output_written[AGAL::max_fragment_outputs] = {0};
	unsigned int sink_kill_count = 0; 

	for ( int i=0; i<ntokens; i++ ) {
		sd[i].op = ops[i]; 
		sd[i].srca = sdsrc_unused; 
		sd[i].srcb = sdsrc_unused; 
	}

	for ( int i=ntokens-1; i>=0; i-- ) {							
		uint8_t mask = 0; 
		if ( ops[i].op == op_kil ) {			
			mask = 1; // x only
			sink_kill_count++; 
			sd[i].flags = sdf_sink;	
		} else {
			PLAYERASSERT (( (opmap[ops[i].op].flags & f_nodest) == 0 )); 
			switch ( ops[i].dest.regtype ) { 
				case reg_output: // sink what is written to 
					mask = ops[i].dest.writemask & ~ sink_output_written[ops[i].dest.regnum];
					sink_output_written[ops[i].dest.regnum] |= mask; 
					sd[i].flags = sdf_sink;	
					break; 
				case reg_varying: // sink what is written to 
					PLAYERASSERT ( aheader->shadertype == AGAL::shadertype_vertex );
					mask = ops[i].dest.writemask & ~ sink_varying_written[ops[i].dest.regnum];
					sink_varying_written[ops[i].dest.regnum] |= mask; 					
					sd[i].flags = sdf_sink;	
					break;
				default:
					if ( sd[i].numreaders ) 
						mask = ops[i].dest.writemask & sd[i].usedwritemask;			 					
					break; 
			}
		}	
		if ( mask ) {									
			sd[i].usedwritemask = mask; 
			uint8_t limitinput = (opmap[ops[i].op].flags & f_horiz)?0xf:mask; 			
			sd[i].srca = MarkInputSimple ( sd, ops[i].sourcea, opmap[ops[i].op].srcaflags, i, limitinput );						
			sd[i].srcb = MarkInputSimple ( sd, ops[i].sourceb, opmap[ops[i].op].srcbflags, i, limitinput );									
		}
	}
	return sd; 
}


#endif


#ifdef INCLUDE_AGAL_INLINE

static bool IsSameRead ( const AGAL::Source &s, const AGAL::Source &s2 ) {
	// ignore swizzle 
	if ( s.regtype == s2.regtype && s.regnum == s2.regnum ) return true; 	
	if ( s2.indirectflag && s2.indextype == s.regtype && s2.regnum == s.regnum ) return true; 	
	// todo, matrix! 
	return false; 
}

static bool IsMatrixOp ( const Opcode &op ) {
	return ( ( (opmap[op.op].srcaflags | opmap[op.op].srcbflags) & (fs_matrix3 | fs_matrix4) ) != 0 );	
}

static int CanInline ( const AGAL::Source &s, const Opcode *ops, unsigned int opidx, unsigned int ntokens ) {
	if ( !(s.regtype == reg_temp || s.regtype == reg_int_temp) ) return -1; 
	if ( s.indirectflag ) return -1; 
	// scan down to see that the temp is never read before eof or being _fully_ written.	
	// check write - on self
	if ( !(ops[opidx].dest.regtype == s.regtype && ops[opidx].dest.regnum == s.regnum && ops[opidx].dest.writemask == 0xf) ) {
		for ( unsigned int i=opidx+1; i<ntokens; i++ ) {
			if ( IsMatrixOp (ops[i]) ) return -1; 
			// check read
			if ( IsSameRead(s,ops[i].sourcea ) ) return -1; 
			if ( IsSameRead(s,ops[i].sourceb ) ) return -1; 
			// check write - it's safe now! 
			if ( ops[i].dest.regtype == s.regtype && ops[i].dest.regnum == s.regnum && ops[i].dest.writemask == 0xf ) break; 
		}
	}
	// have an opcode that reads only one source reg. scan up, to see that it is _never_ read before being _fully_ written 
	// find that write as potential inline entry
	for ( int i=opidx-1; i>=0; i-- ) {
		// check write 
		if ( ops[i].dest.regtype == s.regtype && ops[i].dest.regnum == s.regnum && ops[i].dest.writemask == 0xf ) return i; 
		// check read
		if ( IsSameRead(s,ops[i].sourcea ) ) return -1; 
		if ( IsSameRead(s,ops[i].sourceb ) ) return -1; 
		if ( IsMatrixOp (ops[i]) ) return -1; 
	}
	return -1; 
}

static bool IsSameWrite ( const AGAL::Destination &d, const AGAL::Source &s ) {
	// ignore swizzle 
	if ( d.regtype == s.regtype && d.regnum == s.regnum ) return true;
	//  !!@ no idea????
	if ( s.indirectflag && s.indextype == d.regtype && s.regnum == d.regnum ) return true; 	
	// todo, matrix! 
	return false; 
}

static bool SrcRegsAreWritten ( const Opcode *ops, unsigned int opidx, unsigned int last) {
	// give our ops[opidx], look at the two source registers.
	// if either one is written after this operation and before 'last', return true
	// if either one is a reg_int_inline, we need to recurse into that instruction and check it

	for ( unsigned int i=opidx+1; i<last; i++ ) {
		if (ops[opidx].sourcea.regtype == reg_int_inline) {
			// recurse back and check
			if (SrcRegsAreWritten(ops, ops[opidx].sourcea.regnum, last))
				return true;
		}
		if (ops[opidx].sourceb.regtype == reg_int_inline) {
			// recurse back and check
			if (SrcRegsAreWritten(ops, ops[opidx].sourceb.regnum, last))
				return true;
		}
		if ( IsSameWrite(ops[i].dest, ops[opidx].sourcea) ) {
			return true;
		}
		if ( IsSameWrite(ops[i].dest, ops[opidx].sourceb) ) {
			return true;
		}
	}

	return false;
}

void AGAL::InlineOperations ( const char *bytecode, size_t bytecodelen, char **newbytecode, size_t *newbytecodelen ) {
	// temp register reads reg_int_temp or reg_temp are converted to reg_int_inline.
	// reg_int_inline regsiter numbers are token numbers to inline from 
	// destination of inlined opcodes are set to reg_int_inline as well with the token number as dest. 
	// dest of reg_int_inline is ignored during code generation. 

	*newbytecode = new char[bytecodelen]; 
	*newbytecodelen = bytecodelen; 
	memcpy ( *newbytecode, bytecode, bytecodelen ); 

	unsigned int ntokens = (bytecodelen-sizeof(Header))/sizeof(Opcode);	
	if ( !ntokens ) return; 	
	Opcode *ops = (Opcode*)((*newbytecode)+sizeof(Header));

	for ( unsigned int i=0; i<ntokens; i++ ) {
		// check every op for inlining seperately, n^2 bad, but might work 		
		if ( ops[i].sourcea.regtype == ops[i].sourceb.regtype && 
			ops[i].sourcea.regnum == ops[i].sourceb.regnum ) 
			continue;  // same register read, can never inline 
		// never inline matrix ops
		if ( IsMatrixOp (ops[i]) ) continue; 
		
		int i_a = CanInline ( ops[i].sourcea, ops, i, ntokens );
		int i_b = CanInline ( ops[i].sourceb, ops, i, ntokens );
		if ( i_a!=-1 && !SrcRegsAreWritten(ops, i_a, i)) {
			ops[i].sourcea.regtype = reg_int_inline; 
			ops[i].sourcea.regnum = i_a; 
			ops[i_a].dest.regtype = reg_int_inline;
			ops[i_a].dest.regnum = i; 
		}
		if ( i_b!=-1 && !SrcRegsAreWritten(ops, i_b, i)) {
			ops[i].sourceb.regtype = reg_int_inline; 
			ops[i].sourceb.regnum = i_b; 
			ops[i_b].dest.regtype = reg_int_inline;
			ops[i_b].dest.regnum = i; 
		}
	}
}

#endif