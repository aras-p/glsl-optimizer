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

#ifndef H_AGAL
#define H_AGAL

#include "MiniFlashString.h" 
#include "MiniFlashTypes.h" 

// documentation, macros and constants for the AGAL (Adobe Graphics Assembly Language) bytecode
// this file is not a specific implementation, it just defines all the numeric bytecode values
// might need tweaking for endianess and bitfield implementations on different compilers

#define INCLUDE_AGAL_ASSEMBLER // debug define, should not ship (or add for agal runtime creation when Assemble is implemented) 
#define INCLUDE_AGAL_DEPENDENCYGRAPH 
#define INCLUDE_AGAL_INLINE
#define ASSEMBLER_VERBOSE

#define INCLUDE_AGAL_SIMPLEDEPENDENCY

#ifndef M_PI
	#define M_PI 3.1415926535897
#endif

// how to commonly use this for creating a backend (handle all errors!): 
//
// Validate ( vertexprogram ), Validate ( fragmentprogram )
// Describe ( vertexprogram, vertexdesc ), Describe ( fragmentprogram, fragmentdesc )
// ValidateLinkage ( vertexdesc, fragmentdesc )
// optional, unpacked programs are easier to translate:
//   vertexprogram = Unpack ( vertexprogram ) fragmentprogram = Unpack ( fragmentprogram )
//   Validate ( vertexprogram ), Validate ( fragmentprogram )
//   Describe ( vertexprogram, vertexdesc ), Describe ( fragmentprogram, fragmentdesc )
//   ValidateLinkage ( vertexdesc, fragmentdesc )
// create native backend code
// compile native backend code
// link native backend code

namespace AGAL {	

#if defined(_WIN32) || defined(_MAC) || defined(UNIX)
	#pragma pack(push,1)
#else
	#error use the correct packing pragma for your platform
#endif
	

	// first byte in every agal bytecode stream
	static const uint8_t magic = 0xa0; 
	static const uint8_t shadertype_opcode = 0xa1;
	static const uint8_t shadertype_vertex = 0x0; 
	static const uint8_t shadertype_fragment = 0x1; 

	static const uint32_t current_version = 0x1;

	// first 7 bytes header
	struct Header {
		uint8_t magic;				// must be magic value
		uint32_t version;			// version 
		uint8_t shadertypeopcode;	// must be shadertype_opcode
		uint8_t shadertype;			// must be shadertype_vertex or shadertype_fragment
	}; 

	// register types	
	static const uint8_t reg_attrib = 0x0;
	static const uint8_t reg_const = 0x1;
	static const uint8_t reg_temp = 0x2;
	static const uint8_t reg_output = 0x3;
	static const uint8_t reg_varying = 0x4;	
	static const uint8_t reg_sampler = 0x5; // special for sampler, can not be used in generic source

	static const uint8_t reg_int_const = 0x11; // internal const register: only used for unpack
	static const uint8_t reg_int_temp = 0x12; // internal temp register: only used for unpack	
	static const uint8_t reg_int_inline = 0x13; // internal inline temp register 
	static const uint8_t reg_int_addr = 0x16; // internal addresse register: only used for unpack	
	

	// V1 limits
	static const uint16_t max_attrib_count = 8;
	static const uint16_t max_vertex_const_count = 128;
	static const uint16_t max_fragment_const_count = 28; // 28+4 = ps_2_x minimum
	static const uint16_t max_temp_count = 2^32; // 8; // 8+4 = vs_2_x and ps_2_x minimum // todo, remove, split
	static const uint16_t max_vertex_temp_count = max_temp_count; 
	static const uint16_t max_fragment_temp_count = max_temp_count; 
	static const uint16_t max_varying_count = 8;
	static const uint16_t max_sampler_count = 8;
	static const uint16_t max_fragment_outputs = 1; 
	static const int max_nesting = 1; 

	static const uint32_t max_tokens = 200; 

	static const uint16_t max_int_temp_count = 4; 
	static const uint16_t max_int_const_count = 4; 	

