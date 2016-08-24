# Description

This is a small program which converts existing [ed25519] keys from [OpenSSH]
format to the [TinySSH] format.

# Usage scenario

The conversion enables OpenSSH and TinySSH to run with an identical set of keys,
thus presenting an identical fingerprint upon connection. This avoids warnings
of changed hostkeys when connecting to the same IP.

A possible usage scenario would be a server with an encrypted root device which
needs to be [unlocked remotely]. The initramfs can run a copy of TinySSH and
provide means to enter a passphrase remotely via ssh.

[ed25519]: https://ed25519.cr.yp.to/
[OpenSSH]: http://www.openssh.com/
[TinySSH]: https://tinyssh.org/
[unlocked remotely]: https://wiki.archlinux.org/index.php/Dm-crypt/Specialties#Remote_unlocking_.28hooks:_netconf.2C_dropbear.2C_tinyssh.2C_ppp.29

# Building from source

A script `build.sh` is provided to automate these steps. Simply run it with
`./build.sh` and the binary will be compiled in this folder.

To do it manually:
* run `autoreconf --force --verbose --install` to create the 'configure' script
* configure with `./configure` and any additional flags you want ..
* build with simply `make`

Afterwards you can install the binary with e.g. `sudo make install`.

# Usage of the binary

`$ ./tinyssh-convert [-hv] [-f keyfile] [-d destination_dir]`

The program can be run entirely interactively or both required paths can be
given on the commandline to make it scriptable.

The __keyfile__ shall be an ed25519 private key in OpenSSH format. The
__destination_dir__ is a directory where the converted files will be dropped.

The option `-h` displays help and `-v` shows the current version.