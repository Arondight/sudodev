#!/usr/bin/env bash
# ==============================================================================
# This to build project and do install
# ==============================================================================

RDIR=$(dirname $(readlink -f $0))
BUILD_SH="${RDIR}/build.sh"
BUILD_ARGS="install"

if [[ -x $BUILD_SH ]]
then
  command $BUILD_SH $BUILD_ARGS $@
fi

exit $?

