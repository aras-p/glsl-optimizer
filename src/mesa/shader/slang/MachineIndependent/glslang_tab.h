typedef union {
    struct {
        TSourceLoc line;
        union {
            TString *string;
            float f;
            int i;
            bool b;
        };
        TSymbol* symbol;
    } lex;
    struct {
        TSourceLoc line;
        TOperator op;
        union {
            TIntermNode* intermNode;
            TIntermNodePair nodePair;
            TIntermTyped* intermTypedNode;
            TIntermAggregate* intermAggregate;
        };
        union {
            TPublicType type;
            TQualifier qualifier;
            TFunction* function;
            TParameter param;
            TTypeLine typeLine;
            TTypeList* typeList;
        };
    } interm;
} YYSTYPE;
#define	ATTRIBUTE	258
#define	CONST_QUAL	259
#define	BOOL_TYPE	260
#define	FLOAT_TYPE	261
#define	INT_TYPE	262
#define	BREAK	263
#define	CONTINUE	264
#define	DO	265
#define	ELSE	266
#define	FOR	267
#define	IF	268
#define	DISCARD	269
#define	RETURN	270
#define	BVEC2	271
#define	BVEC3	272
#define	BVEC4	273
#define	IVEC2	274
#define	IVEC3	275
#define	IVEC4	276
#define	VEC2	277
#define	VEC3	278
#define	VEC4	279
#define	MATRIX2	280
#define	MATRIX3	281
#define	MATRIX4	282
#define	IN_QUAL	283
#define	OUT_QUAL	284
#define	INOUT_QUAL	285
#define	UNIFORM	286
#define	VARYING	287
#define	STRUCT	288
#define	VOID_TYPE	289
#define	WHILE	290
#define	SAMPLER1D	291
#define	SAMPLER2D	292
#define	SAMPLER3D	293
#define	SAMPLERCUBE	294
#define	SAMPLER1DSHADOW	295
#define	SAMPLER2DSHADOW	296
#define	IDENTIFIER	297
#define	TYPE_NAME	298
#define	FLOATCONSTANT	299
#define	INTCONSTANT	300
#define	BOOLCONSTANT	301
#define	FIELD_SELECTION	302
#define	LEFT_OP	303
#define	RIGHT_OP	304
#define	INC_OP	305
#define	DEC_OP	306
#define	LE_OP	307
#define	GE_OP	308
#define	EQ_OP	309
#define	NE_OP	310
#define	AND_OP	311
#define	OR_OP	312
#define	XOR_OP	313
#define	MUL_ASSIGN	314
#define	DIV_ASSIGN	315
#define	ADD_ASSIGN	316
#define	MOD_ASSIGN	317
#define	LEFT_ASSIGN	318
#define	RIGHT_ASSIGN	319
#define	AND_ASSIGN	320
#define	XOR_ASSIGN	321
#define	OR_ASSIGN	322
#define	SUB_ASSIGN	323
#define	LEFT_PAREN	324
#define	RIGHT_PAREN	325
#define	LEFT_BRACKET	326
#define	RIGHT_BRACKET	327
#define	LEFT_BRACE	328
#define	RIGHT_BRACE	329
#define	DOT	330
#define	COMMA	331
#define	COLON	332
#define	EQUAL	333
#define	SEMICOLON	334
#define	BANG	335
#define	DASH	336
#define	TILDE	337
#define	PLUS	338
#define	STAR	339
#define	SLASH	340
#define	PERCENT	341
#define	LEFT_ANGLE	342
#define	RIGHT_ANGLE	343
#define	VERTICAL_BAR	344
#define	CARET	345
#define	AMPERSAND	346
#define	QUESTION	347

