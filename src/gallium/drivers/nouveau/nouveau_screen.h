#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

struct nouveau_screen {
	struct pipe_screen base;
	struct nouveau_device *device;
	struct nouveau_channel *channel;

        /**
         * Create a new texture object, using the given template info, but on top of
         * existing memory.
         * 
         * It is assumed that the buffer data is layed out according to the expected
         * by the hardware. NULL will be returned if any inconsistency is found.  
         */
        struct pipe_texture * (*texture_blanket)(struct pipe_screen *,
                                                 const struct pipe_texture *templat,
                                                 const unsigned *stride,
                                                 struct pipe_buffer *buffer);

	int (*pre_pipebuffer_map_callback) (struct pipe_screen *pscreen,
		struct pipe_buffer *pb, unsigned usage);
};

static inline struct nouveau_screen *
nouveau_screen(struct pipe_screen *pscreen)
{
	return (struct nouveau_screen *)pscreen;
}

static inline struct nouveau_bo *
nouveau_bo(struct pipe_buffer *pb)
{
	return pb ? *(struct nouveau_bo **)(pb + 1) : NULL;
}

int nouveau_screen_init(struct nouveau_screen *, struct nouveau_device *);
void nouveau_screen_fini(struct nouveau_screen *);

struct nouveau_miptree {
	struct pipe_texture base;
	struct nouveau_bo *bo;
};

static inline struct nouveau_miptree *
nouveau_miptree(struct pipe_texture *pt)
{
	return (struct nouveau_miptree *)pt;
}

#endif
