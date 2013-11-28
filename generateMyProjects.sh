#!/bin/sh

rm -rf projects/vs2008; GYP_MSVS_VERSION=2008 gyp -d=all --depth=../src/ --format=msvs --generator-output=projects/vs2008 --generator-output-flat ; echo "projects/vs2008 contains:" ; ls -l projects/vs2008
rm -rf projects/vs2010; GYP_MSVS_VERSION=2010 gyp -d=all --depth=../src/ --format=msvs --generator-output=projects/vs2010 --generator-output-flat ; echo "projects/vs2010 contains:" ; ls -l projects/vs2010
rm -rf projects/vs2012; GYP_MSVS_VERSION=2012 gyp -d=all --depth=../src/ --format=msvs --generator-output=projects/vs2012 --generator-output-flat ; echo "projects/vs2012 contains:" ; ls -l projects/vs2012
rm -rf projects/xcode5; gyp -d=all --depth=../src/ --format=xcode --generator-output=projects/xcode5 --generator-output-flat ; echo "projects/xcode5 contains:" ; ls -l projects/xcode5
