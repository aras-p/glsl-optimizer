#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "nvvertprog.h"

#define SCALAR_FLAG (1<<31)
#define FLAG_MASK (1<<31)
#define OPN(operator, ip, op) {#operator, VP_OPCODE_##operator, ip, op}

#warning "This is just a hack to get everything to compile"
struct ati_fragment_shader {};

struct{
	char *name;
	int opcode;
	unsigned long ip; /* number of input operands and flags */
	unsigned long op;
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

void dump_program_params(struct vertex_program *vp)
{
	int i;
	int pi;

	fprintf(stderr, "NumInstructions=%d\n", vp->Base.NumInstructions);
	fprintf(stderr, "NumTemporaries=%d\n", vp->Base.NumTemporaries);
	fprintf(stderr, "NumParameters=%d\n", vp->Base.NumParameters);
	fprintf(stderr, "NumAttributes=%d\n", vp->Base.NumAttributes);
	fprintf(stderr, "NumAddressRegs=%d\n", vp->Base.NumAddressRegs);
	
#if 0	
	for(pi=0; pi < vp->Base.NumParameters; pi++){
		fprintf(stderr, "{ ");
		for(i=0; i < 4; i++)
			fprintf(stderr, "%f ", vp->Base.LocalParams[pi][i]);
		fprintf(stderr, "}\n");
	}
#endif	
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

static void debug_vp(struct vertex_program *vp)
{
	struct vp_instruction *vpi;
	int i, operand_index;
	int operator_index;
	
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
		
		for(operand_index=0; operand_index < (op_names[operator_index].ip & (~FLAG_MASK));
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
			
			if(operand_index+1 < (op_names[operator_index].ip & (~FLAG_MASK)) )
				fprintf(stderr, ",");
		}
		fprintf(stderr, "\n");
	}
	
}

void update_params(struct r300_vertex_program *vp)
{
	int pi;
	struct vertex_program *mesa_vp=(void *)vp;
	
	vp->params.length=0;
	
	/* Temporary solution */
	
	for(pi=0; pi < mesa_vp->Parameters->NumParameters; pi++){
		switch(mesa_vp->Parameters->Parameters[pi].Type){
			
		case NAMED_PARAMETER:
			//fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
		case CONSTANT:
			vp->params.body.f[pi*4+0]=mesa_vp->Parameters->Parameters[pi].Values[0];
			vp->params.body.f[pi*4+1]=mesa_vp->Parameters->Parameters[pi].Values[1];
			vp->params.body.f[pi*4+2]=mesa_vp->Parameters->Parameters[pi].Values[2];
			vp->params.body.f[pi*4+3]=mesa_vp->Parameters->Parameters[pi].Values[3];
			vp->params.length+=4;
		break;

		case STATE:
			fprintf(stderr, "State found! bailing out.\n");
			exit(0);
			/* fetch state info */
			continue;
		break;
		default: _mesa_problem(NULL, "Bad param type in %s", __FUNCTION__);
		}
	
	}
	
}
		
unsigned long translate_dst_mask(GLboolean *mask)
{
	unsigned long flags=0;
	
	if(mask[0]) flags |= VSF_FLAG_X;
	if(mask[1]) flags |= VSF_FLAG_Y;
	if(mask[2]) flags |= VSF_FLAG_Z;
	if(mask[3]) flags |= VSF_FLAG_W;
	
	return flags;
}

unsigned long translate_dst_class(enum register_file file)
{
	
	switch(file){
		case PROGRAM_TEMPORARY:
			return R300_VPI_OUT_REG_CLASS_TEMPORARY;
		case PROGRAM_OUTPUT:
			return R300_VPI_OUT_REG_CLASS_RESULT;
		/*	
		case PROGRAM_INPUT:
		case PROGRAM_LOCAL_PARAM:
		case PROGRAM_ENV_PARAM:
		case PROGRAM_NAMED_PARAM:
		case PROGRAM_STATE_VAR:
		case PROGRAM_WRITE_ONLY:
		case PROGRAM_ADDRESS:
		*/
		default:
			fprintf(stderr, "problem in %s", __FUNCTION__);
			exit(0);
	}
}

unsigned long translate_src_class(enum register_file file)
{
	
	switch(file){
		case PROGRAM_TEMPORARY:
			return R300_VPI_IN_REG_CLASS_TEMPORARY;
			
			
		case PROGRAM_INPUT:
		case PROGRAM_LOCAL_PARAM:
		case PROGRAM_ENV_PARAM:
		case PROGRAM_NAMED_PARAM:
		case PROGRAM_STATE_VAR:
			return R300_VPI_IN_REG_CLASS_PARAMETER;
		/*	
		case PROGRAM_OUTPUT:
		case PROGRAM_WRITE_ONLY:
		case PROGRAM_ADDRESS:
		*/
		default:
			fprintf(stderr, "problem in %s", __FUNCTION__);
			exit(0);
	}
}

unsigned long translate_swizzle(GLubyte swizzle)
{
	switch(swizzle){
		case 0: return VSF_IN_COMPONENT_X;
		case 1: return VSF_IN_COMPONENT_Y;
		case 2: return VSF_IN_COMPONENT_Z;
		case 3: return VSF_IN_COMPONENT_W;
		
		case SWIZZLE_ZERO:
		case SWIZZLE_ONE:
		default:
			fprintf(stderr, "problem in %s", __FUNCTION__);
			exit(0);
	}
}

unsigned long translate_src(struct vp_src_register *src)
{
	return MAKE_VSF_SOURCE(src->Index,
				translate_swizzle(src->Swizzle[0]),
				translate_swizzle(src->Swizzle[1]),
				translate_swizzle(src->Swizzle[2]),
				translate_swizzle(src->Swizzle[3]),
				translate_src_class(src->File),
				src->Negate ? VSF_FLAG_ALL : VSF_FLAG_NONE);
}

unsigned long translate_opcode(enum vp_opcode opcode)
{

	switch(opcode){
		case VP_OPCODE_ADD: return R300_VPI_OUT_OP_ADD;
		case VP_OPCODE_DST: return R300_VPI_OUT_OP_DST;
		case VP_OPCODE_EX2: return R300_VPI_OUT_OP_EX2;
		case VP_OPCODE_EXP: return R300_VPI_OUT_OP_EXP;
		case VP_OPCODE_FRC: return R300_VPI_OUT_OP_FRC;
		case VP_OPCODE_LG2: return R300_VPI_OUT_OP_LG2;
		case VP_OPCODE_LIT: return R300_VPI_OUT_OP_LIT;
		case VP_OPCODE_LOG: return R300_VPI_OUT_OP_LOG;
		case VP_OPCODE_MAD: return R300_VPI_OUT_OP_MAD;
		case VP_OPCODE_MAX: return R300_VPI_OUT_OP_MAX;
		case VP_OPCODE_MIN: return R300_VPI_OUT_OP_MIN;
		case VP_OPCODE_MUL: return R300_VPI_OUT_OP_MUL;
		case VP_OPCODE_POW: return R300_VPI_OUT_OP_POW;
		case VP_OPCODE_RCP: return R300_VPI_OUT_OP_RCP;
		case VP_OPCODE_RSQ: return R300_VPI_OUT_OP_RSQ;
		case VP_OPCODE_SGE: return R300_VPI_OUT_OP_SGE;
		case VP_OPCODE_SLT: return R300_VPI_OUT_OP_SLT;
		/* these ops need special handling */
		case VP_OPCODE_ABS: 
		case VP_OPCODE_ARL:
		case VP_OPCODE_DP3:
		case VP_OPCODE_DP4:
		case VP_OPCODE_DPH:
		case VP_OPCODE_FLR:
		case VP_OPCODE_MOV:
		case VP_OPCODE_SUB:
		case VP_OPCODE_SWZ:
		case VP_OPCODE_XPD:
		case VP_OPCODE_RCC:
		case VP_OPCODE_PRINT:
		case VP_OPCODE_END:
			fprintf(stderr, "%s should not be called with opcode %d", __FUNCTION__, opcode);
		break;
		default: 
			fprintf(stderr, "%s unknown opcode %d", __FUNCTION__, opcode);
	}
	exit(-1);
	return 0;
}
		
static void translate_program(struct r300_vertex_program *vp)
{
	struct vertex_program *mesa_vp=(void *)vp;
	struct vp_instruction *vpi;
	int inst_index=0;
	int operand_index, i;
	int op_found;	
	update_params(vp);
	
	vp->program.length=0;
	
	for(vpi=mesa_vp->Instructions; vpi->Opcode != VP_OPCODE_END; vpi++, inst_index++){
		switch(vpi->Opcode){
		case VP_OPCODE_ABS: 
		case VP_OPCODE_ARL:
		case VP_OPCODE_DP3:
		case VP_OPCODE_DP4:
		case VP_OPCODE_DPH:
		case VP_OPCODE_DST:
		case VP_OPCODE_FLR:
		case VP_OPCODE_MOV:
		case VP_OPCODE_SUB:
		case VP_OPCODE_SWZ:
		case VP_OPCODE_XPD:
		case VP_OPCODE_RCC:
		case VP_OPCODE_PRINT:
			fprintf(stderr, "Dont know how to handle op %d yet\n", vpi->Opcode);
			exit(-1);
		break;
		case VP_OPCODE_END:
			break;
		default:
			break;
		}
		vp->program.body.i[inst_index].op=MAKE_VSF_OP(translate_opcode(vpi->Opcode), vpi->DstReg.Index,
				translate_dst_mask(vpi->DstReg.WriteMask), translate_dst_class(vpi->DstReg.File));
		
		op_found=0;
		for(i=0; i < sizeof(op_names) / sizeof(*op_names); i++)
			if(op_names[i].opcode == vpi->Opcode){
				op_found=1;
				break;
			}
		if(!op_found){
			fprintf(stderr, "op %d not found in op_names\n", vpi->Opcode);
			exit(-1);
		}
			
		switch(op_names[i].ip){
			case 1:
				vp->program.body.i[inst_index].src1=translate_src(&vpi->SrcReg[0]);
				vp->program.body.i[inst_index].src2=0;
				vp->program.body.i[inst_index].src3=0;
			break;
			
			case 2:
				vp->program.body.i[inst_index].src1=translate_src(&vpi->SrcReg[0]);
				vp->program.body.i[inst_index].src2=translate_src(&vpi->SrcReg[1]);
				vp->program.body.i[inst_index].src3=0;
			break;
			
			case 3:
				vp->program.body.i[inst_index].src1=translate_src(&vpi->SrcReg[0]);
				vp->program.body.i[inst_index].src2=translate_src(&vpi->SrcReg[1]);
				vp->program.body.i[inst_index].src3=translate_src(&vpi->SrcReg[2]);
			break;
			
			default:
				fprintf(stderr, "scalars and op RCC not handled yet");
				exit(-1);
			break;
		}
	}
	vp->program.length=inst_index*4;

}

static void r300BindProgram(GLcontext *ctx, GLenum target, struct program *prog)
{
	fprintf(stderr, "r300BindProgram\n");
}

/* Mesa doesnt seem to have prototype for this */
struct program *
_mesa_init_ati_fragment_shader( GLcontext *ctx, struct ati_fragment_shader *prog,
                           GLenum target, GLuint id);

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
			vp=CALLOC_STRUCT(r300_vertex_program);
		
