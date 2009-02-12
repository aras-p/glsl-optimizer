#!/bin/sh

for i in *.txt ; do
echo $i
./vp-tris $i
done

