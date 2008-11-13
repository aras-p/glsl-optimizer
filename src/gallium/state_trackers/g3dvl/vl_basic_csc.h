#ifndef vl_basic_csc_h
#define vl_basic_csc_h

struct pipe_context;
struct vlCSC;

int vlCreateBasicCSC
(
	struct pipe_context *pipe,
	struct vlCSC **csc
);

#endif
