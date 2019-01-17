/*
   __ _ _
   ___ / _(_) | ___  ___
   / __| |_| | |/ _ \/ __|
   | (__|  _| | |  __/\__ \
   \___|_| |_|_|\___||___/

 */


/////////////
// HEADERS //
/////////////
#include <stdio.h>
#include <dirent.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include "config.h"


//////////////////////
// GLOBAL VARIABLES //
//////////////////////

// To store number of files in directory
int len=0;

// To store number of files in child directory
int len_preview=0;

// To store number of bookmarks
int len_bookmarks=0;

// To store number of scripts
int len_scripts=0;

// Counter variable
int i = 0;

// Direcotry to be opened
char* dir;

// Name of selected file
char selected_file[128];

// Name of the editor
char editor[20];

// char array to work with strtok() and for other one time use
char temp_dir[250];

// To work with strtok()
char *pch;

// Stores Home Directory of User
struct passwd *info;

/*
   Base directory to be used for sorting
   `dir` for current_win
   `next_dir` for preview_win
 */
char sort_dir[250];

// Stores the path for the cache directory
char cache_path[250];

// Stores the path for the clipboard file
char clipboard_path[250];

// Stores bookmarks file path
char bookmarks_path[250];

// Stores scripts directory path
char scripts_path[250];

// Stores the path for the temp clipboard file
char temp_clipboard_path[250];

// Stores the path for trash
char trash_path[250];

// Index of currently selected item in `char* directories`
int selection = 0;

// Index to start printing from `directories` array
int start = 0;

// Flag to clear preview_win
int clearFlag = 0;

// Flag is set to 1 when returning from `fzf`
int searchFlag = 0;

// Flag is set to 1 when user goes up a directory
int backFlag = 0;

// Flag to display hidden files
int hiddenFlag = SHOW_HIDDEN;

// Stores the last token in the path. For eg, it will store 'a' is path is /b/a
char *last;

// Shows current directory
WINDOW *current_win;

// Shows child directory preview
WINDOW *preview_win;

// Shows status bar
WINDOW *status_win;

// Shows keybindings
WINDOW *keys_win;

// To store maximum height & width of terminal and store start x and y positions of windows
int startx, starty, maxx, maxy;


//////////////////////
// HELPER FUNCTIONS //
//////////////////////

/*
   Initializes the program
   Sets the relevant file paths
 */
void init()
{
    // Get UID of user
    uid_t uid = getuid();
    // Get home directory of user from UID
    info = getpwuid(uid);

    // Set the editor
    if( getenv("EDITOR") == NULL)
        sprintf(editor, "%s", "vim");
    else
        sprintf(editor, "%s", getenv("EDITOR"));

    // Make the cache directory
    struct stat st = {0};
    sprintf(cache_path,"%s/.cache/cfiles",info->pw_dir);
    if (stat(cache_path, &st) == -1) {
        mkdir(cache_path, 0751);
    }

    // Set the path for the clipboard file
    sprintf(clipboard_path,"%s/clipboard",cache_path);
    // Set the path for the bookmarks file
    sprintf(bookmarks_path,"%s/bookmarks",cache_path);
    // Set the path for the scripts directory
    sprintf(scripts_path,"%s/scripts",cache_path);
    // Set the path for the temp clipboard file
    sprintf(temp_clipboard_path,"%s/clipboard.tmp",cache_path);
    // Set the path for trash
    sprintf(trash_path,"%s/.local/share/Trash/files",info->pw_dir);

    if (stat(scripts_path, &st) == -1) {
        mkdir(scripts_path, 0751);
    }

    // Set dir as $HOME
    dir = info->pw_dir;
}


/*
   Initializes ncurses
 */
void curses_init()
{
    initscr();
    noecho();
    curs_set(0);
    start_color();
    init_pair(1, DIR_COLOR, 0);
    init_pair(2, STATUS_FILECOUNT_COLOR, 0);
    init_pair(3, STATUS_SELECTED_COLOR, 0);
    init_pair(4, 0, 0);
}


/*
   Checks if `path` is a file or directory
 */
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}


/*
   Checks if a file exists or not
 */
int fileExists(char *file)
{
    if( access( file, F_OK ) != -1 )
        return 1;
    else
        return 0;
}


/*
   Gets the last token from temp_dir by using `tokenizer` as a delimeter
 */
