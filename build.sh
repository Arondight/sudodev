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
    && cmake .. -DCMAKE_BUILD_TYPE=Release $@ \
    && cmake --build . --config Release -- -j4

  return $?
}

build $@

exit $?

