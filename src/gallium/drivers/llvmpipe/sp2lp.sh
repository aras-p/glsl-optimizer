#!/bin/sh
#
# Port changes from softpipe to llvmpipe. Invoke as
#
#   sp2lp.sh <commit>..<commit>
#
# Note that this will only affect llvmpipe -- you still need to actually
# cherry-pick/merge the softpipe changes themselves if they affect directories
# outside src/gallium/drivers/softpipe

git format-patch \
	--keep-subject \
	--relative=src/gallium/drivers/softpipe \
	--src-prefix=a/src/gallium/drivers/llvmpipe/ \
	--dst-prefix=b/src/gallium/drivers/llvmpipe/ \
	--stdout $@ \
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
| git am -3 -k