void getLastToken(char *tokenizer)
{
    pch = strtok(temp_dir, tokenizer);
    while (pch != NULL)
    {
        last = pch;
        pch = strtok(NULL,tokenizer);
    }
}


/*
   Get number of bookmarks
 */
int getNumberOfBookmarks()
{
    FILE *fp = fopen(bookmarks_path, "r");
    if( fp == NULL )
    {
        return -1;
    }
    char buf[250];
    int num = 0;
    while(fgets(buf, 250, (FILE*) fp))
    {
        num++;
    }
    fclose(fp);
    return num;
}


/*
   Displays the bookmarks in `keys_win`
 */
void displayBookmarks()
{
    FILE *fp = fopen(bookmarks_path, "r");
    char buf[250];
    wprintw(keys_win,"Key\tPath\n");
    while(fgets(buf, 250, (FILE*) fp))
    {
        wprintw(keys_win, "%c", buf[0]);
        strncpy(temp_dir, buf + 2, strlen(buf)-2);
        strtok(temp_dir,"\n");
        wprintw(keys_win, "\t%s\n", temp_dir);
    }
    fclose(fp);
}


/*
   Opens the bookmark denoted by `secondKey`
 */
void openBookmarkDir(char secondKey)
{
    FILE *fp = fopen(bookmarks_path, "r");
    char buf[250];
    while(fgets(buf, 250, (FILE*) fp))
    {
        if(buf[0] == secondKey)
        {
            strncpy(temp_dir, buf + 2, strlen(buf)-2);
            strtok(temp_dir,"\n");
            if( fileExists(temp_dir) == 1 )
                strcpy(dir, temp_dir);
            start = 0;
            selection = 0;
            break;
        }
    }
    fclose(fp);
}


/*
   Checks if bookmark denoted with `bookmark` exists
 */
int bookmarkExists(char bookmark)
{
    FILE *fp = fopen(bookmarks_path, "r");
    if( fp == NULL )
    {
        return 0;
    }
    char buf[250];
    while(fgets(buf, 250, (FILE*) fp))
    {
        if(buf[0] == bookmark)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}


/*
   Adds new bookmark
 */
void addBookmark(char bookmark, char *path)
{
    FILE *fp = fopen(bookmarks_path, "a+");
    fprintf(fp,"%c:%s\n", bookmark, path);
    fclose(fp);
}


/*
   Creates a new window with dimensions `height` and `width` starting at `starty` and `startx`
 */
WINDOW *create_newwin(int height, int width, int starty, int startx)
{
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    return local_win;
}


/*
   For qsort
 */
int compare (const void * a, const void * b )
{
    // They store the full paths of the arguments
    char temp_filepath1[250];
    char temp_filepath2[250];

    // Generate full paths
    sprintf(temp_filepath1,"%s/%s", sort_dir, *(char **)a);
    sprintf(temp_filepath2,"%s/%s", sort_dir, *(char **)b);

    if(is_regular_file(temp_filepath1) == 0 && is_regular_file(temp_filepath2) == 1)
        return -1;
    else if(is_regular_file(temp_filepath1) == 1 && is_regular_file(temp_filepath2) == 0)
        return 1;
    else
        return strcasecmp(*(char **)a, *(char **)b);
}


/*
   Gets file MIME
 */
void getMIME(char *filepath, char mime[10])
{
    char cmd[250];
    char buf[20];
    FILE *fp;
    sprintf(cmd, "xdg-mime query filetype %s", filepath);

    if((fp = popen(cmd,"r")) == NULL)
    {
        exit(0);
    }
    while(fgets(buf,250,fp) != NULL){}
    strtok(buf,"/");
    strcpy(mime,buf);
}


/*
   Opens a file using xdg-open
 */
void openFile(char *filepath)
{
    char mime[10];
    getMIME(filepath, mime);
    if(strcmp(mime,"text") == 0)
    {
        char cmd[250];
        sprintf(cmd,"%s %s",editor, filepath);
        endwin();
        system(cmd);
        return;
    }
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        execl("/usr/bin/xdg-open", "xdg-open", filepath, (char *)0);
        exit(1);
    }
}


/*
   Checks if path is in clipboard
 */
