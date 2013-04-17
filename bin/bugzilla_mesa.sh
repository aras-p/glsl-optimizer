#!/bin/bash

# This script is used to generate the list of fixed bugs that
# appears in the release notes files, with HTML formatting.
#
# Note: This script could take a while until all details have
#       been fetched from bugzilla.
#
# Usage examples:
#
# $ bin/bugzilla_mesa.sh mesa-9.0.2..mesa-9.0.3
# $ bin/bugzilla_mesa.sh mesa-9.0.2..mesa-9.0.3 > bugfixes
# $ bin/bugzilla_mesa.sh mesa-9.0.2..mesa-9.0.3 | tee bugfixes
# $ DRYRUN=yes bin/bugzilla_mesa.sh mesa-9.0.2..mesa-9.0.3
# $ DRYRUN=yes bin/bugzilla_mesa.sh mesa-9.0.2..mesa-9.0.3 | wc -l


# regex pattern: trim before url
trim_before='s/.*\(http\)/\1/'

# regex pattern: trim after url
trim_after='s/\(show_bug.cgi?id=[0-9]*\).*/\1/'

# regex pattern: always use https
use_https='s/http:/https:/'

# extract fdo urls from commit log
urls=$(git log $* | grep 'bugs.freedesktop.org/show_bug' | sed -e $trim_before -e $trim_after -e $use_https | sort | uniq)

# if DRYRUN is set to "yes", simply print the URLs and don't fetch the
# details from fdo bugzilla.
#DRYRUN=yes

if [ "x$DRYRUN" = xyes ]; then
	for i in $urls
	do
		echo $i
	done
else
	echo "<ul>"
	echo ""

	for i in $urls
	do
		id=$(echo $i | cut -d'=' -f2)
		summary=$(wget --quiet -O - $i | grep -e '<title>.*</title>' | sed -e 's/ *<title>Bug [0-9]\+ &ndash; \(.*\)<\/title>/\1/')
		echo "<li><a href=\"$i\">Bug $id</a> - $summary</li>"
		echo ""
	done

	echo "</ul>"
fi
