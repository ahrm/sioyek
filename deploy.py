import os

if __name__ == '__main__':
	with open('E:\\labs\hexomancer-sioyek\\sioyek\\last_version.txt', 'r') as infile:
		last_version = int(infile.read())
	new_version = last_version + 1
	with open('E:\\labs\hexomancer-sioyek\\sioyek\\last_version.txt', 'w') as outfile:
		outfile.write(str(new_version))
	
	os.chdir('E:\\labs\hexomancer-sioyek\\sioyek')
	os.system("git pull upstream main")
	os.system(f"git tag v0.31.{new_version}")
	os.system("git push origin main")
	os.system("git push origin --tags")
	# os.system(f"git push origin v0.31.{new_version}")
	os.system("pause")