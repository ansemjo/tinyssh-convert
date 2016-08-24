#!/usr/bin/env bash

#  This file is governed by Licenses which are listed in
#  the LICENSE file, which shall be included in all copies
#  and redistributions of this project.

name=${1:-tinyssh-convert}

say() { X=$1; shift; printf '\e[1m%s\e[0m  ' "$X"; printf "$@"; printf '\n'; }
err() { >&2 say 'ERR!' "$1"; exit 1; }

gcc -g -o "${name}" *.c || err "compilation failed!"

say 'SUCCESS!' 'Compiled file is saved in: %s' "./${name}"