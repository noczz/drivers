#!/bin/sh

echo 'cat /proc/devices | grep scull'
cat /proc/devices | grep scull

devfiles=$(ls -l /dev/scull*)

echo
echo 'Hello'

if [ -n "$devfiles" ]; then
	echo 'Hello, I am scull0' > /dev/scull0
	echo 'Herlo, I am scull1' > /dev/scull1
	echo 'Hello, I am scull2' > /dev/scull2
	echo 'Hello, I am scull3' > /dev/scull3
	echo 'Hello, I am scull_single' > /dev/scull_single
	echo 'Hello, I am scull_uid' > /dev/scull_uid
	echo 'Hello, I am scull_wuid' > /dev/scull_wuid
	echo 'Hello, I am scull_priv' > /dev/scull_priv
	echo
	echo 'cat /dev/scull[0-3_][a-z][^i]*'
	cat /dev/scull[0-3]
	cat /dev/scull_*[^0-3]
else
	echo "/dev/scull* don't exist"
fi

echo
echo 'cat /proc/scull_seq'
cat /proc/scullseq

echo
echo 'scull devices:'
ls -al /dev/scull*