		/* note that vp points to mesa_program since its first on the struct
		*/
		return _mesa_init_vertex_program(ctx, &vp->mesa_program, target, id);
		
		case GL_FRAGMENT_PROGRAM_ARB:
			fprintf(stderr, "fragment prog\n");
			fp=CALLOC_STRUCT(fragment_program);
		return _mesa_init_fragment_program(ctx, fp, target, id);
		
		case GL_FRAGMENT_PROGRAM_NV:
			fprintf(stderr, "nv fragment prog\n");
			fp=CALLOC_STRUCT(fragment_program);
		return _mesa_init_fragment_program(ctx, fp, target, id);
		
		case GL_FRAGMENT_SHADER_ATI:
			fprintf(stderr, "ati fragment prog\n");
			afs=CALLOC_STRUCT(ati_fragment_shader);
		return _mesa_init_ati_fragment_shader(ctx, afs, target, id);
	}
	
	return NULL;	
}


static void r300DeleteProgram(GLcontext *ctx, struct program *prog)
{
	fprintf(stderr, "r300DeleteProgram\n");
	
	/* check that not active */
	_mesa_delete_program(ctx, prog);
}
     
static GLboolean r300IsProgramNative(GLcontext *ctx, GLenum target,
				struct program *prog);

static void r300ProgramStringNotify(GLcontext *ctx, GLenum target, 
				struct program *prog)
{
	struct r300_vertex_program *vp=(void *)prog;
	
	fprintf(stderr, "r300ProgramStringNotify\n");
	/* XXX: There is still something wrong as mesa doesnt call r300IsProgramNative at all */
	(void)r300IsProgramNative(ctx, target, prog);

	switch(target) {
	case GL_VERTEX_PROGRAM_ARB:
		vp->translated=GL_FALSE;
		break;
	}
	
}

static GLboolean r300IsProgramNative(GLcontext *ctx, GLenum target,
				struct program *prog)
{
	
	fprintf(stderr, "r300IsProgramNative\n");
	//exit(0);
	debug_vp((struct vertex_program *)prog);
	
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
