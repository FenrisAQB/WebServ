#!/bin/bash

getChapter() {
	local chapter="${QUERY_STRING}"
	if [ -z "$chapter" ]; then
		chapter="1"
	else
		chapter=$((10#$chapter))
	fi
	echo "$chapter"
}

readFile() {
	local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
	local file_path="${script_dir}/../html/long_file.html"

	if [ -f "$file_path" ]; then
		cat "$file_path"
	else
		echo "Error opening file: File not found at $file_path"
		return 1
	fi
}

extractChapter() {
	local chapter_num="$1"
	local content="$2"

	local chapter_start="<h3 class=\"title solo\">Chapter ${chapter_num}."
	local chapter_end='<a class="indexLink solo" href="#index">'
	local start_pos=$(echo "$content" | grep -b -o "$chapter_start" | head -1 | cut -d: -f1)
	if [ -z "$start_pos" ]; then
		echo "Chapter not found"
		return 1
	fi
	local temp_content=$(echo "$content" | tail -c +$((start_pos + 1)))
	local end_content=$(echo "$temp_content" | sed -n "/$chapter_end/q;p")
	cat << EOF
<!DOCTYPE html>
<html>
<head>
	<br><title>Chapter ${chapter_num}</title>
	<link rel="stylesheet" href="../style.css">
</head>
<body>
	<div class="main-content">
		${end_content}
	</div>
EOF
	if [ "$chapter_num" -eq 1 ]; then
		echo '<a class="link solo" href="./getCgi.sh?chapter=2">Chapter 2</a>'
	elif [ "$chapter_num" -eq 64 ]; then
		echo '<a class="link solo" href="./getCgi.sh?chapter=63">Chapter 63</a>'
	else
		cat << EOF
<div class="duo">
<a class="link" href="./getCgi.sh?chapter=$((chapter_num - 1))">Chapter $((chapter_num - 1))</a>
<a class="link" href="./getCgi.sh?chapter=$((chapter_num + 1))">Chapter $((chapter_num + 1))</a>
</div>
EOF
	fi
	cat << EOF
<br><a class="link solo" href="/">Back to home</a>
</body>
</html>
EOF
}

main() {
	local chapter_num=$(getChapter)
	local content=$(readFile)

	if [ $? -ne 0 ]; then
		echo "$content"
		return 1
	fi
	extractChapter "$chapter_num" "$content"
}

main
