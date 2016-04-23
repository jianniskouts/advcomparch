#!/bin/bash

## This script is only intended to be executed inside the PARSEC main dir
if [ ! -f .parsec_uniquefile ]; then
	echo "Please run this script inside the PARSEC main dir."
	exit 1
fi

## Add hooks in gcc-serial config files
for configFile in `ls config/gcc-serial.bldconf pkgs/{apps,kernels}/*/parsec/gcc-serial.bldconf`; do
	benchName=$(echo $configFile | cut -d/ -f3)
	echo "==================== $benchName ===================="

	oldTXT="source \${PARSECDIR}/gcc-serial.bldconf/gcc-hooks.bldconf/)"
	hooksTXT="source \${PARSECDIR}/$(echo $configFile | sed s/gcc-serial.bldconf/gcc-hooks.bldconf/)"
	alreadyDone=$(grep "$hooksTXT" "$configFile" | wc -l)

	if [ "x$alreadyDone" == "x0" ]; then
		sed -i.old s/gcc.bldconf/gcc-hooks.bldconf/ $configFile
	else
		echo "Nothing to be done"
	fi
	echo "===================================================="
done

## In Ferret there is a spelling mistake:
## __parsec_roi_begins() instead of __parsec_roi_begin() and
## __parsec_roi_ends()   instead of __parsec_roi_end()
## Let's fix it now!
sed -i.old s/__parsec_roi_begins/__parsec_roi_begin/ pkgs/apps/ferret/src/benchmark/ferret-serial.c
sed -i.old s/__parsec_roi_ends/__parsec_roi_end/ pkgs/apps/ferret/src/benchmark/ferret-serial.c
