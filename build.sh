#!/usr/bin/env bash

name="tinyssh-convert"
builddir="./openssh-portable"
gitrepo="https://github.com/openssh/openssh-portable.git"

makefile="${builddir}/Makefile"

err() { >&2 printf '\e[1mERR!\e[0m  %s\n' "$*"; exit 1; }

# get openssh-portable from GitHub
mkdir ${builddir} || err "Could not create build directory!"
git clone --depth=5 "${gitrepo}" "${builddir}" || err "Could not clone the repository."

# copy tinyssh-convert source to builddir
cp "./${name}.c" "${builddir}" || err "Failed to copy source into build directory."

# insert lines in Makefile.in
if ( ! grep "${name}" "${makefile}.in" >/dev/null ); then
  search="ssh-keygen"
  sed -e '/./{H;$!d;}' -e "x;/${search}\.o/!d;" -e "s/${search}/${name}/g" \
    "${makefile}.in" >> "${makefile}.in" || err "Could not insert entry in Makefile template."
fi

# run configures & build
oldpwd=$(pwd)
cd "${builddir}"

autoreconf || err "'autoreconf' failed."
./configure --without-openssl || err "Configuration failed."
make "${name}" || err "Compilation failed!"

# copy compiled file to original directory
cp "${name}" "${oldpwd}/" || err "Failed to copy compiled file back to original directory."

printf '\e[1m%s\e[0m %s%q\n' 'SUCCESS!' 'Compiled file is saved in: ' "./${name}"
