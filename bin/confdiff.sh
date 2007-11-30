#!/bin/bash -e

usage()
{
	echo "Usage: $0 <target1> <target2>"
	echo "Highlight differences between Mesa configs"
	echo "Example:"
	echo "  $0 linux linux-x86"
}

die()
{
	echo "$@" >&2
	return 1
}

case "$1" in
-h|--help) usage; exit 0;;
esac

[ $# -lt 2 ] && die 2 targets needed. See $0 --help
target1=$1
target2=$2

topdir=$(cd "`dirname $0`"/..; pwd)
cd "$topdir"

[ -f "./configs/$target1" ] || die Missing configs/$target1
[ -f "./configs/$target2" ] || die Missing configs/$target2

trap 'rm -f "$t1" "$t2"' 0

t1=$(mktemp)
t2=$(mktemp)

make -f- -n -p <<EOF | sed '/^# Not a target/,/^$/d' > $t1
TOP = .
include \$(TOP)/configs/$target1
default:
EOF

make -f- -n -p <<EOF | sed '/^# Not a target/,/^$/d' > $t2
TOP = .
include \$(TOP)/configs/$target2
default:
EOF

diff -pu -I'^#' $t1 $t2
