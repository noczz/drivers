module="complete"

make
rmmod $module || exit 1
insmod ./$module.ko $* || exit 1
make clean
