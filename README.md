## What you get
A simple C source to fix all absolute soft links to relative soft links inside a root file system on build host machine.

### For example
```
Origin 
/etc/modprobe.d/blacklist-oss.conf linked to /lib/linux-sound-base/noOSS.modprobe.conf

After fix 
/etc/modprobe.d/blacklist-oss.conf linked to ../../lib/linux-sound-base/noOSS.modprobe.conf
```

## Build
Type: 
gcc -o /usr/local/bin/rootfs_fixlinks rootfs_fixlinks.c 

## Tested
On Ubuntu 18.04.5 LTS (Bionic Beaver)
