'''
This script was tested with PyMuPDF version 1.17.6. Other versions may require slight modification of the code.

This script can be used to embed annotations as you create them, so they are viewable in other
PDF viewers.
This is basically a script that can either add a bookmark or a highlight to a page based on the command line arguments.
Here is the `prefs_user.config` that I used:

	execute_command_b python path/to/embedded_annotations.py bookmark "%1" %{mouse_pos_document} "%5"
	execute_command_h python path/to/embedded_annotations.py highlight "%1" "%4" "%3"

And here is a `keys_user.config` file that can be used:

	execute_command_b b
	execute_command_h;add_highlight h

which basically adds both sioyek and embedded highlights and bookmarks (if `ADD_BOOKMARKS_TO_SIOYEK` is True). Alternatively, you can use
a different keybinding for embedded annotations so you have control over what happens. Now you can use alt+b or alt+h to bookmark/highlight.

	execute_command_b <A-b>
	execute_command_h <A-h>
'''

import sys
import fitz
import subprocess

# if set to true, we re-add the bookmarks into sioyek, so we have both types of bookmarks
ADD_BOOKMARKS_TO_SIOYEK = False
PATH_TO_SIOYEK = r'path/to/sioyek.exe'

def add_bookmark(doc_path, page_number, location, text):
	doc = fitz.open(doc_path)
	page = doc.loadPage(page_number)
	page.addTextAnnot(location, text)
	doc.saveIncr()
	doc.close()
	if ADD_BOOKMARKS_TO_SIOYEK:
		subprocess.run([PATH_TO_SIOYEK, '--execute-command', 'add_bookmark','--execute-command-data', text])
		subprocess.run([PATH_TO_SIOYEK, '--execute-command', 'reload'])


def add_highlight(doc_path, page_number, text):
	doc = fitz.open(doc_path)
	page = doc.loadPage(page_number)
	quads = page.searchFor(text, flags=fitz.TEXT_PRESERVE_WHITESPACE, hit_max=50)
	page.addHighlightAnnot(quads)
	doc.saveIncr()
	doc.close()

if __name__ == '__main__':
	if sys.argv[1] == 'bookmark':
		add_bookmark(sys.argv[2], int(sys.argv[3]), (float(sys.argv[4]), float(sys.argv[5])), sys.argv[6])

	if sys.argv[1] == 'highlight':
		add_highlight(sys.argv[2], int(sys.argv[3]), sys.argv[4])