	// source registers: always 64 bits
	// 63.............................................................0	
	// D-------------QQ----IIII----TTTTSSSSSSSSOOOOOOOONNNNNNNNNNNNNNNN
	// D = Direct=0/Indirect=1 for direct Q and I are ignored, 1bit	
	// Q = Index register component select (2 bits) 
	// I = Index register type (4 bits) 
	// T = Register type (4 bits) 
	// S = Swizzle (8 bits, 2 bits per component) 
	// O = Indirect offset (8 bits) 
	// N = Register number (16 bits)
	// - = undefined, should be 0 
	struct Source {
		union {
			uint64_t packed; 
			struct {
				uint16_t regnum;
				int8_t indexoffset;
				uint8_t swizzle; 
				uint8_t regtype;				
				uint8_t indextype; 
				uint8_t indexselect; 				
				uint8_t indirectflag;
			};
		};
		void Set ( uint8_t _regtype, uint16_t _regnum, uint8_t _swizzle ) { packed = 0; regnum = _regnum; regtype = _regtype;  swizzle = _swizzle; }
	}; 

	// texture sampler register: always 64 bits
	// 63.............................................................0	
	// FFFFMMMMWWWWSSSSDDDD--------TTTT--------BBBBBBBBNNNNNNNNNNNNNNNN
	// N = Sampler index (16 bits) 
	// B = Texture lod bias, signed integer, scale by 8. the floating point value used is b/8.0 (8 bits) 
	// T = Register type, must be reg_sampler
	// F = Filter (0=nearest,1=linear,2=anisotropic) (4 bits) 
	// M = Mipmap (0=disable,1=nearest,2=linear) 
	// W = Wrapping (0=clamp,1=repeat)
	// S = Special flag bits (bit 0=centroid sampling, bit 1=single component read) 
	// D = Dimension (0=2D,1=Cube,2=3D) 
	struct Sampler {
		union {
			uint64_t packed; 
			struct {
				uint16_t regnum;
				int8_t lodbias; 
				uint8_t _reserved0; 
				uint8_t regtype; 
				uint8_t _reserved2:4;
				uint8_t dim:4; 
				uint8_t special:4;
				uint8_t wrap:4; 
				uint8_t mipmap:4; 
				uint8_t filter:4;
			};
		};
	}; 

	static const uint8_t filter_nearest = 0;
	static const uint8_t filter_linear = 1;
	static const uint8_t filter_anisotropic = 2; 

	static const uint8_t mipmap_none = 0;
	static const uint8_t mipmap_nearest = 1;
	static const uint8_t mipmap_linear = 2;

	static const uint8_t wrap_clamp = 0;
	static const uint8_t wrap_repeat = 1;

	static const uint8_t dim_2d = 0;
	static const uint8_t dim_cube = 1;
	static const uint8_t dim_3d = 2;
	static const uint8_t dim_rect = 3;

	static const uint8_t special_centroid = (1<<0);	
	static const uint8_t special_single = (1<<1); 
	static const uint8_t special_depth = (1<<2); 

	// destination registers: always 32 bits
	// 31.............................0
	// ----TTTT----MMMMNNNNNNNNNNNNNNNN
	// T = Register type (4 bits) 
	// M = Write mask (4 bits) 
	// N = Register number (16 bits)
	// - = undefined, should be 0 
	struct Destination {
		union {
			uint32_t packed; 
			struct {
				uint16_t regnum;
				uint8_t writemask; 
				uint8_t regtype; 
			};
		};
		void Set ( uint8_t _regtype, uint16_t _regnum, uint8_t _writemask ) { regnum = _regnum; regtype = _regtype; writemask = _writemask; }
	};

	// opcodes: always 192 bits
	// opcode[32] dest[32] source[64] (source[64] or sampler[64])
	struct Opcode {
		union {
			uint8_t rawbytes[24]; 
			struct {
				uint32_t op; 
				Destination dest; 
				Source sourcea; 
				union {
					Source sourceb; 
					Sampler sampler; 
				}; 
			};
		};
	}; 

#if defined(_WIN32) || defined(_MAC) || defined(UNIX)
	#pragma pack(pop)
#else
	#error use the correct packing pragma for your platform
#endif

