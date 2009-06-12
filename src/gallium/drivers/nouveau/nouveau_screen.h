#ifndef __NOUVEAU_SCREEN_H__
#define __NOUVEAU_SCREEN_H__

struct nouveau_screen {
	struct pipe_screen base;
	struct nouveau_device *device;
	struct nouveau_channel *channel;
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

#endif
