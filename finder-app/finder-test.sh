#!/bin/sh
# Tester script for assignment 1 and assignment 2
# Author: Siddhant Jajoo

set -e
set -u

NUMFILES=10
WRITESTR=AELD_IS_FUN
WRITEDIR=/tmp/aeld-data
# Assignment 4 Part 2: Assume config files are in /etc/finder-app/conf
username=$(cat /etc/finder-app/conf/username.txt)

if [ $# -lt 3 ]
then
	echo "Using default value ${WRITESTR} for string to write"
	if [ $# -lt 1 ]
	then
		echo "Using default value ${NUMFILES} for number of files to write"
	else
		NUMFILES=$1
	fi	
else
	NUMFILES=$1
	WRITESTR=$2
	WRITEDIR=/tmp/aeld-data/$3
fi

MATCHSTR="The number of files are ${NUMFILES} and the number of matching lines are ${NUMFILES}"

echo "Writing ${NUMFILES} files containing string ${WRITESTR} to ${WRITEDIR}"

rm -rf "${WRITEDIR}"

# create $WRITEDIR if not assignment1
# Assignment 4 Part 2: Assume config files are in /etc/finder-app/conf
assignment=`cat /etc/finder-app/conf/assignment.txt`

if [ $assignment != 'assignment1' ]
then
	mkdir -p "$WRITEDIR"

	#The WRITEDIR is in quotes because if the directory path consists of spaces, then variable substitution will consider it as multiple argument.
	#The quotes signify that the entire string in WRITEDIR is a single string.
	#This issue can also be resolved by using double square brackets i.e [[ ]] instead of using quotes.
	if [ -d "$WRITEDIR" ]
	then
		echo "$WRITEDIR created"
	else
		exit 1
	fi
fi
#echo "Removing the old writer utility and compiling as a native application"
#make clean
#make

# Assignment 2: Remove build artifacts and compile writer for host
# Assignment 3: Remove make step from finder-test.sh (lol)
#echo "Compiling writer program"
#make clean writer

for i in $( seq 1 $NUMFILES)
do
	# Assignment 2: Use writer program instead of writer.sh script
	# Assignment 4 Part 1: Run binaries from PATH
	writer "$WRITEDIR/${username}$i.txt" "$WRITESTR"
done

# Assignment 4 Part 1: Run binaries from PATH
OUTPUTSTRING=$(finder.sh "$WRITEDIR" "$WRITESTR")
# Assignment 4 Part 1: Write output of finder command to /tmp/assignment4-result.txt
writer "/tmp/assignment4-result.txt" "${OUTPUTSTRING}"

# remove temporary directories
rm -rf /tmp/aeld-data

set +e
echo ${OUTPUTSTRING} | grep "${MATCHSTR}"
if [ $? -eq 0 ]; then
	echo "success"
	exit 0
else
	echo "failed: expected  ${MATCHSTR} in ${OUTPUTSTRING} but instead found"
	exit 1
fi
