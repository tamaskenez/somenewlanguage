#!/bin/bash -e

cd $(dirname $0)

mkdir -p b
if [[ ! -d "b/buildaux" ]]; then
    echo -e "-- Cloning [buildaux]: cd b && git clone https://github.com/tamaskenez/buildaux.git buildaux"
    cd b && git clone https://github.com/tamaskenez/buildaux.git
else
    echo -e "\n-- Updating [buildaux]: cd b/buildaux && git pull --ff-only"
    cd b/buildaux
    git pull --ff-only
    cd -
fi

cp b/buildaux/1_bootstrap.sh \
   b/buildaux/clang-format-all.sh \
   b/buildaux/.clang-format \
   .
