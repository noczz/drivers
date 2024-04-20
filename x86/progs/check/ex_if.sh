output=$(ls -l /abc) # no thing will be transform
echo $output
if [ $output ]; then
	echo true
else
	echo false
fi
