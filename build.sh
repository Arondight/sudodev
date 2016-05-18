#!/usr/bin/env bash
# ==============================================================================
# This to build project
# ==============================================================================

RDIR=$(dirname $(readlink -f $0))
BUILDDIR=${RDIR}/build

function build ()
{
  mkdir -p $BUILDDIR  \
    && cd $BUILDDIR \
    && cmake -DCMAKE_BUILD_TYPE=Release ..  \
    && cmake --build . --config Release -- -j4

  return $?
}

function install ()
{
  cd $BUILDDIR  \
    && sudo make install -j4

  return $?
}

build

if [[ 0 -eq $? && 'install' == $1 ]]
then
  install
fi

exit $?