	// some usual swizzle constants
	static const uint8_t swizzle_all_x = 0; 
	static const uint8_t swizzle_all_y = 0x55; 
	static const uint8_t swizzle_all_z = 0xaa; 
	static const uint8_t swizzle_all_w = 0xff; 
	static const uint8_t swizzle_identity = 0xe4; 
	static const uint8_t swizzle_xyzz = 0xa4;

	static const uint8_t destwrite_x = 0x1; 
	static const uint8_t destwrite_y = 0x2; 
	static const uint8_t destwrite_z = 0x4; 
	static const uint8_t destwrite_w = 0x8; 
	static const uint8_t destwrite_xyz = 0x7; 
	static const uint8_t destwrite_xyzw = 0xf; 


	// opcode mnemonics and numeric values
	static const uint32_t op_mov = 0x00;
	static const uint32_t op_add = 0x01; 
	static const uint32_t op_sub = 0x02; 
	static const uint32_t op_mul = 0x03; 
	static const uint32_t op_div = 0x04; 
	static const uint32_t op_rcp = 0x05; 
	static const uint32_t op_min = 0x06; 
	static const uint32_t op_max = 0x07; 
	static const uint32_t op_frc = 0x08; 
	static const uint32_t op_sqt = 0x09; 
	static const uint32_t op_rsq = 0x0a; 
	static const uint32_t op_pow = 0x0b; 
	static const uint32_t op_log = 0x0c; 
	static const uint32_t op_exp = 0x0d; 
	static const uint32_t op_nrm = 0x0e; 
	static const uint32_t op_sin = 0x0f; 
	static const uint32_t op_cos = 0x10; 
	static const uint32_t op_crs = 0x11; 
	static const uint32_t op_dp3 = 0x12; 
	static const uint32_t op_dp4 = 0x13; 
	static const uint32_t op_abs = 0x14; 
	static const uint32_t op_neg = 0x15; 
	static const uint32_t op_sat = 0x16; 
	static const uint32_t op_m33 = 0x17; 
	static const uint32_t op_m44 = 0x18; 
	static const uint32_t op_m43 = 0x19; 
	static const uint32_t op_ifz = 0x1a; 
	static const uint32_t op_inz = 0x1b; 
	static const uint32_t op_ife = 0x1c; 
	static const uint32_t op_ine = 0x1d; 
	static const uint32_t op_ifg = 0x1e; 
	static const uint32_t op_ifl = 0x1f; 
	static const uint32_t op_ieg = 0x20; 
	static const uint32_t op_iel = 0x21; 
	static const uint32_t op_els = 0x22; 
	static const uint32_t op_eif = 0x23; 
	static const uint32_t op_rep = 0x24; 
	static const uint32_t op_erp = 0x25; 
	static const uint32_t op_brk = 0x26; 
	static const uint32_t op_kil = 0x27; 
	static const uint32_t op_tex = 0x28; 
	static const uint32_t op_sge = 0x29; 
	static const uint32_t op_slt = 0x2a; 
	static const uint32_t op_sgn = 0x2b; 
	static const uint32_t op_seq = 0x2c; 
	static const uint32_t op_sne = 0x2d; 

	// one entry per opcode
	struct OpcodeMap {
		char mnemonic[4]; 
		uint8_t opcode; 		
		uint16_t flags;
		uint8_t srcaflags;
		uint8_t srcbflags;		
	}; 

	static const int opmap_max = 0x2e;

	// flags for opcode and destination
	static const uint8_t f_nodest			= 0x01; 
	static const uint8_t f_incnest			= 0x02; 
	static const uint8_t f_decnest			= 0x04; 
	static const uint8_t f_fragonly			= 0x08; 
	static const uint8_t f_secsrcnoswizzle  = 0x10;
	static const uint8_t f_outxyz			= 0x20;
	static const uint8_t f_flow				= 0x40; 
	static const uint8_t f_horiz			= 0x80; // horizontal opcode, not per channel 	
	static const uint8_t f_notimpl          = 0xff; // opcode not implemented
	
	
	// source flags
	static const uint8_t fs_scalar			= 0x01;
	static const uint8_t fs_vector			= 0x02;
	static const uint8_t fs_constscalar		= 0x04;
	static const uint8_t fs_sampler			= 0x08;
	static const uint8_t fs_matrix3			= 0x10; 
	static const uint8_t fs_matrix4			= 0x20; 
	static const uint8_t fs_noindirect		= 0x40; 

