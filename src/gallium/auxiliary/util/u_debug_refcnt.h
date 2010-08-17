/*
 * u_debug_refcnt.h
 *
 *  Created on: Aug 17, 2010
 *      Author: lb
 */

#ifndef U_DEBUG_REFCNT_H_
#define U_DEBUG_REFCNT_H_

#include <pipe/p_config.h>
#include <pipe/p_state.h>

#if defined(DEBUG) && (!defined(PIPE_OS_WINDOWS) || defined(PIPE_SUBSYSTEM_WINDOWS_USER))
extern int debug_refcnt_state;

void debug_reference_slowpath(const struct pipe_reference* p, void* get_desc, int change);

static INLINE void debug_reference(const struct pipe_reference* p, void* get_desc, int change)
{
	if(debug_refcnt_state >= 0)
		debug_reference_slowpath(p, get_desc, change);
}
#else
static INLINE void debug_reference(const struct pipe_reference* p, void* get_desc, const char* op)
{}
#endif

#endif /* U_DEBUG_REFCNT_H_ */
