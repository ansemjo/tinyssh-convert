#!/usr/bin/env bash

name=$1

err() { >&2 printf '\e[1mERR!\e[0m  %s\n' "$*"; exit 1; }

gcc -g -o "${name}" *.c || err "compilation failed!"

printf '\e[1m%s\e[0m %s%q\n' 'SUCCESS!' 'Compiled file is saved in: ' "./${name}"
