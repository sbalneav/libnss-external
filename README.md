# libnss-external
libnss_external is an nss library designed to provide nss services using the
text output of commands.  It currently implements the passwd, group, and shadow
databases for lookup.

Implementation:
---------------

The libnss_external library runs a popen(3) on the external commands provided,
and then parses the result to provide to gnu libc's NSS mechanism.

Building:
---------

Untar the nss_external-1.0.tar.gz file.

cd nss_external-1.0
./configure --prefix=/usr
make
sudo make install

Installation:
-------------

The building phase should put the library in /usr/lib.  To use nss_external,
edit your /etc/nsswitch.conf file as follows:

....
passwd:         compat external
group:          compat external
shadow:         compat external
....

Create a directory /etc/nss-external.  In this directory, either place the 3
programs (named "passwd", "group", and "shadow") for the three databases, or
better still, simply install your program somewhere better suited in the
filesystem, and provide 3 symbolic links.

Example:
--------

Let's say you'd like to provide users with passwd and group entries from
another machine via SSH, provided they have a master socket to the host in
their home directory.

Place the following short shell script in /usr/share/nss-external-ssh, and call
it "sshnss":

------------------------------------------------------------------------------
#!/bin/sh

NSSSOCKET=$HOME/.nsssocket
NSSSERVER=my.hostname
DB=$(basename $0)

test -S $NSSSOCKET || exit 0

ssh -S $NSSSOCKET $NSSSERVER getent $DB $@
------------------------------------------------------------------------------

Install the symbolic links (as root) in /etc/nss-external:

ln -s /usr/share/nss-external-ssh/sshnss /etc/nss-external/passwd
ln -s /usr/share/nss-external-ssh/sshnss /etc/nss-external/group
ln -s /usr/share/nss-external-ssh/sshnss /etc/nss-external/shadow

Make sure you, as a user, have esablished a remote ssh master socket, like
so:

ssh -M -S $HOME/.nsssocket my.hostname

if everything's working alright, you should be able to execute a:

getent passwd

or:

getent group

and see passwd and group entries from the remote system.  If the socket
goes away for some reason nss_external doesn't do anything.

Modifying:
----------

Currently, nss_external doesn't pull across any user or group id's less
than MINUID or MINGID, both of which are set to 500.  If for some reason
you need to modify this, change it in nss_external.h, in the src directory.
