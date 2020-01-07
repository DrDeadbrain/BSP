#!/bin/sh
module="ceasar"
device="ceasar"
mode="664" # -rw-rw-r-- 

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod ./src/$module.ko $* || exit 1

# remove stale nodes
rm -f /dev/${device}e   # Encode
rm -f /dev/${device}d   # decode 

# $2 Zweite spalte == modulname. Printen der Ersten Spalte (major Num)
major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices) 


mknod /dev/${device}e c $major 0 # create node Encode as minnum 0
mknod /dev/${device}d c $major 1 # create node decode as minnum 1


# give appropriate group/permissions, and change the group.
# Not all distributions have staff, some have "wheel" instead.
group="users"
#grep -q '^staff:' /etc/group || group="users"

# gruppe ändern
chgrp $group /dev/${device}e
chgrp $group /dev/${device}d

# Zugreifbar machen
chmod $mode  /dev/${device}e    
chmod $mode  /dev/${device}d
 
