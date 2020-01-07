 #!/bin/sh
module="ceasar"
device="ceasar"

# invoke rmmod with all arguments we got
/sbin/rmmod $module $* || exit 1

# Remove stale nodes

rm -f /dev/${device}d
rm -f /dev/${device}e

