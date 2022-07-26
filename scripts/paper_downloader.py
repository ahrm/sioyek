'''
Allows sioyek to automatically download papers from scihub by clicking on their names
Requires the following python packages to be installed:
* PyPaperBot: https://github.com/ferru97/PyPaperBot
* habanero: https://github.com/sckott/habanero

Here is an example `prefs_user.config` file which uses this script:

    execute_command_l python /path/to/paper_downloader.py "%{sioyek_path}" "%{paper_name}"
    control_click_command execute_command_l

Now, you can control+click on paper names to download them and open them in sioyek.
'''

# where to put downloaded papers, if it is None, we use a papers folder next to this script
PAPERS_FOLDER_PATH = None
SIOYEK_PATH = None
PYTHON_EXECUTABLE = 'python'

import os
import pathlib
import subprocess
import sys
import json
import time
import regex
import fitz

from habanero import Crossref

def log(file_name, text):
    with open(file_name, 'a') as f:
        f.write(str(text) + '\n')

def show_status(string):
    if len(string) > 0:
        subprocess.run([SIOYEK_PATH, '--execute-command', 'set_status_string','--execute-command-data', string])
    else:
        subprocess.run([SIOYEK_PATH, '--execute-command', 'clear_status_string'])
        

def clean_paper_name(paper_name):
    first_quote_index = -1
    last_quote_index = -1

    try:
        first_quote_index = paper_name.index('"')
        last_quote_index = paper_name.rindex('"')
    except ValueError as e:
        pass

    if first_quote_index != -1 and last_quote_index != -1 and (last_quote_index - first_quote_index) > 10:
        return paper_name[first_quote_index + 1:last_quote_index]

    paper_name = paper_name.strip()
    if paper_name.endswith('.'):
        paper_name = paper_name[:-1]

    return paper_name

def is_file_a_paper_with_name(file_name, paper_name):
    doc = fitz.open(file_name)
    page_text = doc.get_page_text(0)
    doc.close()
    if regex.search('(' + regex.escape(paper_name) + '){e<=6}', page_text, flags=regex.IGNORECASE):
        return True
    return False


class ListingDiff():
    def __init__(self, path):
        self.path = path

    def get_listing(self):
        return set(os.listdir(self.path))

    def __enter__(self):
        self.listing = self.get_listing()
        return self
    
    def new_files(self):
        return list(self.get_listing().difference(self.listing))
    
    def new_pdf_files(self):
        return [f for f in self.new_files() if f.endswith('.pdf')]
    
    def reset(self):
        self.listing = self.get_listing()

    def __exit__(self, exc_type, exc_value, exc_traceback):
        pass

def get_cached_doi_map():
    json_path = get_papers_folder_path() / 'doi_map.json'
    if os.path.exists(json_path):
        with open(json_path, 'r') as infile:
            return json.load(infile)
    else:
        return dict()

def write_cache_doi_map(doi_map):
    json_path = get_papers_folder_path() / 'doi_map.json'
    with open(json_path, 'w') as outfile:
        json.dump(doi_map, outfile)


def get_papers_folder_path_():
    if PAPERS_FOLDER_PATH != None:
        return pathlib.Path(PAPERS_FOLDER_PATH)
    return pathlib.Path(os.path.dirname(os.path.realpath(__file__))) / 'papers'

def get_papers_folder_path():
    path = get_papers_folder_path_()
    path.mkdir(parents=True, exist_ok=True)
    return path

def get_doi_with_name(paper_name):
    crossref = Crossref()
    response = crossref.works(query=paper_name)
    if len(response['message']['items']) == 0:
        return None
    return response['message']['items'][0]['DOI']

def download_paper_with_doi(doi_string, paper_name, doi_map):
    download_dir = get_papers_folder_path()
    methods_to_try = [
        ('scihub', [PYTHON_EXECUTABLE, '-m', 'PyPaperBot', '--doi={}'.format(doi_string), '--dwn-dir={}'.format(download_dir)]),
        ('google scholar', [PYTHON_EXECUTABLE, '-m', 'PyPaperBot', '--query={}'.format(paper_name), '--scholar-pages=1', '--scholar-results=1', '--dwn-dir={}'.format(download_dir)])
    ]
    with ListingDiff(download_dir) as listing_diff:

        ignored_files = []

        for method_name, method_args in methods_to_try:
            show_status('trying to download "{}" from {}'.format(paper_name, method_name))
            try:
                subprocess.run(method_args)
            except Exception as e:
                show_status('error in download from {}'.format(method_name))
            pdf_files = listing_diff.new_pdf_files()
            if len(pdf_files) > 0:
                returned_file = download_dir / pdf_files[0]
                if is_file_a_paper_with_name(str(returned_file), paper_name):
                    doi_map[doi_string] = str(returned_file)
                    write_cache_doi_map(doi_map)
                    return returned_file
                else:
                    ignored_files.append(returned_file)
                    listing_diff.reset()

        if len(ignored_files) > 0:
            show_status('could not find a suitable paper, this is a throw in the dark')
            return ignored_files[0]

    return None

def get_paper_file_name_with_doi_and_name(doi, paper_name):

    doi_map = get_cached_doi_map()
    if doi in doi_map.keys():
        if os.path.exists(doi_map[doi]):
            return doi_map[doi]
    return download_paper_with_doi(doi, paper_name, doi_map)


if __name__ == '__main__':

    SIOYEK_PATH = sys.argv[1]
    paper_name = clean_paper_name(sys.argv[2])

    show_status('finding doi ...')
    try:
        doi = get_doi_with_name(sys.argv[2])
        if doi:
            # show_status('downloading doi: {}'.format(doi))
            file_name = get_paper_file_name_with_doi_and_name(doi, paper_name)
            show_status("")
            if file_name:
                subprocess.run([SIOYEK_PATH, str(file_name), '--new-window'])
        else:
            show_status('doi not found')
            time.sleep(5)
            show_status("")
    except Exception as e:
        show_status('error: {}'.format(str(e)))
        time.sleep(5)
        show_status("")