#!/bin/bash

set -eux

REPO_DIR=../..
SHAPOFONT_DIR=${REPO_DIR}/../shapofont

CPP_INC_DIR=${REPO_DIR}/example/fonts/cpp/include/shapofont
CPP_SRC_DIR=${REPO_DIR}/example/fonts/cpp/src

rm -f ${CPP_INC_DIR}/ShapoSans*.hpp
rm -f ${CPP_SRC_DIR}/ShapoSans*.cpp

mkdir -p ${CPP_INC_DIR}
mkdir -p ${CPP_SRC_DIR}

cp -f ${SHAPOFONT_DIR}/mamefont/cpp/include/shapofont/ShapoSans*.hpp ${CPP_INC_DIR}/.
cp -f ${SHAPOFONT_DIR}/mamefont/cpp/src/ShapoSans*.cpp ${CPP_SRC_DIR}/.
