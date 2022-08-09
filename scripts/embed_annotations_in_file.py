import sys
from sioyek import Sioyek

if __name__ == '__main__':
	SIOYEK_PATH = sys.argv[1]
	LOCAL_DATABASE_PATH = sys.argv[2]
	SHARED_DATABASE_PATH = sys.argv[3]
	FILE_PATH = sys.argv[4]

	sioyek = Sioyek(SIOYEK_PATH, LOCAL_DATABASE_PATH, SHARED_DATABASE_PATH)
	document = sioyek.get_document(FILE_PATH)
	document.embed_new_annotations(save=True)
	document.close()
	sioyek.reload()
	sioyek.close()