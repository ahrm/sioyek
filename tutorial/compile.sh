#!/usr/bin/env sh
pdflatex tut.tex
bibtex tut
pdflatex tut.tex
pdflatex tut.tex