	// map data
	static const OpcodeMap opmap[opmap_max] = {
		{ "mov", 0x00, 0, fs_vector, 0 },
		{ "add", 0x01, 0, fs_vector, fs_vector },
		{ "sub", 0x02, 0, fs_vector, fs_vector },
		{ "mul", 0x03, 0, fs_vector, fs_vector },
		{ "div", 0x04, 0, fs_vector, fs_vector }, // a*rcp(b) enough? 
		{ "rcp", 0x05, 0, fs_vector, 0 },
		{ "min", 0x06, 0, fs_vector, fs_vector },
		{ "max", 0x07, 0, fs_vector, fs_vector },
		{ "frc", 0x08, 0, fs_vector, 0 },
		{ "sqt", 0x09, 0, fs_vector, 0 },
		{ "rsq", 0x0a, 0, fs_vector, 0 },
		{ "pow", 0x0b, 0, fs_vector, fs_vector }, // a^b ~= a / (b - a * b + a) enough? 
		{ "log", 0x0c, 0, fs_vector, 0 },
		{ "exp", 0x0d, 0, fs_vector, 0 },
		{ "nrm", 0x0e, f_outxyz | f_horiz, fs_vector, 0 },
		{ "sin", 0x0f, 0, fs_vector, 0 },
		{ "cos", 0x10, 0, fs_vector, 0 },
		{ "crs", 0x11, f_outxyz | f_horiz, fs_vector, fs_vector },
		{ "dp3", 0x12, f_horiz, fs_vector, fs_vector },
		{ "dp4", 0x13, f_horiz, fs_vector, fs_vector },
		{ "abs", 0x14, 0, fs_vector, 0 },
		{ "neg", 0x15, 0, fs_vector, 0 },
		{ "sat", 0x16, 0, fs_vector, 0 },
		{ "m33", 0x17, f_horiz | f_outxyz | f_secsrcnoswizzle, fs_vector, fs_matrix3 },
		{ "m44", 0x18, f_horiz | f_secsrcnoswizzle, fs_vector, fs_matrix4 },
		{ "m43", 0x19, f_horiz | f_outxyz | f_secsrcnoswizzle, fs_vector, fs_matrix3 }, 
		{ "ifz", 0x1a, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, 0 },
		{ "inz", 0x1b, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, 0 },
		{ "ife", 0x1c, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar }, // scalar does not make any difference... can check for all component swizzle though
		{ "ine", 0x1d, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar },
		{ "ifg", 0x1e, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar },
		{ "ifl", 0x1f, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar },
		{ "ieg", 0x20, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar },
		{ "iel", 0x21, f_notimpl | f_nodest | f_incnest | f_flow, fs_scalar, fs_scalar },
		{ "els", 0x22, f_notimpl | f_nodest | f_incnest | f_decnest | f_flow, 0, 0 },
		{ "eif", 0x23, f_notimpl | f_nodest | f_decnest | f_flow, 0, 0 },
		{ "rep", 0x24, f_notimpl | f_nodest | f_incnest | f_flow, fs_constscalar, 0 },
		{ "erp", 0x25, f_notimpl | f_nodest | f_decnest | f_flow, 0, 0 },
		{ "brk", 0x26, f_notimpl | f_nodest | f_flow, 0, 0 },
		{ "kil", 0x27, f_nodest | f_fragonly, fs_scalar, 0 }, // kill fragment, if scalar < 0 
		{ "tex", 0x28, f_horiz | f_fragonly, fs_vector | fs_noindirect, fs_sampler | fs_noindirect },
		{ "sge", 0x29, 0, fs_vector, fs_vector }, // set-if-greater-equal: dest = src0 >= src1 ? 1 : 0 
		{ "slt", 0x2a, 0, fs_vector, fs_vector }, // set-if-less-than: dest = src0 < src1 ? 1 : 0 
		{ "sgn", 0x2b, f_notimpl, fs_vector, 0 }, // sign (-1,0,1)
		{ "seq", 0x2c, 0, fs_vector, fs_vector }, // set-if-equal: dest = src0 == src1 ? 1 : 0 
		{ "sne", 0x2d, 0, fs_vector, fs_vector }, // set-if-not-equal: dest = src0 != src1 ? 1 : 0 
	};