int checkClipboard(char *filepath)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[250];
    char temp[250];
    sprintf(temp,"%s", filepath);
    temp[strlen(temp)]='\0';
    if(f == NULL)
    {
        return 0;
    }
    while(fgets(buf, 250, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        if(strcmp(temp,buf) == 0)
            return 1;
    }
    fclose(f);
    return 0;
}


/*
   Writes to clipboard
 */
void writeClipboard(char *filepath)
{
    FILE *f = fopen(clipboard_path,"a+");
    if (f == NULL)
    {
        endwin();
        printf("Error accessing clipboard!\n");
        exit(1);
    }
    fprintf(f, "%s\n", filepath);
    fclose(f);
}


/*
   Removes entry from clipboard
 */
void removeClipboard(char *filepath)
{
    char cmd[250];
    filepath[strlen(filepath)-1] = '\0';
    sprintf(cmd,"sed -i '\\|^%s|d' %s", filepath, clipboard_path);
    system(cmd);
}


/*
   Empties Clipboard
 */
void emptyClipboard()
{
    if( remove(clipboard_path) == -1)
    {
        return;
    }
}


/*
   Gets previews of images
 */
void getImgPreview(char *filepath, int maxy, int maxx)
{
    pid_t pid;
    FILE *fp;
    char buf[64];

    pid = fork();

    if (pid == 0)
    {
        // Stores shell command for getting image dimensions through w3mimgdisplay
        char getdimensions_command[250];
        // Stores shell command for displaying image through w3mimgdisplay
        char imgdisplay_command[250];
        int width;
        int height;

        // Get dimensions of image and store it as a string in `buf`
        sprintf(getdimensions_command,"echo -e \'5;%s' | /usr/lib/w3m/w3mimgdisplay",filepath);
        if((fp = popen(getdimensions_command,"r")) == NULL)
        {
            exit(0);
        }
        while(fgets(buf,64,fp) != NULL){}

        // Get Dimensions from `buf` and store them `width` and `height`
        sscanf(buf,"%d %d", &width, &height);

        // Set appropriate maxx and maxy so that image displays within the preview_win
        maxx = maxx * 5;
        maxy = maxy * 5;

        // Scale the image if dimensions are bigger than preview_win
        if(width > maxx)
        {
            height = height * maxx/width;
            width = maxx;
        }
        if(height > maxy)
        {
            width = width * maxy/height;
            height = maxy;
        }

        // Run the w3mimgdisplay command  with appropriate arguments
        sprintf(imgdisplay_command,"echo -e '0;1;%d;%d;%d;%d;;;;;%s\n4;\n3;' | /usr/lib/w3m/w3mimgdisplay",maxx+maxx/5,8,width,height,filepath);
        system(imgdisplay_command);
        exit(1);
    }
}


/*
   Gets previews of text in files
 */
void getTextPreview(char *filepath, int maxy, int maxx)
{
    // Don't Generate Preview if file size > 50MB
    struct stat st;
    stat(filepath, &st);
    if(st.st_size > 10000000)
        return;
    FILE *fp = fopen(filepath,"r");
    if(fp == NULL)
        return;
    char buf[250];
    int t=0;
    while(fgets(buf, 250, (FILE*) fp))
    {
        wmove(preview_win,t+1,2);
        wprintw(preview_win,"%.*s",maxx-4,buf);
        t++;
    }
    wrefresh(preview_win);

    fclose(fp);
}


/*
   Gets previews of video files
 */
void getVidPreview(char *filepath, int maxy, int maxx)
{
    char buf[250];
    sprintf(temp_dir,"mediainfo \"%s\" > ~/.cache/cfiles/preview",filepath);
    endwin();
    system(temp_dir);
    sprintf(temp_dir,"%s/preview",cache_path);
    sprintf(buf, "less %s", temp_dir);
    system(buf);
    refresh();
}


/*
   Gets previews of archives
 */
void getArchivePreview(char *filepath, int maxy, int maxx)
{
    char buf[250];
    sprintf(temp_dir,"atool -lq \"%s\" > ~/.cache/cfiles/preview",filepath);
    system(temp_dir);
    sprintf(temp_dir,"%s/preview",cache_path);
    getTextPreview(temp_dir, maxy, maxx);
}


/*
   Gets previews of video files (Dummy)
 */
