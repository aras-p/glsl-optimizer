#ifndef __NOUVEAU_SHADER_H__
#define __NOUVEAU_SHADER_H__

typedef struct _nouveau_regrec         nouveau_regrec;
typedef struct _nouveau_srcreg         nouveau_srcreg;
typedef struct _nouveau_dstreg         nouveau_dstreg;
typedef struct _nouveau_vertex_program nouveau_vertex_program;

/* Instruction flags, used by emit_arith functions */
#define NOUVEAU_OUT_ABS (1 << 0)
#define NOUVEAU_OUT_SAT (1 << 1)

typedef enum {
	UNKNOWN = 0,
	HW_TEMP,
	HW_INPUT,
	HW_CONST,
	HW_OUTPUT
} nouveau_regtype;

/* To track a hardware register's state */
struct _nouveau_regrec {
	nouveau_regtype file;
	int             hw_id;
	int             ref;
};

struct _nouveau_srcreg {
	nouveau_regrec *hw;
	int idx;

	int negate;
	int swizzle;
};

struct _nouveau_dstreg {
	nouveau_regrec *hw;
	int idx;

	int mask;

	int condup, condreg;
	int condtest;
	int condswz;
};

struct _nouveau_vertex_program {
	struct vertex_program mesa_program; /* must be first! */

	/* Used to convert from Mesa register state to on-hardware state */
	long           *temps_in_use;
	nouveau_regrec  inputs[14];
	nouveau_regrec  temps[64];

	long           *hwtemps_written;
	long           *hwtemps_in_use;

	unsigned int   *insns;
	unsigned int    insns_alloced;
	unsigned int    inst_count;
	unsigned int    inst_start;
};

/* Helper functions */
void nvsRecInit   (long **rec, int max);
void nvsBitSet    (long *rec, int bit);
void nvsBitClear  (long *rec, int bit);
int  nvsAllocIndex(long *rec, int max);

int nv40TranslateVertexProgram(nouveau_vertex_program *vp);
//int nv40TranslateFragmentProgram(nouveau_vertex_program *vp);

#endif /* __NOUVEAU_SHADER_H__ */

