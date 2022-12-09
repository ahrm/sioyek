'''
example `prefs_user.config`:
    # start reading from highlighted line
    execute_command_a python D:\labs2\tts\second-try\server_read.py "%1" %4 "%6"
    # stop reading
    execute_command_b python D:\labs2\tts\second-try\server_stop.py
    # keep highlighting the current line being read
    execute_command_c python D:\labs2\tts\second-try\server_follow.py
    # stop highlighting the current line being read
    execute_command_d python D:\labs2\tts\second-try\server_unfollow.py
    # start the tts server (should be running before executing previous commands)
    execute_command_e python D:\labs2\tts\second-try\manager_server.py

example `keys_user.config`:
    execute_command_a r
    execute_command_b <S-r>
    execute_command_e <C-<f5>>
    # when we manually move the line, stop following it
    move_visual_mark_down;execute_command_d j
    move_visual_mark_up;execute_command_d k
'''

from flask import Flask
from flask import request
import threading
import time
import datetime
import subprocess

import hashlib
import json
import os

import fitz
import pygame
from pygame import mixer
import platform

if (platform.system() == "Darwin"):
    # cached audio files will be stored in this directory
    DATA_FOLDER = "./tts-output/"
    SIOYEK_PATH = "sioyek"
    POWER_SHELL = "pwsh"
else:
    # cached audio files will be stored in this directory
    DATA_FOLDER = "D:\\tts-output\\"
    SIOYEK_PATH = "sioyek.exe"
    POWER_SHELL = "powershell"

if(not os.path.exists(DATA_FOLDER)):
    os.mkdir(DATA_FOLDER)

def lcs(pattern_1, pattern_2):
    m = len(pattern_1)
    n = len(pattern_2)
    # dp will store solutions as the iteration goes on
    dp = [
        [None] * (n + 1) for item in range(m + 1)
    ]
    for i in range(m + 1):
        for j in range(n + 1):
            if i == 0 or j == 0:
                dp[i][j] = 0
            elif pattern_1[i - 1] == pattern_2[j - 1]:
                dp[i][j] = dp[i - 1][j - 1] + 1
            else :
                dp[i][j] = max(dp[i - 1][j], dp[i][j - 1])
    return dp[m][n]

def convert_text_to_appropriate_format_for_tts(text):
    lines = text.split('\n')
    n_lines = len(lines)
    out_lines = []
    lines_to_merge = []
    
    for i, line in enumerate(lines):
        if i < (n_lines-1) and line[-1] == '-':
            lines_to_merge.append(line[:-1])
        elif i < (n_lines-1) and line[-1] != '.':
            lines_to_merge.append(line + ' ')
        else:
            lines_to_merge.append(line)
            line_text_raw = ''.join(lines_to_merge)
            line_text_clean = ''.join(x for x in line_text_raw if ord(x) < 127)
            out_lines.append(line_text_clean)
            lines_to_merge = []
    return ('\n'.join(out_lines)).replace('"', '""')


def parse_alignment_file(filename):
    with open(filename, 'r') as infile:
        data = json.load(infile)
    output = []
    for i, frag in enumerate(data['fragments']):
        output.append((float(frag['begin']), float(frag['end']), frag['lines'][0], i))
    return output

