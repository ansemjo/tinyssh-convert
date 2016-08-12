#!/usr/bin/env bash

# insert lines in Makefile
sed -e '/./{H;$!d;}' -e 'x;/ssh-keygen\.o/!d;' -e 's/ssh-keygen/tinyssh-convert/g' Makefile
