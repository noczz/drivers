#!/bin/sh

echo
echo 'cat /proc/devices | grep scull'
cat /proc/devices | grep scull

echo
echo 'ls -l /dev/scull*'
ls -l /dev/scull*

echo
echo 'cat /dev/scull*'
cat /dev/scull*

echo
echo 'Hello'
echo 'Hello, I am scull0' > /dev/scull0
echo 'Herlo, I am scull1' > /dev/scull1
echo 'Hello, I am scull2' > /dev/scull2
echo 'Hello, I am scull3' > /dev/scull3
echo 'Hello, I am scull_single' > /dev/scull_single
echo 'Hello, I am scull_uid' > /dev/scull_uid
echo 'Hello, I am scull_wuid' > /dev/scull_wuid
echo 'Hello, I am scull_priv' > /dev/scull_priv

echo
echo 'cat /dev/scull*'
cat /dev/scull*
