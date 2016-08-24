#!/usr/bin/env bash

#  This file is governed by Licenses which are listed in
#  the LICENSE file, which shall be included in all copies
#  and redistributions of this project.

name=${1:-tinyssh-convert}

err() { >&2 printf '\e[1mERR!\e[0m  %s\n' "$*"; exit 1; }

gcc -g -o "${name}" *.c || err "compilation failed!"

printf '\e[1m%s\e[0m %s%q\n' 'SUCCESS!' 'Compiled file is saved in: ' "./${name}"
