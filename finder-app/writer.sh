#!/bin/sh
if test "${#}" -lt 2; then
        printf "Not enough arguments!\nUsage: %s <dir> <string>\n" "${0}"
        exit 1
fi
if test ! -f "${1}"; then
	mkdir -p "${1}"
	rm -r "${1}"
fi
printf "%s" "${2}" > "${1}"
