module="complete"
device="complete"

rmmod $module $* || exit 1

rm -f /dev/${device}
