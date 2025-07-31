#!/bin/bash

set -eux

REPO_DIR=$(cd ../../.. ; pwd)
SHAPOFONT_DIR=${REPO_DIR}/../shapofont

CPP_INC_DIR=include/shapofont
CPP_SRC_DIR=src

rm -f ${CPP_INC_DIR}/ShapoSans*.hpp
rm -f ${CPP_SRC_DIR}/ShapoSans*.cpp

mkdir -p ${CPP_INC_DIR}
mkdir -p ${CPP_SRC_DIR}

cp -f ${SHAPOFONT_DIR}/mamefont/cpp/VL/include/ShapoSansP_s12c09a01w02.hpp ${CPP_INC_DIR}/.
cp -f ${SHAPOFONT_DIR}/mamefont/cpp/VL/include/ShapoSansDigitP_s16c14w02.hpp ${CPP_INC_DIR}/.
cp -f ${SHAPOFONT_DIR}/mamefont/cpp/VL/src/ShapoSansP_s12c09a01w02.cpp ${CPP_SRC_DIR}/.
cp -f ${SHAPOFONT_DIR}/mamefont/cpp/VL/src/ShapoSansDigitP_s16c14w02.cpp ${CPP_SRC_DIR}/.