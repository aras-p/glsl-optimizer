#ifndef COMMON_CMDBUF_H
#define COMMON_CMDBUF_H

void rcommonEnsureCmdBufSpace(radeonContextPtr rmesa, int dwords, const char *caller);
int rcommonFlushCmdBuf(radeonContextPtr rmesa, const char *caller);
int rcommonFlushCmdBufLocked(radeonContextPtr rmesa, const char *caller);
void rcommonInitCmdBuf(radeonContextPtr rmesa, int max_state_size);
void rcommonDestroyCmdBuf(radeonContextPtr rmesa);

void rcommonBeginBatch(radeonContextPtr rmesa,
		       int n,
		       int dostate,
		       const char *file,
		       const char *function,
		       int line);


/**
 * Every function writing to the command buffer needs to declare this
 * to get the necessary local variables.
 */
#define BATCH_LOCALS(rmesa) \
	const radeonContextPtr b_l_rmesa = rmesa

/**
 * Prepare writing n dwords to the command buffer,
 * including producing any necessary state emits on buffer wraparound.
 */
#define BEGIN_BATCH(n) rcommonBeginBatch(b_l_rmesa, n, 1, __FILE__, __FUNCTION__, __LINE__)

/**
 * Same as BEGIN_BATCH, but do not cause automatic state emits.
 */
#define BEGIN_BATCH_NO_AUTOSTATE(n) rcommonBeginBatch(b_l_r300, n, 0, __FILE__, __FUNCTION__, __LINE__)

/**
 * Write one dword to the command buffer.
 */
#define OUT_BATCH(data) \
	do { \
        radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, data);\
	} while(0)

/**
 * Write a relocated dword to the command buffer.
 */
#define OUT_BATCH_RELOC(data, bo, offset, rd, wd, flags) \
	do { \
        if (offset) {\
            fprintf(stderr, "(%s:%s:%d) offset : %d\n",\
            __FILE__, __FUNCTION__, __LINE__, offset);\
        }\
        radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, offset);\
        radeon_cs_write_reloc(b_l_rmesa->cmdbuf.cs, \
                              bo, \
                              rd, \
                              wd, \
                              flags);\
	} while(0)


/**
 * Write n dwords from ptr to the command buffer.
 */
#define OUT_BATCH_TABLE(ptr,n) \
	do { \
		int _i; \
        for (_i=0; _i < n; _i++) {\
            radeon_cs_write_dword(b_l_rmesa->cmdbuf.cs, ptr[_i]);\
        }\
	} while(0)

/**
 * Finish writing dwords to the command buffer.
 * The number of (direct or indirect) OUT_BATCH calls between the previous
 * BEGIN_BATCH and END_BATCH must match the number specified at BEGIN_BATCH time.
 */
#define END_BATCH() \
	do { \
        radeon_cs_end(b_l_rmesa->cmdbuf.cs, __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

/**
 * After the last END_BATCH() of rendering, this indicates that flushing
 * the command buffer now is okay.
 */
#define COMMIT_BATCH() \
	do { \
	} while(0)



#endif
