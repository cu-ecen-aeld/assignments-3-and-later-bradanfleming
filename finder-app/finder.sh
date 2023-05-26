#!/bin/sh
if test "${#}" -lt 2; then
	printf "Not enough arguments!\nUsage: %s <dir> <string>\n" "${0}"
	exit 1
fi
X=$(find $1 -type f | wc -l)
Y=$(grep -R "${2}" "${1}" | wc -l)
printf "The number of files are %s and the number of matching lines are %s\n" "${X}" "${Y}"
