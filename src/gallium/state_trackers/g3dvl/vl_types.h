#ifndef vl_types_h
#define vl_types_h

enum VL_FORMAT
{
	VL_FORMAT_YCBCR_420,
	VL_FORMAT_YCBCR_422,
	VL_FORMAT_YCBCR_444
};

enum VL_PICTURE
{
	VL_TOP_FIELD,
	VL_BOTTOM_FIELD,
	VL_FRAME_PICTURE
};

enum VL_FIELD_ORDER
{
	VL_FIELD_FIRST,
	VL_FIELD_SECOND
};

enum VL_DCT_TYPE
{
	VL_DCT_FIELD_CODED,
	VL_DCT_FRAME_CODED
};

enum VL_SAMPLE_TYPE
{
	VL_FULL_SAMPLE,
	VL_DIFFERENCE_SAMPLE
};

enum VL_MC_TYPE
{
	VL_FIELD_MC,
	VL_FRAME_MC,
	VL_DUAL_PRIME_MC,
	VL_16x8_MC = VL_FRAME_MC
};

struct VL_VERTEX4F
{
	float x, y, z, w;
};

struct VL_VERTEX2F
{
	float x, y;
};

struct VL_TEXCOORD2F
{
	float s, t;
};

struct VL_MC_VS_CONSTS
{
	struct VL_VERTEX4F	scale;
	struct VL_VERTEX4F	mb_pos_trans;
	struct VL_VERTEX4F	denorm;
	struct
	{
		struct VL_VERTEX4F	top_field;
		struct VL_VERTEX4F	bottom_field;
	} mb_tc_trans[2];
};

struct VL_MC_FS_CONSTS
{
	struct VL_VERTEX4F	multiplier;
	struct VL_VERTEX4F	bias;
	struct VL_VERTEX4F	y_divider;
};

struct VL_CSC_FS_CONSTS
{
	struct VL_VERTEX4F	bias;
	float			matrix[16];
};

struct VL_MOTION_VECTOR
{
	struct
	{
		int x, y;
	} top_field, bottom_field;
};

struct VL_CONTEXT;
struct VL_SURFACE;

#endif

