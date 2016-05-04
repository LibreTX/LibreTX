#!/bin/bash

# Stops on first error, echo on
set -e
set -x

SRCDIR=$1

COMMON_OPTIONS="-DALLOW_NIGHTLY_BUILDS=YES -DVERSION_SUFFIX=$2"

cmake ${COMMON_OPTIONS} -DPCB=9X ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=GRUVIN9X ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=MEGA2560 ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=SKY9X ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=9XRPRO ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=TARANIS ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=TARANIS -DPCBREV=REVPLUS ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=TARANIS -DPCBREV=REV9E ${SRCDIR}
make libsimulator

cmake ${COMMON_OPTIONS} -DPCB=HORUS ${SRCDIR}
make libsimulator

make package