	// helper code

	static const int max_const_batch_count = 8; 

	// analysis report of an agal bytecode
	// this can (and should be) used to create declarations when needed in a backend implementation
	struct Description {
		uint8_t shadertype; // shadertype_fragment or shadertype_vertex

		// counts
		int numsamplers; 
		int numinstructions; 
		int numvarying; 
		int numtemp; 
		int numconst;

		// sampler mappings (for texture flags)
		AGAL::Sampler samplers[AGAL::max_sampler_count];

		// read/write masked inputs (all of these values are bitmask) 
		uint8_t read_attribute[AGAL::max_attrib_count]; 
		union {
			uint8_t write_varying[AGAL::max_varying_count];		
			uint8_t read_varying[AGAL::max_varying_count];
		}; 
		uint8_t read_const[AGAL::max_vertex_const_count];
		uint8_t write_output;  
		uint8_t readwrite_temp[AGAL::max_temp_count]; 

		// only possible after an Unpack operation. Included here for decalarions
		uint8_t read_int_const[AGAL::max_int_const_count];
		uint8_t readwrite_int_temp[AGAL::max_int_temp_count];		
		uint8_t readwrite_int_addr;		

		// additional properties
		bool hasindirect;	// has indirect adressing, every const can be read
		bool hassincos;		// for complex macros		
		bool haspredicated; // has a conditional 
		// count some special instructions
		int numtex; 
		int numkil; 

		// constants as batch list 
		int numbatches; 
		struct {
			uint16_t start; 
			uint16_t len; 
		}constbatches[AGAL::max_const_batch_count];
	}; 
	
	// strictly validate AGAL bytecode against limits. 
	// this function assumes hostile/network input
	// all later AGAL processing assumes this has passed
	// validate and describe do similar things, but it is nice to have validate 
	// be the only place that has to care about security checking and that it has no other job whatsoever 
	bool Validate ( const char *bytecode, size_t bytecodelen, bool allowinternal ); 

	// describe AGAL, fill in a AGAL::Description structure 
	// Warning: This assumes that ValidateAGAL already passed, otherwise this function is unsafe
	void Describe ( const char *bytecode, size_t bytecodelen, Description *dest );

	// validate that two descriptions match 
	bool ValidateLinkage ( const Description *frag, const Description *vert ); 

	

	// debugging and testing functions... 
#ifdef INCLUDE_AGAL_ASSEMBLER
	static const int asm_noerror = 0; 
	static const int asm_internal = 1; 	
	static const int asm_invalidopcode = 2; 
	static const int asm_linetooshort = 3; 
	static const int asm_notoken = 4; 
	static const int asm_mismatchedbrackets = 5; 
	static const int asm_extratoken = 6; 
	static const int asm_badregname = 7;
	static const int asm_noregnum = 8;
	static const int asm_badmask = 9;
	static const int asm_badswizzle = 10; 
	static const int asm_noindirect = 11;	
	static const int asm_badtexflag = 12;

	static const char *asm_errorstrings[] = {"No error","Internal error","Invalid opcode","Line too short","Missing token","Unmatched bracket","Extra token",
		"Invalid register name", "Missing register number", "Unexpected mask character", "Unexpected swizzle character", "Unexpected indirect adressing",
		"Unknown texture flag" };

