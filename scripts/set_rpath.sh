#!/bin/bash

if [[ -z "$1" ]] ; then
	RPATH="\$ORIGIN:\$ORIGIN/bin:\$ORIGIN/qt/lib"
else
	RPATH=$1
fi

function doPatchElf {
	echo "patchelf --set-rpath \"${RPATH}\" \"${1}\""
	patchelf --set-rpath "${RPATH}" "${1}"
}

for f in $(find ./ -maxdepth 1 -name "*.so"); do
	doPatchElf ${f}
done

for f in $(find ./bin/ -maxdepth 1 -name "*.so"); do
	doPatchElf ${f}
done


doPatchElf ./bin/visus
doPatchElf ./bin/visusviewer 


