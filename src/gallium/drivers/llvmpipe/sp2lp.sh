#!/bin/sh
#
# Port changes from softpipe to llvmpipe. Invoke as
#
#   sp2lp.sh <commit>
#
# Note that this will only affect llvmpipe -- you still need to actually
# cherry-pick/merge the softpipe changes themselves if they affect directories
# outside src/gallium/drivers/softpipe

git format-patch \
	--keep-subject \
	--relative=src/gallium/drivers/softpipe \
	--src-prefix=a/src/gallium/drivers/llvmpipe/ \
	--dst-prefix=b/src/gallium/drivers/llvmpipe/ \
	--stdout "$1^1..$1" \
| sed \
	-e 's/\<softpipe\>/llvmpipe/g' \
	-e 's/\<sp\>/lp/g' \
	-e 's/\<softpipe_/llvmpipe_/g' \
	-e 's/\<sp_/lp_/g' \
	-e 's/\<SP_/LP_/g' \
	-e 's/\<SOFTPIPE_/LLVMPIPE_/g' \
	-e 's/\<spt\>/lpt/g' \
	-e 's/\<sps\>/lps/g' \
	-e 's/\<spfs\>/lpfs/g' \
	-e 's/\<sptex\>/lptex/g' \
	-e 's/\<setup_\(point\|line\|tri\)\>/llvmpipe_\0/g' \
	-e 's/\<llvmpipe_cached_tile\>/llvmpipe_cached_tex_tile/g' \
	-e 's/_get_cached_tile_tex\>/_get_cached_tex_tile/g' \
	-e 's/\<TILE_SIZE\>/TEX_TILE_SIZE/g' \
	-e 's/\<tile_address\>/tex_tile_address/g' \
	-e 's/\<tile->data\.color\>/tile->color/g' \
| patch -p1
