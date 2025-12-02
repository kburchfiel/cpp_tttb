cd '/home/kjb3/D1V1/kjb3docs/Programming/CPP/cpp_tttb'
pandoc README.md -f markdown -t pdf -s -o README.pdf -V colorlinks=true -V  geometry:paperwidth=8.5in -V geometry:paperheight=11in -V geometry:margin=1in

# (This snippet, which creates a PDF version of the README.md file (while also preserving images), was based on the following sources:
# (1) https://stackoverflow.com/a/58885711/13097194 
# (2) https://pandoc.org/getting-started.html#step-6-converting-a-file 
# (3) https://stackoverflow.com/a/13516042/13097194)
