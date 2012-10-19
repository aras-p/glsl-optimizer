#!/bin/sh

# Script for generating a list of candidates for cherry-picking to a stable branch

# Grep for commits that were marked as a candidate for the stable tree.
git log --reverse --pretty=%H -i --grep='^[[:space:]]*NOTE: This is a candidate' HEAD..origin/master |\
while read sha
do
    # Check to see whether the patch is on the ignore list.
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
done
