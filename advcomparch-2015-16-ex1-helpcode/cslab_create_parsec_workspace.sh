#!/bin/bash

## This script is only intended to be executed inside the PARSEC main dir
if [ ! -f .parsec_uniquefile ]; then
	echo "Please run this script inside the PARSEC main dir."
	exit 1
fi

## You can change this if you want.
WORKSPACE_DIR=./parsec_workspace

## Get executables first
mkdir -p ${WORKSPACE_DIR}/executables
cp pkgs/{apps,kernels}/*/inst/amd64-linux.gcc-serial/bin/* ${WORKSPACE_DIR}/executables/

## Get inputs next
mkdir -p ${WORKSPACE_DIR}/inputs
for benchDir in `ls -d pkgs/{apps,kernels}/*`; do
	benchName=$(echo $benchDir | cut -d/ -f3)
	echo $benchName
	cp $benchDir/inputs/input_simlarge.tar ${WORKSPACE_DIR}/inputs/$benchName-input_simlarge.tar

	WD=$(pwd)
	cd ${WORKSPACE_DIR}/inputs/
	tar xvf $benchName-input_simlarge.tar
	cd $WD

	rm ${WORKSPACE_DIR}/inputs/$benchName-input_simlarge.tar
done

## Facesim works on "./Face_Data" so we need to create a soft link
ln -s inputs/Face_Data ${WORKSPACE_DIR}
