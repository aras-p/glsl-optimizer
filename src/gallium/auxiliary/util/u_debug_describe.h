#ifndef U_DEBUG_DESCRIBE_H_
#define U_DEBUG_DESCRIBE_H_

/* a 256-byte buffer is necessary and sufficient */
void debug_describe_reference(char* buf, const struct pipe_reference*ptr);
void debug_describe_resource(char* buf, const struct pipe_resource *ptr);
void debug_describe_surface(char* buf, const struct pipe_surface *ptr);
void debug_describe_sampler_view(char* buf, const struct pipe_sampler_view *ptr);

#endif /* U_DEBUG_DESCRIBE_H_ */
