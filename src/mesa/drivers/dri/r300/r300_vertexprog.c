#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "nvvertprog.h"

static void r300BindProgram(GLcontext *ctx, GLenum target, struct program *prog)
{
	fprintf(stderr, "r300BindProgram\n");
}


static struct program *r300NewProgram(GLcontext *ctx, GLenum target, GLuint id)
{
r300ContextPtr rmesa = R300_CONTEXT(ctx);
struct r300_vertex_program *vp;
struct fragment_program *fp;
struct ati_fragment_shader *afs;

	fprintf(stderr, "r300NewProgram, target=%d, id=%d\n", target, id);

	switch(target){
		case GL_VERTEX_PROGRAM_ARB:
		fprintf(stderr, "vertex prog\n");
		vp=malloc(sizeof(*vp));
		memset(vp, 0, sizeof(*vp));
		
		/* note that vp points to mesa_program since its first on the struct
		*/
		return _mesa_init_vertex_program(ctx, &vp->mesa_program, target, id);
		
		case GL_FRAGMENT_PROGRAM_ARB:
		fprintf(stderr, "fragment prog\n");
		fp=malloc(sizeof(*fp));
		memset(fp, 0, sizeof(*fp));
			
		return _mesa_init_fragment_program(ctx, fp, target, id);
		case GL_FRAGMENT_PROGRAM_NV:
		fprintf(stderr, "nv fragment prog\n");
		fp=malloc(sizeof(*fp));
		memset(fp, 0, sizeof(*fp));
			
		return _mesa_init_fragment_program(ctx, fp, target, id);
		
		case GL_FRAGMENT_SHADER_ATI:
		fprintf(stderr, "ati fragment prog\n");
		afs=malloc(sizeof(*afs));
		memset(afs, 0, sizeof(*afs));
			
		return _mesa_init_ati_fragment_shader(ctx, afs, target, id);
		
		default:
			return NULL;
	}
	
}


static void r300DeleteProgram(GLcontext *ctx, struct program *prog)
{
	fprintf(stderr, "r300DeleteProgram\n");
	
	/* check that not active */
	_mesa_delete_program(ctx, prog);
}
     
static void r300ProgramStringNotify(GLcontext *ctx, GLenum target, 
				struct program *prog)
{
	struct r300_vertex_program *vp=(void *)prog;
	
	fprintf(stderr, "r300ProgramStringNotify\n");
	r300IsProgramNative(ctx, target, prog);

	switch(target) {
	case GL_VERTEX_PROGRAM_ARB:
		vp->translated=GL_FALSE;
		break;
	}
	
}

