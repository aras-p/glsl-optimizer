.. _blend:

Blend
=====

This state controls blending of the final fragments into the target rendering
buffers.

Blend Factors
-------------

The blend factors largely follow the same pattern as their counterparts
in other modern and legacy drawing APIs.

XXX blurb about dual-source blends

Members
-------

independent_blend_enable
   If enabled, blend state is different for each render target, and
   for each render target set in the respective member of the rt array.
   If disabled, blend state is the same for all render targets, and only
   the first member of the rt array contains valid data.
logicop_enable
   Enables logic ops. Cannot be enabled at the same time as blending, and
   is always the same for all render targets.
logicop_func
   The logic operation to use if logic ops are enabled. One of PIPE_LOGICOP.
dither
   Whether dithering is enabled.
rt
   Contains the per-rendertarget blend state.

Per-rendertarget Members
------------------------

blend_enable
   If blending is enabled, perform a blend calculation according to blend
   functions and source/destination factors. Otherwise, the incoming fragment
   color gets passed unmodified (but colormask still applies).
rgb_func
   The blend function to use for rgb channels. One of PIPE_BLEND.
rgb_src_factor
   The blend source factor to use for rgb channels. One of PIPE_BLENDFACTOR.
rgb_dst_factor
   The blend destination factor to use for rgb channels. One of PIPE_BLENDFACTOR.
alpha_func
   The blend function to use for the alpha channel. One of PIPE_BLEND.
alpha_src_factor
   The blend source factor to use for the alpha channel. One of PIPE_BLENDFACTOR.
alpha_dst_factor
   The blend destination factor to use for alpha channel. One of PIPE_BLENDFACTOR.
colormask
   Bitmask of which channels to write. Combination of PIPE_MASK bits.
