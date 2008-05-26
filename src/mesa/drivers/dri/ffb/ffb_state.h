
#ifndef _FFB_STATE_H
#define _FFB_STATE_H

extern void ffbDDInitStateFuncs(GLcontext *);
extern void ffbDDInitContextHwState(GLcontext *);

extern void ffbCalcViewport(GLcontext *);
extern void ffbXformAreaPattern(ffbContextPtr, const GLubyte *);
extern void ffbSyncHardware(ffbContextPtr fmesa);

#endif /* !(_FFB_STATE_H) */
