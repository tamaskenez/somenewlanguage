#!/bin/bash -e

cd $(dirname $0)

. d/buildaux/script_lib.sh

git_dep https://github.com/abseil/abseil-cpp.git abseil
git_dep https://github.com/tamaskenez/microlib.git microlib r1
git_dep https://github.com/fmtlib/fmt.git fmt

cmake_dep microlib --try-use-ide
cmake_dep abseil
cmake_dep fmt -DFMT_TEST=0 -DFMT_DOC=0
