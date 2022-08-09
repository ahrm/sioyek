import sys
from sioyek import Sioyek

colormap = {'a': (0.94, 0.64, 1.00),
            'b': (0.00, 0.46, 0.86),
            'c': (0.60, 0.25, 0.00),
            'd': (0.30, 0.00, 0.36),
            'e': (0.10, 0.10, 0.10),
            'f': (0.00, 0.36, 0.19),
            'g': (0.17, 0.81, 0.28),
            'h': (1.00, 0.80, 0.60),
            'i': (0.50, 0.50, 0.50),
            'j': (0.58, 1.00, 0.71),
            'k': (0.56, 0.49, 0.00),
            'l': (0.62, 0.80, 0.00),
            'm': (0.76, 0.00, 0.53),
            'n': (0.00, 0.20, 0.50),
            'o': (1.00, 0.64, 0.02),
            'p': (1.00, 0.66, 0.73),
            'q': (0.26, 0.40, 0.00),
            'r': (1.00, 0.00, 0.06),
            's': (0.37, 0.95, 0.95),
            't': (0.00, 0.60, 0.56),
            'u': (0.88, 1.00, 0.40),
            'v': (0.45, 0.04, 1.00),
            'w': (0.60, 0.00, 0.00),
            'x': (1.00, 1.00, 0.50),
            'y': (1.00, 1.00, 0.00),
            'z': (1.00, 0.31, 0.02)
}

if __name__ == '__main__':
    SIOYEK_PATH = sys.argv[1]
    LOCAL_DATABASE_PATH = sys.argv[2]
    SHARED_DATABASE_PATH = sys.argv[3]
    FILE_PATH = sys.argv[4]

    sioyek = Sioyek(SIOYEK_PATH, LOCAL_DATABASE_PATH, SHARED_DATABASE_PATH)
    document = sioyek.get_document(FILE_PATH)
    document.embed_new_annotations(save=True, colormap=colormap)
    document.close()
    sioyek.reload()
    sioyek.close()
