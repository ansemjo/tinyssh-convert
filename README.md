# Description

This is a small program which converts existing [ed25519] keys from [OpenSSH]
format to the [TinySSH] format. It is built against a copy of [openssh-portable].

# Usage scenario

The conversion enables OpenSSH and TinySSH to run with an identical set of keys,
thus presenting an identical fingerprint upon connection. This avoids warnings
of changed hostkeys when connecting to the same IP.

A possible usage scenario would be a server with an encrypted root device which
needs to be [unlocked remotely]. The initramfs can run a copy of TinySSH and
provide means to enter a passphrase remotely via ssh.

[ed25519]: https://ed25519.cr.yp.to/
[OpenSSH]: http://www.openssh.com/
[openssh-portable]: https://github.com/openssh/openssh-portable
[TinySSH]: https://tinyssh.org/
[unlocked remotely]: https://wiki.archlinux.org/index.php/Dm-crypt/Specialties#Remote_unlocking_.28hooks:_netconf.2C_dropbear.2C_tinyssh.2C_ppp.29

# Building from source

The binary is built using the sources of [openssh-portable]. This means that
this program's source is rather small but it also means that the finished binary
has a lot of unused clutter from libssh.

A script `build.sh` is provided to automate these steps. Simply run it with
`./build.sh` and the compiled file will be copied to this folder.

To do it manually:
* clone [github.com/openssh/openssh-portable][openssh-portable]
* copy the source `tinyssh-convert.c` into the cloned copy's root
* duplicate and edit the entry for `ssh-keygen` in the `Makefile.in` to make a
    build target
* configure: `autoreconf && ./configure --without-openssl`
* build: `make tinyssh-convert`

# Usage of the binary

`$ ./tinyssh-convert [-f keyfile] [-d destination_dir]`

The program can be run entirely interactively or both required paths can be
given on the commandline to make it scriptable.

The __keyfile__ shall be an ed25519 private key in OpenSSH format. The
__destination_dir__ is a directory where the converted files will be dropped.



# Notes

All development happened against commit [f217d9b] of [openssh-portable].
[f217d9b]: https://github.com/openssh/openssh-portable/commit/f217d9bd42d306f69f56335231036b44502d8191