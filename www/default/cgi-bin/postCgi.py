import os

def main():
	path = os.getenv("UPLOAD_DIR") + os.getenv("PATH_INFO")
	try:
		with open(path, '+a') as file:
			file.write(os.getenv("BODY"))
			file.close()
	except Exception as e:
		print(f"Post error: {str(e)}")
	print("POST succeed")

main()