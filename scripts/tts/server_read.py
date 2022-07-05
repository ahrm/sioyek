import requests
import sys

if __name__ == '__main__':
    r = requests.post('http://127.0.0.1:5000/read', json={"path": sys.argv[1], "page_number": sys.argv[2], "line_text": sys.argv[3]})
