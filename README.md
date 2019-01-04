# cfiles
`cfiles` is a work in progress terminal file manager written in C using the ncurses
library. It aims to provide an interface like [ranger](https://github.com/ranger/ranger) while being lightweight, fast and
minimal.

![screenshot](cf.png)

## Compiling and Installation
To compile, run

   `gcc cf.c -lncurses -o cf`

To install, simply move the generated executable to a directory that is in your `$PATH`

## Why C?
I wanted to improve my C and learn ncurses so I decided this would be an ideal project.

Apart from this, I have always wanted an alternative to ranger that is faster while still having
a similar UI.

## Todo
- [x] Improve Upwards Scrolling
- [ ] Show sorted directories before files
- [x] Fix the `G` keybinding
- [ ] Add basic operations like renaming, copying etc.
- [ ] Show more info in the statusbar
- [ ] Add file previews
- [x] Add functionality to open files
- [x] Add image previews using w3mimgdisplay
- [ ] Preserve aspect ratio in image previews
- [x] Add fuzzy file search using fzf
- [x] Find a way to redraw windows after displaying image previews or running fzf
- [ ] Find a way to remove cursor after running fzf
- [ ] Select the file directly from fzf
- [x] Supress output from xdg-open
- [ ] Find a way to remember selection position of parent directory
