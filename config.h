#ifndef CONFIG
#define CONFIG

/*
    cfiles settings
*/

// Set to 1 if you want to see hidden files on startup
#define SHOW_HIDDEN 0

// Set to 0 if you want to disable borders
#define SHOW_BORDERS 1


/*
    Change your keybindings in this section    
*/

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

// Toggle Hiddem Files
#define KEY_TOGGLEHIDE '.'

// Bookmarks Key
#define KEY_BOOKMARK '\''

// Add Bookmark Key
#define KEY_ADDBOOKMARK 'm'

// External Scripts Key
#define KEY_SCRIPT 'p'

// Reload
#define KEY_RELOAD 'r'

#endif
