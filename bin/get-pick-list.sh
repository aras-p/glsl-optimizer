#!/bin/sh

# Script for generating a list of candidates for cherry-picking to a stable branch

git log --reverse --pretty=%H HEAD..origin/master |\
while read sha
do
    # Check to see whether the patch was marked as a candidate for the stable tree.
    if git log -n1 $sha | grep -iq '^[[:space:]]*NOTE: This is a candidate' ; then
	if [ -f .git/cherry-ignore ] ; then
	    if grep -q ^$sha .git/cherry-ignore ; then
		continue
	    fi
	fi

	# Check to see if it has already been picked over.
	if git log origin/master..HEAD | grep -q "cherry picked from commit $sha"; then
	    continue
	fi

	git log -n1 --pretty=oneline $sha | cat
    fi
done
