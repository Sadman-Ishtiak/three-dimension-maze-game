#!/usr/bin/env bash
# Builds main.pdf (needs: pdflatex, bibtex, and Python Pygments for minted).
set -euo pipefail
pdflatex -shell-escape -interaction=nonstopmode main.tex
bibtex main
pdflatex -shell-escape -interaction=nonstopmode main.tex
pdflatex -shell-escape -interaction=nonstopmode main.tex
