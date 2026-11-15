lualatex -shell-escape -synctex=1 -interaction=nonstopmode main.tex
lualatex -shell-escape -synctex=1 -interaction=nonstopmode main.tex

bibtex main.aux

lualatex -shell-escape -synctex=1 -interaction=nonstopmode main.tex
lualatex -shell-escape -synctex=1 -interaction=nonstopmode main.tex

rm main.aux main.bbl main.blg main.log main.synctex.gz main.toc
rm -rf ./_minted-main
