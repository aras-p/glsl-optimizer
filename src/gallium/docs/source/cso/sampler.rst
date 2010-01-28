.. _sampler:

Sampler
=======

Texture units have many options for selecting texels from loaded textures;
this state controls an individual texture unit's texel-sampling settings.

Texture coordinates are always treated as four-dimensional, and referred to
with the traditional (S, T, R, Q) notation.

Members
-------

wrap_s
    How to wrap the S coordinate. One of PIPE_TEX_WRAP.
wrap_t
    How to wrap the T coordinate. One of PIPE_TEX_WRAP.
wrap_r
    How to wrap the R coordinate. One of PIPE_TEX_WRAP.
min_img_filter
    The filter to use when minifying texels. One of PIPE_TEX_FILTER.
min_mip_filter
    The filter to use when minifying mipmapped textures. One of
    PIPE_TEX_FILTER.
mag_img_filter
    The filter to use when magnifying texels. One of PIPE_TEX_FILTER.
compare_mode
    If set to PIPE_TEX_COMPARE_R_TO_TEXTURE, texture output is computed
    according to compare_func, using r coord and the texture value as operands.
    If set to PIPE_TEX_COMPARE_NONE, no comparison calculation is performed.
compare_func
    How the comparison is computed. One of PIPE_FUNC.
normalized_coords
    Whether the texture coordinates are normalized. If normalized, they will
    always be in [0, 1]. If not, they will be in the range of each dimension
    of the loaded texture.
prefilter
    Cylindrical texcoord wrap enable per coord. Not exposed by most APIs.
lod_bias
    The bias to apply to the level of detail.
min_lod
    Minimum level of detail, used to clamp LoD after bias.
max_lod
    Maximum level of detail, used to clamp LoD after bias.
border_color
    RGBA color used for out-of-bounds coordinates.
max_anisotropy
    Maximum filtering to apply anisotropically to textures. Setting this to
    1.0 effectively disables anisotropic filtering.