	int Assemble ( const char *source, uint8_t shadertype, char **bytecode, size_t *bytecodelen ); 
	void Disassemble ( const char *bytecode, size_t bytecodelen, FlashString *dest ); 	
#endif

	// some more common helpers

	// check if a dest register matches a source register (type+number) 
	inline bool IsSameRegister ( const Destination &d, const Source &s );
	// check if only one bit is set, return index or -1 if not scalar
	inline int GetScalarOutput ( const Destination &d );
	// check if all elements are a swizzle from the same source, return index or -1 if not scalar
	// check for .xxxx, .yyyy, .zzzz, .wwww (or .x, .y, .z, .w)
	int GetScalarInput ( const Source &s ); 
	// check for linear swizzle: x,y,z,w,xy,yz,zw,xyz,yzw
	inline bool IsLinearSwizzle ( const Source &s ); 
	// get the channel number that is swizzled to. example: for channel y(1), a swizzle of .xzxx produces z(2)
	inline int GetSwizzledChannel ( const Source &s, int basechannel );

	uint8_t ReplicateSwizzle ( uint8_t c );
	inline uint8_t ReplicateLastComponent ( uint8_t swiz );

	inline uint8_t ExtractAndReplicateSwizzle ( uint8_t c, uint8_t deskwriteMask );

	// int constant values, needed when unpack creates regtype_int_const. these are the expected values
	// on those registers
	static const float int_const_values[max_int_const_count*4] = { 0,1,0,0, // z & w free to use
		-1.5500992e-006f, -2.1701389e-005f,  0.0026041667f, 0.00026041668f, // D3DSINCOSCONST1 
		-0.020833334f, -0.12500000f, 1.0f, 0.50000000f,						// D3DSINCOSCONST2 
		1.0f/(2.0f*float(M_PI)),.5f,2.0f*float(M_PI),float(-M_PI) }; // 1/(2*pi), .5, 2*pi, -pi
														
	// unpacking: based on some flags, change code to eliminate some common issues before translating for backends
	// all flags are optional. after unpacking there will be internal opcodes. 
	// assumes code is validated
	static const uint32_t unpack_matrix = 1<<0;				// matrix opcodes are turned to dot product opcodes
	static const uint32_t unpack_nrm = 1<<1;				// replace normalize by dp/rsq/mul sequence
	static const uint32_t unpack_nonlinear_swizzle = 1<<2;	// fix nonlinear swizzles through temp reads or scalar unpack
	static const uint32_t unpack_single_tex = 1<<3;			// single tex reads fixup(expand to all components) 
	static const uint32_t unpack_div = 1<<4;				// replace divide by rcp/mul 
	static const uint32_t unpack_sat = 1<<5;				// replace sat by min/max const
	static const uint32_t unpack_nrm_need_temp = 1<<6;		// normalize can not write to source register	
	static const uint32_t unpack_addr = 1<<8;               // before all indirect reads the adr register is loaded (no redundant loads)	
	static const uint32_t unpack_sqrt = 1<<9;				// sqrt to 1/(1/sqrt(x))
	static const uint32_t unpack_neg = 1<<10;				// negate to 0-x
	static const uint32_t unpack_abs = 1<<11;				// abs to max(x,neg(x))  - also a good test for two pass with neg
	static const uint32_t unpack_tex_writemask = 1<<12;		// need temp for texture writemask	
	static const uint32_t unpack_kill = 1<<13;				// unpack kil to an all-components temp 
	static const uint32_t unpack_only_one_attrib_reg = 1<<14; // two attributes regs can not be used as source regs
	static const uint32_t unpack_op_writes_to_temp = 1<<15; // crs/pow needs to write to a temp register
	static const uint32_t unpack_one_component = 1<<16;		// op only works with one component
	static const uint32_t unpack_nrm_destmask_must_be_xyz = 1<<17; // nrm wants to write to the full xyz value, not just one component
	static const uint32_t unpack_only_one_varying_reg = 1<<18; // two varying regs can not be used as source regs
	static const uint32_t unpack_tex_nosrcswizzle = 1<<19; // tex can not swizzle the 1st source arg
	static const uint32_t unpack_crs_nosrcswizzle = 1<<20; // crs can not swizzle either source arg
	static const uint32_t unpack_srca_indirect = 1<<21; // d3d does not like indirect addressing for sourcea
	static const uint32_t unpack_frag_output_limited = 1<<22; // d3d does not like swizzle when writing to output and can only use mov
	static const uint32_t unpack_pow_diff_dest_sourceb = 1<<23; // d3d does not like pow to use same reg for dest and sourceb
	static const uint32_t unpack_crs_diff_dest_source = 1<<24; // d3d does not like crs to use same reg for dest and sourcea or sourceb
	static const uint32_t unpack_matrix_diff_dest_source = 1<<25; // d3d does not like m** to use same reg for dest and sourcea 
	static const uint32_t unpack_matrix_destmasked = 1<<26;		// d3d does not allow dest masking for m** opcodes 

