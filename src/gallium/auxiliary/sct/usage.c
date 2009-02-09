/* surface / context tracking */


/*

context A:
  render to texture T

context B:
  texture from T

-----------------------

flush surface:
  which contexts are bound to the surface?

-----------------------

glTexSubImage():
  which contexts need to be flushed?

 */


/*

in MakeCurrent():

  call sct_bind_surfaces(context, list of surfaces) to update the
  dependencies between context and surfaces


in SurfaceFlush(), or whatever it is in D3D:

  call sct_get_surface_contexts(surface) to get a list of contexts
  which are currently bound to the surface.



in BindTexture():

  call sct_bind_texture(context, texture) to indicate that the texture
  is used in the scene.


in glTexSubImage() or RenderToTexture():

  call sct_is_texture_used(context, texture) to determine if the texture
  has been used in the scene, but the scene's not flushed.  If TRUE is
  returned it means the scene has to be rendered/flushed before the contents
  of the texture can be changed.


in psb_scene_flush/terminate():

  call sct_flush_textures(context) to tell the SCT that the textures which
  were used in the scene can be released.



*/
