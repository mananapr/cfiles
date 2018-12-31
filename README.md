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
- [ ] Improve Upwards Scrolling
- [ ] Show sorted directories before files
- [ ] Fix the `G` keybinding
- [ ] Add basic operations like renaming, copying etc.
- [ ] Show more info in the statusbar
- [ ] Add file previews
