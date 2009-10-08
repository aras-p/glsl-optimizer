
void
lp_setup_rasterize( struct llvmpipe_context *llvmpipe,
                    struct binned_scene *scene )
{
   lp_rast_bind_surfaces( rast, scene->framebuffer );

   for (i = 0; i < scene->tiles_x; i++) {
      for (j = 0; j < scene->tiles_y; j++) {

         lp_rast_start_tile( rast, i * TILESIZE, j * TILESIZE );

         for (block = scene->tile[i][j].first; block; block = block->next) {
            for (k = 0; k < block->nr_cmds; k++) {
               block->cmd[k].func( rast, block->cmd[k].arg );
            }
         }
      }
   }
}
