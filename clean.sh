#!/usr/bin/env bash
# ==============================================================================
# This to clean project
# ==============================================================================

RDIR=$(dirname $(readlink -f $0))
TOCLEAN=( "${RDIR}/build" "${RDIR}/bin" )

function clean ()
{
  rm -rvf ${TOCLEAN[@]}

  return $?
}

clean $@

exit $?

