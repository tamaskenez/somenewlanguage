#!/bin/bash -e

cd $(dirname $0)

. b/buildaux/script_lib.sh

git_dep https://github.com/abseil/abseil-cpp.git abseil
git_dep https://github.com/tamaskenez/microlib.git microlib

cmake_dep microlib
