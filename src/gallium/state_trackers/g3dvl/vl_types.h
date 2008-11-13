#ifndef vl_types_h
#define vl_types_h

#if 1 /*#ifdef X11*/
#include <X11/Xlib.h>

typedef Display* vlNativeDisplay;
typedef Drawable vlNativeDrawable;
#endif

struct vlDisplay;
struct vlScreen;
struct vlContext;
struct vlSurface;

enum vlResourceStatus
{
	vlResourceStatusFree,
	vlResourceStatusRendering,
	vlResourceStatusDisplaying
};

enum vlProfile
{
	vlProfileMpeg2Simple,
	vlProfileMpeg2Main,

	vlProfileCount
};

enum vlEntryPoint
{
	vlEntryPointIDCT,
	vlEntryPointMC,
	vlEntryPointCSC,

	vlEntryPointCount
};

enum vlFormat
{
	vlFormatYCbCr420,
	vlFormatYCbCr422,
	vlFormatYCbCr444
};

enum vlPictureType
{
	vlPictureTypeTopField,
	vlPictureTypeBottomField,
	vlPictureTypeFrame
};

enum vlMotionType
{
	vlMotionTypeField,
	vlMotionTypeFrame,
	vlMotionTypeDualPrime,
	vlMotionType16x8
};

enum vlFieldOrder
{
	vlFieldOrderFirst,
	vlFieldOrderSecond
};

enum vlDCTType
{
	vlDCTTypeFrameCoded,
	vlDCTTypeFieldCoded
};

struct vlVertex2f
{
	float x, y;
};

struct vlVertex4f
{
	float x, y, z, w;
};

enum vlMacroBlockType
{
	vlMacroBlockTypeIntra,
	vlMacroBlockTypeFwdPredicted,
	vlMacroBlockTypeBkwdPredicted,
	vlMacroBlockTypeBiPredicted,

	vlNumMacroBlockTypes
};

struct vlMpeg2MacroBlock
{
	unsigned int		mbx, mby;
	enum vlMacroBlockType	mb_type;
	enum vlMotionType	mo_type;
	enum vlDCTType		dct_type;
	int			PMV[2][2][2];
	unsigned int		cbp;
	short			*blocks;
};

struct vlMpeg2MacroBlockBatch
{
	struct vlSurface		*past_surface;
	struct vlSurface		*future_surface;
	enum vlPictureType		picture_type;
	enum vlFieldOrder		field_order;
	unsigned int			num_macroblocks;
	struct vlMpeg2MacroBlock	*macroblocks;
};

#endif
