
tile =  [[0,1,4,5],
	 [2,3,6,7],
	 [8,9,12,13],
	 [10,11,14,15]]
shift = 0
align = 1
value = 0L
holder = []

import sys

basemask = [0x
fd = sys.stdout
indent = " "*9
for c in range(4):
   fd.write(indent + "*pdst++ = \n");
   for l,line in enumerate(tile):
	fd.write(indent + "   %s_mm_shuffle_epi8(line%d, (__m128i){"%(l and '+' or ' ',l))
	for i,pos in enumerate(line):
	    mask = 0x00ffffffff & (~(0xffL << shift))
	    value = mask | ((pos) << shift)
	    holder.append(value)
            if holder and (i + 1) %2 == 0:
		fd.write("0x%8.0x"%(holder[0] + (holder[1] << 32)))
		holder = []
		if (i) %4 == 1:
			fd.write( ',')
	        
        fd.write("})%s\n"%((l == 3) and ';' or ''))
   print
   shift += 8
