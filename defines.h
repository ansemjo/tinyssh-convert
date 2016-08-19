#ifndef _headerguard_defines_h_
#define _headerguard_defines_h_

#define USAGE_MESSAGE \
    "Usage: tinyssh-convert [-f keyfile] [-d destination_dir]\n" \
    "Convert an OpenSSH ed25510 privatekey file to TinySSH\n" \
    "compatible format keys and save them in destination_dir.\n"

/* bytesizes */
#define    Bytes             1
#define  KiBytes    1024*Bytes
#define  MiBytes  1024*KiBytes

#endif