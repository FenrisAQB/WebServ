import os

def getChapter():
	chapter = os.getenv("QUERY_STRING")
	if chapter == None:
		chapter = 1
	return int(chapter)

def readFile():
	script_dir = os.path.dirname(os.path.abspath(__file__))
	file_path = os.path.join(script_dir, "..", "html", "long_file.html")
	try:
		with open(file_path, 'r', encoding='utf-8') as file:
			content = file.read()
			return content
	except Exception as e:
		return f"Error opening file: {str(e)}"

def extractChapter(chapter_num, content):
	chapter_start = f'<h3 class="title solo">Chapter {chapter_num}.'
	chapter_end = f'<a class="indexLink solo" href="#index">'
	try:
		start_pos = content.find(chapter_start)
		if start_pos == -1:
			return "Chapter not found"
		end_pos = content.find(chapter_end, start_pos)
		if end_pos == -1:
			end_pos = content.find('<a class="indexLink solo"', start_pos)
		chapter_content = content[start_pos:end_pos] if end_pos != -1 else content[start_pos:]
		html = f"""<!DOCTYPE html>
<html>
<head>
	<br><title>Chapter {chapter_num}</title>
	<link rel="stylesheet" href="../style.css">
</head>
<body>
	<div class="main-content">
		{chapter_content}
	</div>
"""
		if int(chapter_num) == 1:
			html = f"""{html}
<a class="link solo" href="./getCgi.py?chapter=2">Next chapter </a>"""
		elif int(chapter_num) == 64:
			html = f"""{html}
<a class="link solo" href="./getCgi.py?chapter=63">Previous chapter </a>"""
		else:
			html = f"""{html}
<div class="duo">
<a class="link" href="./getCgi.py?chapter={int(chapter_num) - 1}">Previous chapter </a>
<a class="link" href="./getCgi.py?chapter={int(chapter_num) + 1}">Next chapter </a>
</div>"""
		html = f"""{html}
<br><a class="link solo" href="/">Back to home</a>
</body>
</html>"""
		return html
	except Exception as e:
		return 	f"Error finding chapter: {str(e)}"

def main():
	chapter_num = getChapter()
	content = readFile()
	result = extractChapter(chapter_num, content)
	print(result)

main()

