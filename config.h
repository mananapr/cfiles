#ifndef CONFIG
#define CONFIG


/*
    cfiles settings
*/

// Set to 1 if you want to see hidden files on startup
#define SHOW_HIDDEN 0

// Set to 0 if you want to disable borders
#define SHOW_BORDERS 1

// Set to 0 if you don't want to see number of selected files in the statusbar
#define SHOW_SELECTION_COUNT 1

// Set to 0 if you don't want to see PDF Previews
#define SHOW_PDF_PREVIEWS 0

// Program used to open non-text file (Eg: `xdg-open` or `thunar`)
#define FILE_OPENER "xdg-open"

// Display Image Script
#define DISPLAYIMG "/usr/share/cfiles/scripts/displayimg_uberzug"

// Clear Image Preview Script
#define CLEARIMG "/usr/share/cfiles/scripts/clearimg_uberzug"


/*
    Color Settings
*/

// Shell Color Number to use for directories
#define DIR_COLOR 4

// Shell Color Number to use for file count which is displayed in the statusbar
#define STATUS_FILECOUNT_COLOR 1

// Shell Color Number to use for selected file which is displayed in the statusbar
#define STATUS_SELECTED_COLOR 6


/*
    Change your keybindings in this section
*/

// Go up
#define KEY_NAVUP 'k'

// Go down
#define KEY_NAVDOWN 'j'

// Go to next directory / Open file
#define KEY_NAVNEXT 'l'

// Go to parent directory
#define KEY_NAVBACK 'h'

// Go to the end of the current directory
#define KEY_GOEND 'G'

// Go to the start of the current directory
#define KEY_START 'g'

// Go to the top of the current view
#define KEY_TOP 'H'

// Go to the middle of the current view
#define KEY_MID 'M'

// Go to the bottom of the current view
#define KEY_BOTTOM 'L'

// Search all files with current directory as base
#define KEY_SEARCHALL 'F'

// Search all files within the current directory
#define KEY_SEARCHDIR 'f'

// Add file to selection list
#define KEY_SEL ' '

// View all the selected files
#define KEY_VIEWSEL '\t'

// Edit the selection list
#define KEY_EDITSEL 'e'

// Empty the selection list
#define KEY_EMPTYSEL 'u'

// Copy files in selection list to the current directory
#define KEY_YANK 'y'

// Move files in selection list to the current directory
#define KEY_MV 'v'

// Rename files in selection list
#define KEY_RENAME 'a'

// For getting an option to either move the file to trash or delete it
#define KEY_REMOVEMENU 'd'

// For moving the file to trash after pressing KEY_REMOVEMENU
#define KEY_TRASH 'd'

// For removing the file after pressing KEY_REMOVEMENU
#define KEY_DELETE 'D'

// View file info
#define KEY_INFO 'i'

// Open Shell
#define KEY_SHELL 'S'

// Toggle Hidden Files
#define KEY_TOGGLEHIDE '.'

// Toggle Borders
#define KEY_TOGGLEBORDERS 'b'

// Bookmarks Key
#define KEY_BOOKMARK '\''

// Add Bookmark Key
#define KEY_ADDBOOKMARK 'm'

// External Scripts Key
#define KEY_SCRIPT 'p'

// View preview
#define KEY_PREVIEW 'I'

// Reload
#define KEY_RELOAD 'r'

#endif
