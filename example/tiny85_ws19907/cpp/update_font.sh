#!/bin/bash

set -eux

REPO_DIR=$(cd ../../.. ; pwd)
SHAPOFONT_DIR=${REPO_DIR}/../shapofont

CPP_INC_DIR=include/font
CPP_SRC_DIR=src/font

rm -f ${CPP_INC_DIR}/*.hpp ${CPP_SRC_DIR}/*.cpp

mkdir -p ${CPP_INC_DIR}
mkdir -p ${CPP_SRC_DIR}

cp -f ${SHAPOFONT_DIR}/mamefont/cpp/HL/include/MameSansP_s48c40w08.hpp ${CPP_INC_DIR}/.
cp -f ${SHAPOFONT_DIR}/mamefont/cpp/HL/src/MameSansP_s48c40w08.cpp ${CPP_SRC_DIR}/.
