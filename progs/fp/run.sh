#!/bin/sh

for i in *.txt ; do
echo $i
./fp-tri $i
done