	bool Unpack ( const char *bytecode, size_t bytecodelen, uint32_t flags, char **newbytecode, size_t *newbytecodelen, uint16_t pass=0 ); 


#ifdef INCLUDE_AGAL_SIMPLEDEPENDENCY
	static const uint8_t sdf_sink = (1<<0); 
	static const uint8_t sdf_needtemp = (1<<1); 
	static const uint8_t sdf_relative = (1<<2);
	static const uint8_t sdf_simpleop = (1<<3);		// simple ops are: no writemask, no source swizzle (both sources) 
	static const uint8_t sdf_scalarop = (1<<4);		// single writemask, single source swizzle (both sources) 
	static const uint8_t sdf_linearop = (1<<5);		// same writemask as both source swizzles, either .x, .xy, .xyz, .xyzw
	static const uint8_t sdf_incompleteread = (1<<6); 

	static const int16_t sdsrc_unknown = -4; 
	static const int16_t sdsrc_unused = -3;			// empty
	static const int16_t sdsrc_constant = -2;		// uniform, varying, sampler etc
	static const int16_t sdsrc_complex = -1;		// from temp 

	struct SimpleDependencies { // one for each token 
		AGAL::Opcode op;		// just copy op 
		uint8_t flags;			// sdf_* flags
		uint8_t usedwritemask;	// values _actually_ being read
		uint16_t numreaders;	// number of tokens reading from this result
		int16_t srca, srcb;		// _IF_ single token responsible for this input, index of that token 
	};

	SimpleDependencies * CreateSimpleDependencies ( const char *bytecode, size_t bytecodelen );

#endif


#ifdef INCLUDE_AGAL_DEPENDENCYGRAPH

	static const uint8_t gf_sink = (1<<0); 
	static const uint8_t gf_active = (1<<1); 
	static const uint8_t gf_recursing = (1<<2); 

	struct Edge {
		uint16_t srcidx; // data from
		uint16_t dstidx; // data to
		uint8_t mask; 
	};

	struct Node {
		Opcode op;									// copy of original opcode, convenience 
		uint16_t idx;								// array index in allnodes, also ((Opcode*)(bytecode+sizeof(AGAL::Header)))[nodeidx] == op 		
		uint8_t writemask;							// 0 for dead opcodes, actually later read data
		uint8_t flags;								// gf_*, 0 for inactive
		PArray<uint16_t> inputs;					// index into edges array
		PArray<uint16_t> outputs;					// index into edges array	
	};

	struct Graph {
		PArray<uint16_t> sinks; 		
		PArray<Node> allnodes; 		
		PArray<Edge> edges; 

		uint8_t sink_varying_written[AGAL::max_varying_count];
		uint8_t sink_output_written[AGAL::max_fragment_outputs]; 
		unsigned int sink_kill_count; 
	};

	Graph * CreateDependencyGraph ( const char *bytecode, size_t bytecodelen, uint32_t flags );
	void PrintGraph ( AGAL::Graph *graph );

#endif

#ifdef INCLUDE_AGAL_INLINE
	void InlineOperations ( const char *bytecode, size_t bytecodelen, char **newbytecode, size_t *newbytecodelen );
#endif

}; 

#endif
