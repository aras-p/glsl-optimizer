#! /bin/sh
git rm -rf $(git status --porcelain | awk '/^DU/ {print $NF}')
