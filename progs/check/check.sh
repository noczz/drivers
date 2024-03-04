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
	#echo 'Hello, I am scull_pip0' > /dev/scull_pipe0
	#echo 'Hello, I am scull_pip1' > /dev/scull_pipe1
	#echo 'Hello, I am scull_pip2' > /dev/scull_pipe2
	#echo 'Hello, I am scull_pip3' > /dev/scull_pipe3
	echo
	echo 'cat /dev/scull*'
	cat /dev/scull*
else
	echo "/dev/scull* don't exist"
fi

echo
echo 'cat /proc/scull_seq'
cat /proc/scullseq
