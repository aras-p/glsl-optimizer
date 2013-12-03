#!/bin/sh

echo ""

THISDIR=$(dirname $0)
test "${THISDIR}" = "." && THISDIR="${PWD}"

USABLE_GYP="none"

# Do we need a new gyp?
if which gyp > /dev/null 2>&1 ; then
  USABLE_GYP="$(which gyp)"
  echo -n -e "Good, found gyp, testing if it's got --generator-output-flat option .. "
  "${USABLE_GYP}" --help | grep generator-output-flat > /dev/null 2>&1
  if [ ! $? = 0 ] ; then
    echo -e "no\n"
    echo "You have gyp installed, but it's not got my patches, do you want me to"
    echo "download, patch and install my version? These patches add a new option"
    echo "so that Project files can be put in a single, flat directory structure"
    echo "and a new Conditional (config_conditions) such that configurations for"
    echo "more than one Arch can exist in the same generated project file (Win32"
    echo "and x64 in Visual Studio projects for example.)"
    echo ""
    while true; do
        read -p "Do you wish to install my modified gyp? [YN]: " yn
        case $yn in
            [Yy]* ) USABLE_GYP="none"; break;;
            [Nn]* ) echo -e "\nOK then, bye!"; exit 1;;
            * ) echo "Please answer yes or no.";;
        esac
    done
  else
    echo "yes"
  fi
fi

if [ "${USABLE_GYP}" = "none" ] ; then
  if which python2 > /dev/null 2>&1 ; then
    PYTHON=python2
  elif which python > /dev/null 2>&1 ; then
    PYTHON=python
  else
    echo "I couldn't find Python? If you are running on"
    echo "Windows I recommend you use MSYS2 with Pacman"
    exit 2
  fi
  pushd /tmp
  curl -O http://python-distribute.org/distribute_setup.py
  ${PYTHON} distribute_setup.py
  [ -d gyp ] && rm -rf gyp
  git clone https://chromium.googlesource.com/experimental/external/gyp
  pushd gyp
  patch -p1 < ${THISDIR}/gyp-patches/0001-Add-config_conditions-as-PHASE_LATELATE-conditions.patch
  patch -p1 < ${THISDIR}/gyp-patches/0002-Add-generator-output-flat-option.patch
  ${PYTHON} ./setup.py install
  popd
  popd
fi

rm -rf projects/vs2008; GYP_MSVS_VERSION=2008 gyp -d=all --depth=../src/ --format=msvs --generator-output=projects/vs2008 --generator-output-flat ; echo "Generated projects/vs2008, contents:" ; ls -l projects/vs2008
rm -rf projects/vs2010; GYP_MSVS_VERSION=2010 gyp -d=all --depth=../src/ --format=msvs --generator-output=projects/vs2010 --generator-output-flat ; echo "Generated projects/vs2010, contents:" ; ls -l projects/vs2010
rm -rf projects/vs2012; GYP_MSVS_VERSION=2012 gyp -d=all --depth=../src/ --format=msvs --generator-output=projects/vs2012 --generator-output-flat ; echo "Generated projects/vs2012, contents:" ; ls -l projects/vs2012
rm -rf projects/xcode5; gyp -d=all --depth=../src/ --format=xcode --generator-output=projects/xcode5 --generator-output-flat ; echo "Generated projects/xcode5, contents:" ; ls -l projects/xcode5

echo "All done!"
if [ "$OSTYPE" = "msys" ]; then
  echo "You can build all of the Windows projects from this shell via 'cmd.exe /c buildMyProjects.bat'"
fi

exit 0
