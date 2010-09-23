#!/bin/bash
for i in "$@"; do
	n=$(basename "$i" .txt|sed -e 's/s$//')
	echo "const char* tpf_${n}_names[] ="
	echo "{"
	while read j; do
		echo $'\t'"\"$j\"",
	done < "$i"
	echo "};"
	echo
done