#define SCALAR_FLAG (1<<31)
#define FLAG_MASK (1<<31)
#define OPN(operator, ip, op) {#operator, VP_OPCODE_##operator, ip, op}
struct{
	char *name;
	int opcode;
	unsigned long ip; /* input reg index */
	unsigned long op; /* output reg index */
}op_names[]={
	OPN(ABS, 1, 1),
	OPN(ADD, 2, 1),
	OPN(ARL, 1, 1|SCALAR_FLAG),
	OPN(DP3, 2, 3|SCALAR_FLAG),
	OPN(DP4, 2, 3|SCALAR_FLAG),
	OPN(DPH, 2, 3|SCALAR_FLAG),
	OPN(DST, 2, 1),
	OPN(EX2, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(EXP, 1|SCALAR_FLAG, 1),
	OPN(FLR, 1, 1),
	OPN(FRC, 1, 1),
	OPN(LG2, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(LIT, 1, 1),
	OPN(LOG, 1|SCALAR_FLAG, 1),
	OPN(MAD, 3, 1),
	OPN(MAX, 2, 1),
	OPN(MIN, 2, 1),
	OPN(MOV, 1, 1),
	OPN(MUL, 2, 1),
	OPN(POW, 2|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(RCP, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(RSQ, 1|SCALAR_FLAG, 4|SCALAR_FLAG),
	OPN(SGE, 2, 1),
	OPN(SLT, 2, 1),
	OPN(SUB, 2, 1),
	OPN(SWZ, 1, 1),
	OPN(XPD, 2, 1),
	OPN(RCC, 0, 0), //extra
	OPN(PRINT, 0, 0),
	OPN(END, 0, 0),
};
#undef OPN
#define OPN(rf) {#rf, PROGRAM_##rf}

struct{
	char *name;
	int id;
}register_file_names[]={
	OPN(TEMPORARY),
	OPN(INPUT),
	OPN(OUTPUT),
	OPN(LOCAL_PARAM),
	OPN(ENV_PARAM),
	OPN(NAMED_PARAM),
	OPN(STATE_VAR),
	OPN(WRITE_ONLY),
	OPN(ADDRESS),
};
	
char *dst_mask_names[4]={ "X", "Y", "Z", "W" };

/* from vertex program spec:
      Instruction    Inputs  Output   Description
      -----------    ------  ------   --------------------------------
      ABS            v       v        absolute value
      ADD            v,v     v        add
      ARL            v       a        address register load
      DP3            v,v     ssss     3-component dot product
      DP4            v,v     ssss     4-component dot product
      DPH            v,v     ssss     homogeneous dot product
      DST            v,v     v        distance vector
      EX2            s       ssss     exponential base 2
      EXP            s       v        exponential base 2 (approximate)
      FLR            v       v        floor
      FRC            v       v        fraction
      LG2            s       ssss     logarithm base 2
      LIT            v       v        compute light coefficients
      LOG            s       v        logarithm base 2 (approximate)
      MAD            v,v,v   v        multiply and add
      MAX            v,v     v        maximum
      MIN            v,v     v        minimum
      MOV            v       v        move
      MUL            v,v     v        multiply
      POW            s,s     ssss     exponentiate
      RCP            s       ssss     reciprocal
      RSQ            s       ssss     reciprocal square root
      SGE            v,v     v        set on greater than or equal
      SLT            v,v     v        set on less than
      SUB            v,v     v        subtract
      SWZ            v       v        extended swizzle
      XPD            v,v     v        cross product
*/

void dump_program_params(struct vertex_program *vp){
	int i;
	int pi;

	fprintf(stderr, "NumInstructions=%d\n", vp->Base.NumInstructions);
	fprintf(stderr, "NumTemporaries=%d\n", vp->Base.NumTemporaries);
	fprintf(stderr, "NumParameters=%d\n", vp->Base.NumParameters);
	fprintf(stderr, "NumAttributes=%d\n", vp->Base.NumAttributes);
	fprintf(stderr, "NumAddressRegs=%d\n", vp->Base.NumAddressRegs);
	
	for(pi=0; pi < vp->Base.NumParameters; pi++){
		fprintf(stderr, "{ ");
		for(i=0; i < 4; i++)
			fprintf(stderr, "%f ", vp->Base.LocalParams[pi][i]);
		fprintf(stderr, "}\n");
	}
	
	for(pi=0; pi < vp->Parameters->NumParameters; pi++){
		fprintf(stderr, "param %02d:", pi);
		
		switch(vp->Parameters->Parameters[pi].Type){
			
		case NAMED_PARAMETER:
			fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
			fprintf(stderr, "(NAMED_PARAMETER)");
		break;
		
		case CONSTANT:
			fprintf(stderr, "(CONSTANT)");
		break;
		
		case STATE:
			fprintf(stderr, "(STATE)\n");
			/* fetch state info */
			continue;
		break;

		}
		
		fprintf(stderr, "{ ");
		for(i=0; i < 4; i++)
			fprintf(stderr, "%f ", vp->Parameters->Parameters[pi].Values[i]);
		fprintf(stderr, "}\n");
		
	}
}

static GLboolean r300IsProgramNative(GLcontext *ctx, GLenum target,
				struct program *prog)
{
	struct vertex_program *vp=(void *)prog;
	struct vp_instruction *vpi;
	int i, operand_index;
	int operator_index;
	
	fprintf(stderr, "r300IsProgramNative\n");
	//exit(0);
	
	dump_program_params(vp);

	vpi=vp->Instructions;
	
	for(;; vpi++){
		if(vpi->Opcode == VP_OPCODE_END)
			break;
		
		for(i=0; i < sizeof(op_names) / sizeof(*op_names); i++){
			if(vpi->Opcode == op_names[i].opcode){
				fprintf(stderr, "%s ", op_names[i].name);
				break;
			}
		}
		operator_index=i;
		
		for(i=0; i < sizeof(register_file_names) / sizeof(*register_file_names); i++){
			if(vpi->DstReg.File == register_file_names[i].id){
				fprintf(stderr, "%s ", register_file_names[i].name);
				break;
			}
		}
		
		fprintf(stderr, "%d.", vpi->DstReg.Index);
		
		for(i=0; i < 4; i++)
			if(vpi->DstReg.WriteMask[i])
				fprintf(stderr, "%s", dst_mask_names[i]);
		fprintf(stderr, " ");
		
		for(operand_index=0; operand_index < op_names[operator_index].ip & (~FLAG_MASK);
			operand_index++){
				
			if(vpi->SrcReg[operand_index].Negate)
				fprintf(stderr, "-");
			
			for(i=0; i < sizeof(register_file_names) / sizeof(*register_file_names); i++){
				if(vpi->SrcReg[operand_index].File == register_file_names[i].id){
					fprintf(stderr, "%s ", register_file_names[i].name);
					break;
				}
			}
			fprintf(stderr, "%d.", vpi->SrcReg[operand_index].Index);
			
			for(i=0; i < 4; i++)
				fprintf(stderr, "%s", dst_mask_names[vpi->SrcReg[operand_index].Swizzle[i]]);
			
			if(operand_index+1 < op_names[operator_index].ip & (~FLAG_MASK) )
				fprintf(stderr, ",");
		}
		fprintf(stderr, "\n");
		//op_names[i].ip
		//op_names[i].op
	}
	return 1;
}

/* This is misnamed and shouldnt be here since fragment programs use these functions too */		
void r300InitVertexProgFuncs(struct dd_function_table *functions)
{
#if 1
	functions->NewProgram=r300NewProgram;
	functions->BindProgram=r300BindProgram;
	functions->DeleteProgram=r300DeleteProgram;
	functions->ProgramStringNotify=r300ProgramStringNotify;
	functions->IsProgramNative=r300IsProgramNative;
#endif	
}
