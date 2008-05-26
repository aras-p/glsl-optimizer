
#include "../../X86/assyntax.h"

#define SETUP_RGBA  0x1
#define SETUP_TMU0  0x2
#define SETUP_TMU1  0x4


/* Pack either rgba or texture into the remaining half of a 32 byte vertex.
 */
#define CLIP_R  24
#define CLIP_G  16
#define CLIP_B  20
#define CLIP_A  28                      /*  defined inf fxdrv.h              */

#define CLIP_S0 16
#define CLIP_T0 20
#define CLIP_S1 24
#define CLIP_T1 28

#define SIZE 4
#define TYPE (0)
#define TAG(x) x
#include "fx_3dnow_fasttmp.h"

#define SIZE 8
#define TYPE (SETUP_RGBA)
#define TAG(x) x##_RGBA
#include "fx_3dnow_fasttmp.h"

#define SIZE 6
#define TYPE (SETUP_TMU0)
#define TAG(x) x##_TMU0
#include "fx_3dnow_fasttmp.h"

#define SIZE 8
#define TYPE (SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_TMU0_TMU1
#include "fx_3dnow_fasttmp.h"

#undef CLIP_S1
#undef CLIP_T1
#define CLIP_S1 16
#define CLIP_T1 20

#define SIZE 6
#define TYPE (SETUP_TMU1)
#define TAG(x) x##_TMU1
#include "fx_3dnow_fasttmp.h"

/* These three need to use a full 64 byte clip-space vertex.
 */
#undef CLIP_S0
#undef CLIP_T0
#undef CLIP_S1
#undef CLIP_T1

#define CLIP_S0 32
#define CLIP_T0 36
#define CLIP_S1 40
#define CLIP_T1 44

#define SIZE 10
#define TYPE (SETUP_RGBA|SETUP_TMU0)
#define TAG(x) x##_RGBA_TMU0
#include "fx_3dnow_fasttmp.h"

#define SIZE 12
#define TYPE (SETUP_RGBA|SETUP_TMU0|SETUP_TMU1)
#define TAG(x) x##_RGBA_TMU0_TMU1
#include "fx_3dnow_fasttmp.h"

#undef CLIP_S1
#undef CLIP_T1
#define CLIP_S1 32
#define CLIP_T1 36

#define SIZE 10
#define TYPE (SETUP_RGBA|SETUP_TMU1)
#define TAG(x) x##_RGBA_TMU1
#include "fx_3dnow_fasttmp.h"


