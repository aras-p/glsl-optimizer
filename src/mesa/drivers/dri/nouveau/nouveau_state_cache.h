
#ifndef __NOUVEAU_STATE_CACHE_H__
#define __NOUVEAU_STATE_CACHE_H__

#include "mtypes.h"

#define NOUVEAU_STATE_CACHE_ENTRIES 2048

typedef struct nouveau_state_atom_t{
	uint32_t value;
	uint32_t dirty;
}nouveau_state_atom;

typedef struct nouveau_state_cache_t{
	nouveau_state_atom atoms[NOUVEAU_STATE_CACHE_ENTRIES];
	uint32_t current_pos;
	// master dirty flag
	uint32_t dirty;
}nouveau_state_cache;


#endif