class Manager:

    def __init__(self):
        self.should_follow_lines = True
        self.doc_cache = dict()
        self.alignment_cache = dict()
        self.worker_threads = []
        self.background_mutex = threading.Lock()
        self.current_track = None
        self.offset = None
        self.current_state = None
        self.last_clean_time = datetime.datetime.now()
        mixer.init()
    
    def get_doc(self, path):
        if path not in self.doc_cache.keys():
            self.doc_cache[path] = fitz.open(path)
        return self.doc_cache[path]
    
    def get_page_text(self, path, page):
        return self.get_doc(path).get_page_text(page)

    def get_page_text_tts(self, path, page):
        return convert_text_to_appropriate_format_for_tts(self.get_page_text(path, page))

    def get_digest(self, path, page, fast):
        text = self.get_page_text(path, page)
        digest = hashlib.sha1(text.encode('utf-8')).hexdigest()
        if fast:
            return digest + "_fast"
        else:
            return digest + "_good"

    def get_alignment(self, digest):
        file_name = DATA_FOLDER + digest + "_alignment.json"
        if digest not in self.alignment_cache.keys():
            self.alignment_cache[digest] = parse_alignment_file(file_name)
        return self.alignment_cache[digest]
    
    def has_fast_tts(self, path, page):
        digest = self.get_digest(path, page, True)
        return os.path.exists(DATA_FOLDER + digest + "_alignment.json")

    def has_good_tts(self, path, page):
        digest = self.get_digest(path, page, False)
        return os.path.exists(DATA_FOLDER + digest + "_alignment.json")

    def enable_line_follow(self):
        self.should_follow_lines = True

    def disable_line_follow(self):
        self.should_follow_lines = False
    
    def create_files_or_wait_for_background(self, path, page_number, fast):
        with self.background_mutex:
            job = self.find_background_job(path, page_number, fast)
            if job != None:
                job.join()
            else:
                self.create_files(path, page_number, fast)


    def create_files(self, path, page_number, fast):
            digest = self.get_digest(path, page_number, fast)


            tts_text_file_name = DATA_FOLDER + digest + "_tts.txt"
            align_text_file_name = DATA_FOLDER + digest + "_align.text"
            align_output_file_name = DATA_FOLDER + digest + "_alignment.json"
            audio_file_name = DATA_FOLDER + digest + ".wave"
            ogg_file_name = DATA_FOLDER + digest + ".ogg"

            tts_text = self.get_page_text_tts(path, page_number)
            align_text = self.get_page_text(path, page_number)

            with open(tts_text_file_name, "w", encoding='utf8') as outfile:
                outfile.write(tts_text)

            with open(align_text_file_name, "w", encoding='utf8') as outfile:
                outfile.write(align_text)

            if fast:
                subprocess.run([POWER_SHELL, os.path.dirname(__file__) + "\\generator.ps1", tts_text_file_name, audio_file_name])
            else:
                subprocess.run([POWER_SHELL, os.path.dirname(__file__) + "\\generator2.ps1", tts_text_file_name, audio_file_name])

            subprocess.run(["sox", audio_file_name, ogg_file_name, 'tempo', '1.5'])
            subprocess.run([POWER_SHELL, os.path.dirname(__file__) + "\\aligner.ps1", ogg_file_name, align_text_file_name, align_output_file_name])

    def find_background_job(self, path, page_number, fast):
        for thread_path, thread_page, thread_fast, thread in self.worker_threads:
            if (thread_path, thread_page, thread_fast) == (path, page_number, fast):
                return thread
        return None

    def add_background_job(self, path, page_number, fast):
        with self.background_mutex:
            if self.find_background_job(path, page_number, fast) != None:
                return False
            thread = threading.Thread(target=self.create_files, args=(path, page_number, fast))
            self.worker_threads.append((path, page_number, fast, thread))
            thread.start()
            return True
            

    def get_best_line_index(self, alignment, line_text):
        best_line_index = -1
        best_line_score = -1

        for i, line_data in enumerate(alignment):
            current_line_text = line_data[2]
            score = lcs(current_line_text, line_text)
            if score > best_line_score:
                best_line_score = score
                best_line_index = i
        return best_line_index

    def set_track_with_digest(self, digest):
        file_path = DATA_FOLDER + digest + ".ogg"
        if self.current_track != file_path:
            mixer.music.stop()
            mixer.music.unload()
            mixer.music.load(file_path)
            mixer.music.play()
            mixer.music.set_volume(0.7)
            self.current_track = file_path

    def goto_line_with_text(self, path, page_number, line_text):
        digest = None

        if self.has_good_tts(path, page_number):
            digest = self.get_digest(path, page_number, False)
        elif self.has_fast_tts(path, page_number):
            digest = self.get_digest(path, page_number, True)
        else:
            self.create_files_or_wait_for_background(path, page_number, True)
            self.add_background_job(path, page_number, False)
            digest = self.get_digest(path, page_number, True)
        
        alignment = self.get_alignment(digest)
        if line_text != None:
            line_index = self.get_best_line_index(alignment, line_text)
        else:
            line_index = 0

        line_begin = alignment[line_index][0]
        self.set_track_with_digest(digest)
        mixer.music.play()
        mixer.music.set_pos(line_begin) #!!!!!!!!!!!!!!!!! ogg processing is accurate no need for hack mp3
        self.offset = line_begin
        self.current_state = (path, page_number, line_index, digest)
        self.enable_line_follow()

    def find_pos_index(self, pos):
        alignments = self.get_alignment(self.current_state[3])

        for i, (begin, end, _, _) in enumerate(alignments):
            if pos >= begin and pos <= end:
                return i

        return -1

    def highlight_line(self, line_text):
        subprocess.run([SIOYEK_PATH, "--focus-text", '"{}"'.format(line_text), "--focus-text-page", str(self.current_state[1]), "--nofocus"])

    def better_audio_is_available(self):
        if self.current_state[-1].endswith("_fast") and self.has_good_tts(self.current_state[0], self.current_state[1]):
            return True
        return False

    def goto_line_if_changed(self):
        current_pos = mixer.music.get_pos()
        if current_pos == None:
            return
        if current_pos != -1:
            if self.offset == None:
                pos = current_pos / 1000
            else:
                pos = current_pos / 1000 + self.offset
            index = self.find_pos_index(pos)
            if index != self.current_state[2]:
                new_line_text = self.get_alignment(self.current_state[3])[index][2]
                if self.better_audio_is_available():
                    self.goto_line_with_text(self.current_state[0], self.current_state[1], new_line_text)
                else:
                    self.highlight_line(new_line_text)
                    self.current_state = self.current_state[0], self.current_state[1], index, self.current_state[3]


    def clean_old_files(self):
        if self.last_clean_time == None or (datetime.datetime.now() - self.last_clean_time).seconds > 1000:
            # find old files
            self.last_clean_time = datetime.datetime.now()

            for file in os.listdir(DATA_FOLDER):
                stat = os.stat(DATA_FOLDER + file)
                file_time = datetime.datetime.fromtimestamp(stat.st_mtime)
                if (datetime.datetime.now() - file_time).days > 10:
                    try:
                        os.remove(DATA_FOLDER + file)
                    except Exception as e:
                        pass

    def cache_if_not_busy(self):
        should_cache = False
        with self.background_mutex:
            if len(self.worker_threads) == 0 and self.current_state != None:
                should_cache = True
        if should_cache:
            self.add_background_job(self.current_state[0], self.current_state[1]+1, False)


    def stop(self):
        mixer.music.stop()
        self.current_state = None
    
    def remove_old_threads(self):
        remaining_threads = []
        with self.background_mutex:
            for thread_path, thread_page, thread_fast, thread in self.worker_threads:
                if thread.is_alive():
                    remaining_threads.append((thread_path, thread_page, thread_fast, thread))

            self.worker_threads = remaining_threads

    def run(self):
        while True:
            self.remove_old_threads()
            if self.should_follow_lines and mixer.music.get_pos() == -1 and self.current_state != None:
                self.goto_line_with_text(self.current_state[0], self.current_state[1]+1, None)
            elif self.should_follow_lines and mixer.music.get_pos() != -1:
                self.goto_line_if_changed()

            manager.clean_old_files()
            self.cache_if_not_busy()
            time.sleep(0.1)


app = Flask(__name__)
should_stop = False

manager = Manager()

@app.route("/")
def hello_world():
    return "hello world"

@app.route("/stop")
def stop():
    global manager
    manager.stop()
    return "stopping"

@app.route("/follow")
def follow():
    global manager
    manager.enable_line_follow()
    return "following"

@app.route("/unfollow")
def unfollow():
    global manager
    manager.disable_line_follow()
    return "unfollowing"

@app.route('/read', methods=['POST'])
def read():
    global manager
    content = request.json
    print(content)
    manager.goto_line_with_text(content['path'], int(content['page_number']), content['line_text'])
    return "read"

if __name__ == '__main__':
    thread = threading.Thread(target=manager.run)
    thread.start()
    app.run()
