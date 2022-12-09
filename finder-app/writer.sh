#!/bin/bash
if [ $# -ne 2 ] 
then
    echo "ERROR: Invalid number of arguments provided!"
    exit 1
fi
writefile=$1
writestr=$2

if [ ! -d $( dirname $writefile ) ]
then
    mkdir $( dirname $writefile )
    if [ $? -ne 0 ]
    then
        echo "ERROR: Could not create directory $( dirname $writefile )"
        exit 1
    fi
fi
echo $writestr > $writefile

if [ $? -ne 0 ]
then
    echo "ERROR: Could not write string $writestr to file $writefile"
    exit 1
fi
exit 0