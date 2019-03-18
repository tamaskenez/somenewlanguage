#!/bin/bash -e
# Use $1 to postfix clang-format (e.g. clang-format-aarch64)
git ls-files -- '*.cpp' '*.h' '*.inl' '*.hpp' | xargs clang-format$1 -i -style=file