void getDummyVidPreview(char *filepath, int maxy, int maxx)
{
    wmove(preview_win,1,2);
    wprintw(preview_win,"%.*s",maxx-4,"Press i to see info");
    wrefresh(preview_win);
}


/*
   Sets `temp_dir` to filepath and then stores the extension in `last`
 */
void getFileType(char *filepath)
{
    strcpy(temp_dir, filepath);
    getLastToken(".");
}


/*
   Checks `last` for extension and then calls the appropriate preview function
 */
void getPreview(char *filepath, int maxy, int maxx)
{
    getFileType(filepath);
    if(strcasecmp("jpg",last) == 0 || strcasecmp("png",last) == 0 || strcasecmp("gif",last) == 0 || strcasecmp("jpeg",last) == 0 || strcasecmp("mp3",last) == 0)
    {
        getImgPreview(filepath, maxy, maxx);
        clearFlag = 1;
    }
    else if(strcasecmp("mp4",last) == 0 || strcasecmp("mkv",last) == 0 || strcasecmp("avi",last) == 0 || strcasecmp("webm",last) == 0)
        getDummyVidPreview(filepath, maxy, maxx);
    else if(strcasecmp("zip",last) == 0 || strcasecmp("rar",last) == 0 || strcasecmp("cbr",last) == 0 || strcasecmp("cbz",last) == 0)
        getArchivePreview(filepath, maxy, maxx);
    else
        getTextPreview(filepath, maxy, maxx);
}


/*
   Gets path of parent directory
 */
void getParentPath(char *path)
{
    char *p;
    p = strrchr(path,'/');
    path[p-path] = '\0';
    // Parent directory is root
    if(path[0] != '/')
    {
        path[0] = '/';
        path[1] = '\0';
    }
}


/*
   Returns number of files in `char* directory`
 */
int getNumberofFiles(char* directory)
{
    int len=0;
    DIR *pDir;
    struct dirent *pDirent;

    pDir = opendir (directory);
    if (pDir == NULL) {
        return -1;
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        // Skip . and ..
        if( strcmp(pDirent->d_name,".") != 0 && strcmp(pDirent->d_name,"..") != 0 )
        {
            if( pDirent->d_name[0] == '.' )
                if( hiddenFlag == 0 )
                    continue;
            len++;
        }
    }
    closedir (pDir);
    return len;
}


/*
   Stores all the file names in `char* directory` to `char *target[]`
 */
int getFiles(char* directory, char* target[])
{
    int i = 0;
    DIR *pDir;
    struct dirent *pDirent;

    pDir = opendir (directory);
    if (pDir == NULL) {
        return -1;
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        // Skip . and ..
        if( strcmp(pDirent->d_name,".") != 0 && strcmp(pDirent->d_name,"..") != 0 )
        {
            if( pDirent->d_name[0] == '.' )
                if( hiddenFlag == 0 )
                    continue;
            target[i++] = strdup(pDirent->d_name);
        }
    }

    closedir (pDir);
    return 1;
}


/*
   Copy files in clipboard to `present_dir`
 */
void copyFiles(char *present_dir)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[250];
    pid_t pid;
    int status;
    if(f == NULL)
    {
        return;
    }
    endwin();
    while(fgets(buf, 250, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        sprintf(temp_dir,"cp -r -v \"%s\" \"%s\"",buf,present_dir);
        system(temp_dir);
    }
    refresh();
    fclose(f);
}

/*
   Removes files in clipboard
 */
void removeFiles()
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[250];
    if(f == NULL)
    {
        return;
    }
    endwin();
    while(fgets(buf, 250, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        sprintf(temp_dir,"rm -r -v \"%s\"",buf);
        system(temp_dir);
    }
    refresh();
    fclose(f);
    remove(clipboard_path);
}


/*
   Rename files in clipboard
 */
