#!/bin/bash

main() {
	local path="${UPLOAD_DIR}${PATH_INFO}"
	if printf "%s" "${BODY}" >> "$path" 2>/dev/null; then
		echo "POST succeed"
	else
		echo "Post error: Failed to write to $path"
	fi
}

main
