#
# :-(o)-:
#

all:
	@gcc -o /usr/local/bin/rootfs_fixlinks rootfs_fixlinks.c
	
clean:
	@rm -f /usr/local/bin/rootfs_fixlinks 
