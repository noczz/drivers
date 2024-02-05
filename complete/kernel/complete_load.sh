module="complete"
device="complete"
mode="664"

if grep -q '^staff:' /etc/group; then
	group="staff"
else
	group="wheel"
fi

insmod ./$module.ko $* || exit 1

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

rm -f /dev/${device}
mknod /dev/${device} c $major 0
chgrp $group /dev/${device}
chmod $mode /dev/${device}