void renameFiles()
{
    // For opening clipboard and temp_clipboard
    FILE *f = fopen(clipboard_path, "r");
    FILE *f2;

    // For storing shell commands
    char cmd[250];

    // Buffers for reading clipboard and temp_clipboard
    char buf[250];
    char buf2[250];

    // Counters used when reading clipboard and copylock_path
    int count = 0;
    int count2 = 0;

    // Make `temp_clipboard`
    sprintf(cmd,"cp %s %s",clipboard_path,temp_clipboard_path);
    system(cmd);
    // Exit curses mode and open temp_clipboard_path in EDITOR
    endwin();
    sprintf(cmd,"%s %s", editor, temp_clipboard_path);
    system(cmd);

    // Open clipboard and temp_clipboard and mv path from clipboard to adjacent entry in temp_clipboard
    while(fgets(buf, 250, (FILE*) f))
    {
        count2=-1;
        f2 = fopen(temp_clipboard_path,"r");
        while(fgets(buf2, 250, (FILE*) f2))
        {
            count2++;
            if(buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            if(count2 == count)
            {
                if(buf2[strlen(buf2)-1] == '\n')
                    buf2[strlen(buf2)-1] = '\0';
                sprintf(cmd,"mv \"%s\" \"%s\"",buf,buf2);
                system(cmd);
            }
        }
        count++;
        fclose(f2);
    }
    fclose(f);
    // Remove clipboard and temp_clipboard
    sprintf(cmd,"rm %s %s",temp_clipboard_path,clipboard_path);
    system(cmd);
    // Start curses mode
    refresh();
}


/*
   Move files in clipboard to `present_dir`
 */
void moveFiles(char *present_dir)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[250];
    int status;
    if(f == NULL)
    {
        return;
    }
    endwin();
    while(fgets(buf, 250, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        sprintf(temp_dir,"mv -f \"%s\" \"%s\"",buf,present_dir);
        system(temp_dir);
    }
    fclose(f);
    refresh();
    remove(clipboard_path);
}


/*
   Creates current_win, preview_win and status_win
 */
void init_windows()
{
    current_win = create_newwin(maxy, maxx/2+3, 0, 0);
    preview_win = create_newwin(maxy, maxx/2 -1, 0, maxx/2 + 1);
    status_win = create_newwin(2, maxx, maxy, 0);
}


/*
   Displays current status in the status bar
 */
void displayStatus()
{
    wmove(status_win,1,1);
    wattron(status_win, COLOR_PAIR(2));
    wprintw(status_win, "(%d/%d)", selection+1, len);
    wprintw(status_win, "\t%s", dir);
    wattroff(status_win, COLOR_PAIR(2));
    wattron(status_win, COLOR_PAIR(3));
    wprintw(status_win, "/%s", selected_file);
    wattroff(status_win, COLOR_PAIR(3));
    wrefresh(status_win);
}


/*
   Displays message on status bar
 */
void displayAlert(char *message)
{
    wclear(status_win);
    wattron(status_win, A_BOLD);
    wprintw(status_win, "\n%s", message);
    wattroff(status_win, A_BOLD);
    wrefresh(status_win);
}


/*
 */
void refreshWindows()
{
    if(SHOW_BORDERS == 1)
    {
        box(current_win,0,0);
        box(preview_win,0,0);
    }
    wrefresh(current_win);
    wrefresh(preview_win);
}


/*
   Checks if some flags are enabled and handles them accordingly
 */
void handleFlags(char** directories)
{
    // Clear the preview_win
    if(clearFlag == 1)
    {
        wclear(preview_win);
        wrefresh(preview_win);
        clearFlag = 0;
    }

    // Select the file in `last` and set `start` accordingly
    if(searchFlag == 1)
    {
        searchFlag = 0;
        last[strlen(last)-1] = '\0';
        for(i=0; i<len; i++)
            if(strcmp(directories[i],last) == 0)
            {
                selection = i;
                break;
            }
        if(len > maxy)
        {
            if(selection > maxy-3)
                start = selection - maxy + 3;
        }

        // Resets everything (Prevents unexpected behaviour after quitting fzf)
        endwin();
        refresh();
    }

    // Select the folder in `last` and set start accordingly
    if(backFlag == 1)
    {
        backFlag = 0;
        for(i=0; i<len; i++)
            if(strcmp(directories[i],last) == 0)
            {
                selection = i;
                break;
            }
        if(len > maxy)
        {
            if(selection > maxy-3)
                start = selection - maxy + 3;
        }
    }
}


///////////////////
// MAIN FUNCTION //
///////////////////

int main(int argc, char* argv[])
{
    // Initialization
    init();
    curses_init();

    // For Storing user keypress
    char ch;

    // Main Loop
    do
    {
        // Get number of files in the home directory
        len = getNumberofFiles(dir);

        // Array of all the files in the current directory
        char* directories[len];
        int status;
        status = getFiles(dir, directories);
        // Sort files by name
        strcpy(sort_dir,dir);
        qsort(directories, len, sizeof (char*), compare);

        // In case the last file is selected and it get's removed or moved
        if(selection > len-1)
        {
            selection = len-1;
        }

        // Check if some flag are set to true and handle them accordingly
        handleFlags(directories);

        // Get Size of terminal
        getmaxyx(stdscr, maxy, maxx);
        // Save last two rows for status_win
        maxy = maxy - 2;

        // Make the two windows side-by-side and make the status window
        init_windows();

        // Print all the elements and highlight the selection
        int t = 0;
        for( i=start; i<len; i++ )
        {
            sprintf(temp_dir,"%s/%s",dir,directories[i]);
            if(i==selection)
                wattron(current_win, A_STANDOUT);
            else
                wattroff(current_win, A_STANDOUT);
            if( is_regular_file(temp_dir) == 0 )
            {
                wattron(current_win, A_BOLD);
                wattron(current_win, COLOR_PAIR(1));
            }
            else
            {
                wattroff(current_win, A_BOLD);
                wattroff(current_win, COLOR_PAIR(1));
            }
            wmove(current_win,t+1,2);
            if( checkClipboard(temp_dir) == 0)
                wprintw(current_win, "%.*s\n", maxx/2+2, directories[i]);
            else
                wprintw(current_win, "* %.*s\n", maxx/2, directories[i]);
            t++;
        }

        // Store name of selected file
        strcpy(selected_file, directories[selection]);

        // Display Status
        displayStatus();

        // Stores files in the selected directory
        char next_dir[250] = "";
        // Stores path of parent directory
        char prev_dir[250] = "";

        // Get path of parent directory
        sprintf(prev_dir,"%s",dir);
        getParentPath(prev_dir);

        // Get path of child directory
        sprintf(next_dir,"%s/%s", dir, directories[selection]);
        // Stores number of files in the child directory
        len_preview = getNumberofFiles(next_dir);
        // Stores files in the child directory
        char* next_directories[len_preview];
        status = getFiles(next_dir, next_directories);

        // Selection is a directory
        strcpy(sort_dir,next_dir);
        if(len_preview > 0)
            qsort(next_directories, len_preview, sizeof (char*), compare);

        // Selection is not a directory
        if(len != 0)
        {
            if(len_preview != -1)
                for( i=0; i<len_preview; i++ )
                {
                    wmove(preview_win,i+1,2);
                    sprintf(temp_dir, "%s/%s", next_dir, next_directories[i]);
                    if( is_regular_file(temp_dir) == 0 )
                    {
                        wattron(preview_win, A_BOLD);
                        wattron(preview_win, COLOR_PAIR(1));
                    }
                    else
                    {
                        wattroff(preview_win, A_BOLD);
                        wattroff(preview_win, COLOR_PAIR(1));
                    }
                    wprintw(preview_win, "%.*s\n", maxx/2 - 3, next_directories[i]);
                }

            // Get Preview of File
            else
            {
                getPreview(next_dir,maxy,maxx/2+2);
            }
        }

        // Disable STANDOUT attribute if enabled
        wattroff(current_win, A_STANDOUT);
        wattroff(current_win, COLOR_PAIR(1));
        wattroff(preview_win, COLOR_PAIR(1));
        // Draw borders and refresh windows
        refreshWindows();

        // For fzf file search
        char cmd[250];
        char buf[250];
        FILE *fp;

        // For two key keybindings
        char secondKey;
        // For taking user confirmation
        char confirm;

        // Keybindings
        switch( ch = wgetch(current_win) ) {
            //Go up
            case 'k':
                selection--;
                selection = ( selection < 0 ) ? 0 : selection;
                // Scrolling
                if(len >= maxy)
                    if(selection <= start + maxy/2)
                    {
                        if(start == 0)
                            wclear(current_win);
                        else
                        {
                            start--;
                            wclear(current_win);
                        }
                    }
                break;

                // Go down
            case 'j':
                selection++;
                selection = ( selection > len-1 ) ? len-1 : selection;
                // Scrolling
                if(len >= maxy)
                    if(selection - 1 > maxy/2)
                    {
                        if(start + maxy - 2 != len)
                        {
                            start++;
                            wclear(current_win);
                        }
                    }
                break;

                // Go to child directory or open file
            case 'l':
                if(len_preview != -1)
                {
                    strcpy(dir, next_dir);
                    selection = 0;
                    start = 0;
                }
                else
                {
                    // Open file
                    openFile(next_dir);
                    clearFlag = 1;
                }
                break;

                // Go up a directory
            case 'h':
                // Copy present directory to temp_dir to work with strtok()
                strcpy(temp_dir, dir);
                strcpy(dir, prev_dir);
                selection = 0;
                start = 0;
                backFlag = 1;
                // Get the last token in `temp_dir` and store it in `last`
                getLastToken("/");
                break;

                // Goto start
            case KEY_START:
                selection = 0;
                start = 0;
                break;

                // Goto end
            case KEY_GOEND:
                selection = len - 1;
                if(len > maxy - 2)
                    start = len - maxy + 2;
                else
                    start = 0;
                break;

                // Go to top of current view
            case KEY_TOP:
                selection = start;
                break;

                // Go to the bottom of current view
            case KEY_BOTTOM:
                if(len >= maxy)
                    selection = start + maxy - 3;
                else
                    selection = len - 1;
                break;

                // Go to the middle of current view
            case KEY_MID:
                if( len >= maxy )
                    selection = start + maxy/2;
                else
                    selection = (len / 2) - 1;
                break;

                // Search using fzf
            case KEY_SEARCHALL:
                sprintf(temp_dir,"cd %s && fzf",info->pw_dir);
                endwin();
                if((fp = popen(temp_dir,"r")) == NULL)
                {
                    exit(0);
                }
                while(fgets(buf,250,fp) != NULL){}
                char path[250];
                sprintf(path, "%s/%s",info->pw_dir,buf);
                // Copy `path` into `temp_dir` to work with strtok.
                //Then fetch the last token from `temp_dir` and store it in `last`.
                strcpy(temp_dir,path);
                getLastToken("/");
                getParentPath(path);
                strcpy(dir,path);
                selection = 0;
                start = 0;
                clearFlag = 1;
                searchFlag = 1;
                break;

                // Search in the same directory
            case KEY_SEARCHDIR:
                if( hiddenFlag == 1 )
                    sprintf(cmd,"cd %s && ls -a | fzf",dir);
                else
                    sprintf(cmd,"cd %s && ls | fzf",dir);
                endwin();
                if((fp = popen(cmd,"r")) == NULL)
                {
                    exit(0);
                }
                while(fgets(buf,250,fp) != NULL){}
                sprintf(path, "%s/%s",info->pw_dir,buf);
                strcpy(temp_dir,path);
                getLastToken("/");
                getParentPath(path);
                strcpy(dir,path);
                selection = 0;
                start = 0;
                clearFlag = 1;
                searchFlag = 1;
                break;

                // Opens bash shell in present directory
            case KEY_SHELL:
                sprintf(temp_dir,"cd \"%s\" && bash",dir);
                endwin();
                system(temp_dir);
                start = 0;
                selection = 0;
                refresh();
                break;

                // Bulk Rename
            case KEY_RENAME:
                if( access( clipboard_path, F_OK ) == -1 )
                {
                    sprintf(temp_dir, "%s/%s",dir,directories[selection]);
                    writeClipboard(temp_dir);
                }
                renameFiles();
                break;

                // Write to clipboard
            case KEY_SEL:
                sprintf(temp_dir, "%s/%s", dir, directories[selection]);
                if (checkClipboard(temp_dir) == 1)
                    removeClipboard(temp_dir);
                else
                    writeClipboard(temp_dir);
                break;

                // Empty Clipboard
            case KEY_EMPTYSEL:
                emptyClipboard();
                break;

                // Copy clipboard contents to present directory
            case KEY_YANK:
                copyFiles(dir);
                break;

                // Moves clipboard contents to present directory
            case KEY_MV:
                moveFiles(dir);
                break;

                // Moves clipboard contents to trash
            case KEY_REMOVEMENU:
                if( fileExists(clipboard_path) == 1 )
                {
                    keys_win = create_newwin(3, maxx, maxy-3, 0);
                    wprintw(keys_win,"Key\tCommand");
                    wprintw(keys_win,"\n%c\tMove to Trash", KEY_TRASH);
                    wprintw(keys_win,"\n%c\tDelete", KEY_DELETE);
                    wrefresh(keys_win);
                    secondKey = wgetch(current_win);
                    delwin(keys_win);
                    if( secondKey == KEY_TRASH )
                        moveFiles(trash_path);
                    else if( secondKey == KEY_DELETE )
                    {
                        displayAlert("Confirm (y/n): ");
                        confirm = wgetch(status_win);
                        if(confirm == 'y')
                            removeFiles();
                        else
                        {
                            displayAlert("ABORTED");
                            sleep(1);
                        }
                    }
                }
                else
                {
                    displayAlert("Select some files first!");
                    sleep(1);
                }
                break;

                // Go to bookmark
            case KEY_BOOKMARK:
                len_bookmarks = getNumberOfBookmarks();
                if( len_bookmarks == -1 )
                {
                    displayAlert("No Bookmarks Found!");
                    sleep(1);
                }
                else
                {
                    keys_win = create_newwin(len_bookmarks+1, maxx, maxy-len_bookmarks, 0);
                    displayBookmarks();
                    secondKey = wgetch(keys_win);
                    openBookmarkDir(secondKey);
                    delwin(keys_win);
                }
                break;

                // Add Bookmark
            case KEY_ADDBOOKMARK:
                displayAlert("Enter Bookmark Key");
                secondKey = wgetch(status_win);
                if( bookmarkExists(secondKey) == 1 )
                {
                    displayAlert("Bookmark Key Exists!");
                    sleep(1);
                }
                else
                {
                    addBookmark(secondKey, dir);
                }
                break;

                // See selection list
            case KEY_VIEWSEL:
                if( access( clipboard_path, F_OK ) != -1 )
                {
                    sprintf(temp_dir,"less %s",clipboard_path);
                    endwin();
                    system(temp_dir);
                    refresh();
                }
                else
                {
                    displayAlert("Selection List is Empty!");
                    sleep(1);
                }
                break;

                // Edit selection list
            case KEY_EDITSEL:
                if( fileExists(clipboard_path) == 1 )
                {
                    sprintf(temp_dir,"%s %s", editor, clipboard_path);
                    endwin();
                    system(temp_dir);
                    refresh();
                }
                else
                {
                    displayAlert("Selection List is Empty!");
                    sleep(1);
                }
                break;

                // View Preview
            case KEY_INFO:
                getVidPreview(next_dir,maxy,maxx/2+2);
                break;

                // Enable/Disable hidden files
            case KEY_TOGGLEHIDE:
                if( hiddenFlag == 1 )
                    hiddenFlag = 0;
                else
                    hiddenFlag = 1;
                start = 0;
                selection = 0;
                break;

                // Run External Script
            case KEY_SCRIPT:
                len_scripts = getNumberofFiles(scripts_path);
                if(len_scripts == 0)
                {
                    displayAlert("No scripts found!");
                    sleep(1);
                }
                else
                {
                    char* scripts[len_scripts];
                    getFiles(scripts_path, scripts);
                    keys_win = create_newwin(len_scripts+1, maxx, maxy-len_scripts, 0);
                    wprintw(keys_win,"%s\t%s\n", "S.No.", "Name");
                    for(i=0; i<len_scripts; i++)
                    {
                        wprintw(keys_win, "%d\t%s\n", i+1, scripts[i]);
                    }
                    secondKey = wgetch(keys_win);
                    int option = secondKey - '0';
                    option--;
                    if(option < len_scripts && option >= 0)
                    {
                        char cmd[250];
                        char path[250];
                        sprintf(temp_dir, "%s/%s", scripts_path, scripts[option]);
                        sprintf(path, "%s/%s", dir, directories[selection]);
                        sprintf(cmd, "%s %s", temp_dir, path);
                        endwin();
                        system(cmd);
                        refresh();
                    }
                    for(i=0; i<len_scripts; i++)
                    {
                        free(scripts[i]);
                    }
                }
                break;

                // Clear Preview Window
            case KEY_RELOAD:
                clearFlag = 1;
                break;
        }

        // Free Memory
        for( i=0; i<len_preview; i++ )
        {
            free(next_directories[i]);
        }
        for( i=0; i<len; i++ )
        {
            free(directories[i]);
        }

    } while( ch != 'q');

    // End curses mode
    endwin();
    return 0;
}
