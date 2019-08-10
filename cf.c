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
#include <limits.h>
#include <locale.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "config.h"


//////////////////////
// GLOBAL VARIABLES //
//////////////////////

// Stores the value of signal most recently raised
int raised_signal=-1;

// To store number of files in directory
int len=0;

// To store number of files in child directory
int len_preview=0;

// To store number of bookmarks
int len_bookmarks=0;

// To store number of scripts
int len_scripts=0;

// To store number of selected files
int selectedFiles = 0;

// Counter variable
int i = 0;

// Stores allocation size for dynamic arrays
int allocSize;

// Name of selected file
char selected_file[NAME_MAX];

// Direcotry to be opened
char* dir = NULL;

// Stores files in the selected directory
char *next_dir = NULL;

// Stores path of parent directory
char *prev_dir = NULL;

// Name of the editor
char *editor = NULL;

// Name of the shell
char *shell = NULL;

// char array to work with strtok() and for other one time use
char *temp_dir = NULL;

// To work with strtok()
char *pch = NULL;

// Stores Home Directory of User
struct passwd *info;

/*
   Base directory to be used for sorting
   `dir` for current_win
   `next_dir` for preview_win
*/
char *sort_dir = NULL;

// Stores the path for the cache directory
char *cache_path = NULL;

// Stores the path for the clipboard file
char *clipboard_path = NULL;

// Stores bookmarks file path
char *bookmarks_path = NULL;

// Stores scripts directory path
char *scripts_path = NULL;

// Stores the path for the temp clipboard file
char *temp_clipboard_path = NULL;

// Stores the path for trash
char *trash_path = NULL;

// Index of currently selected item in `char* directories`
int selection = 0;

// Index to start printing from `directories` array
int start = 0;

// Flag to clear preview_win
int clearFlag = 0;

// Flag to clear preview_win for images
int clearFlagImg = 0;

// Flag is set to 1 when returning from `fzf`
int searchFlag = 0;

// Flag is set to 1 when user goes up a directory
int backFlag = 0;

// Flag to display hidden files
int hiddenFlag = SHOW_HIDDEN;

// Flag to display borders
int bordersFlag = SHOW_BORDERS;

// Flag to display PDF previews
int pdfflag = SHOW_PDF_PREVIEWS;

// Stores the last token in the path. For eg, it will store 'a' is path is /b/a
char *last = NULL;

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
   Displays current status in the status bar
*/
void displayStatus()
{
    wmove(status_win,1,0);
    wattron(status_win, COLOR_PAIR(2));
    if( SHOW_SELECTION_COUNT == 1 )
    {
        wprintw(status_win,"[%d] ", selectedFiles);
    }
    wprintw(status_win, "(%d/%d)", selection+1, len);
    wprintw(status_win, " %s", dir);
    wattroff(status_win, COLOR_PAIR(2));
    wattron(status_win, COLOR_PAIR(3));
    if(strcasecmp(dir,"/") == 0)
        wprintw(status_win, "%s", selected_file);
    else
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
    Signal Handler. Sets `raised_signal` to `signal`
*/
static void cb_sig(int signal)
{
    if (signal == SIGUSR1)
        raised_signal = SIGUSR1;
    else if (signal == SIGCHLD)
        raised_signal = SIGCHLD;
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
    Sets value of `selectedFiles`
*/
void setSelectionCount()
{
    FILE *fp = fopen(clipboard_path, "r");
    if( fp == NULL )
    {
        selectedFiles = 0;
        return;
    }
    char buf[PATH_MAX];
    int num = 0;
    while(fgets(buf, PATH_MAX, (FILE*) fp))
    {
        num++;
    }
    selectedFiles = num;
    fclose(fp);
}


/*
   Initializes the program
   Sets the relevant file paths
*/
void init(int argc, char* argv[])
{
    // Set Locale (For wide characters)
    setlocale(LC_ALL, "");

    // Get UID of user
    uid_t uid = getuid();
    // Get home directory of user from UID
    info = getpwuid(uid);

    // Set the shell
    if( getenv("SHELL") == NULL)
    {
        shell = malloc(10);
        if(shell == NULL)
        {
            printf("%s\n", "Couldn't initialize shell");
            exit(1);
        }
        snprintf(shell, 10, "%s", "/bin/bash");
    }
    else
    {
        allocSize = snprintf(NULL, 0, "%s", getenv("SHELL"));
        shell = malloc(allocSize + 1);
        if(shell == NULL)
        {
            printf("%s\n", "Couldn't initialize shell");
            exit(1);
        }
        snprintf(shell, allocSize+1, "%s", getenv("SHELL"));
    }

    // Set the editor
    if( getenv("EDITOR") == NULL)
    {
        editor = malloc(4);
        if(editor == NULL)
        {
            printf("%s\n", "Couldn't initialize editor");
            exit(1);
        }
        snprintf(editor, 4, "%s", "vim");
    }
    else
    {
        allocSize = snprintf(NULL, 0, "%s", getenv("EDITOR"));
        editor = malloc(allocSize + 1);
        if(editor == NULL)
        {
            printf("%s\n", "Couldn't initialize editor");
            exit(1);
        }
        snprintf(editor, allocSize+1, "%s", getenv("EDITOR"));
    }

    // Get the cache directory path
    struct stat st = {0};
    if( getenv("XDG_CONFIG_HOME") == NULL)
    {
        allocSize = snprintf(NULL,0,"%s/.config/cfiles",info->pw_dir);
        cache_path = malloc(allocSize+1);
        if(cache_path == NULL)
        {
            printf("%s\n", "Couldn't initialize cache path");
            exit(1);
        }
        snprintf(cache_path,allocSize+1,"%s/.config/cfiles",info->pw_dir);
    }
    else
    {
        allocSize = snprintf(NULL,0,"%s/cfiles",getenv("XDG_CONFIG_HOME"));
        cache_path = malloc(allocSize+1);
        if(cache_path == NULL)
        {
            printf("%s\n", "Couldn't initialize cache path");
            exit(1);
        }
        snprintf(cache_path,allocSize+1,"%s/cfiles",getenv("XDG_CONFIG_HOME"));
    }
    // Make the cache directory
    if (stat(cache_path, &st) == -1) {
        mkdir(cache_path, 0751);
    }

    // Set the path for the clipboard file
    allocSize = snprintf(NULL,0,"%s/clipboard",cache_path);
    clipboard_path = malloc(allocSize+1);
    if(clipboard_path == NULL)
    {
        printf("%s\n", "Couldn't initialize clipboard path");
        exit(1);
    }
    snprintf(clipboard_path,allocSize+1,"%s/clipboard",cache_path);

    // Set the path for the bookmarks file
    allocSize = snprintf(NULL,0,"%s/bookmarks",cache_path);
    bookmarks_path = malloc(allocSize+1);
    if(bookmarks_path == NULL)
    {
        printf("%s\n", "Couldn't initialize bookmarks path");
        exit(1);
    }
    snprintf(bookmarks_path,allocSize+1,"%s/bookmarks",cache_path);

    // Set the path for the scripts directory
    allocSize = snprintf(NULL,0,"%s/scripts",cache_path);
    scripts_path = malloc(allocSize+1);
    if(scripts_path == NULL)
    {
        printf("%s\n", "Couldn't initialize scripts path");
        exit(1);
    }
    snprintf(scripts_path,allocSize+1,"%s/scripts",cache_path);

    // Set the path for the temp clipboard file
    allocSize = snprintf(NULL,0,"%s/clipboard.tmp",cache_path);
    temp_clipboard_path = malloc(allocSize+1);
    if(temp_clipboard_path == NULL)
    {
        printf("%s\n", "Couldn't initialize temp clipboard path path");
        exit(1);
    }
    snprintf(temp_clipboard_path,allocSize+1,"%s/clipboard.tmp",cache_path);

    // Set the path for trash
    allocSize = snprintf(NULL,0,"%s/.local/share/Trash/files",info->pw_dir);
    trash_path = malloc(allocSize+1);
    if(trash_path == NULL)
    {
        printf("%s\n", "Couldn't initialize trash path");
        exit(1);
    }
    snprintf(trash_path,allocSize+1,"%s/.local/share/Trash/files",info->pw_dir);

    if (stat(scripts_path, &st) == -1) {
        mkdir(scripts_path, 0751);
    }

    // Set dir as current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        allocSize = snprintf(NULL,0,"%s",cwd);
        dir = malloc(allocSize+1);
        if(dir == NULL)
        {
            printf("%s\n", "Couldn't initialize dir");
            exit(1);
        }
        snprintf(dir,allocSize+1,"%s",cwd);
    }
    else
    {
        printf("Couldn't open current directory");
        exit(1);
    }

    // Path of directory was specified
    if(argc == 2)
    {
        // Absolute path
        if(argv[1][0] == '/')
        {
            free(dir);
            allocSize = snprintf(NULL, 0, "%s", argv[1]);
            dir = malloc(allocSize+1);
            if(dir == NULL)
            {
                printf("%s\n", "Couldn't initialize dir");
                exit(1);
            }
            // Remove trailing forward slash, if exists, from path
            if(strlen(argv[1]) > 1 && argv[1][strlen(argv[1])-1] == '/')
                argv[1][strlen(argv[1])-1] = '\0';
            snprintf(dir,allocSize+1,"%s", argv[1]);
        }
        // Relative path
        else
        {
            char *temp;
            int temp_size;
            temp_size = snprintf(NULL, 0, "%s", argv[1]);
            allocSize = snprintf(NULL, 0, "%s", dir);
            temp = malloc(temp_size + allocSize + 2);
            snprintf(temp, temp_size + allocSize + 2, "%s/%s", dir, argv[1]);
            free(dir);
            dir = malloc(temp_size + allocSize + 2);
            if(dir == NULL)
            {
                printf("%s\n", "Couldn't initialize dir");
                exit(1);
            }
            snprintf(dir, temp_size + allocSize + 2, "%s", temp);
            free(temp);
        }
    }

    // Excess arguments given
    else if(argc > 2)
    {
        printf("Incorrect Usage!\n");
        exit(1);
    }

    // Set value of selectedFiles
    if( SHOW_SELECTION_COUNT == 1 )
    {
        setSelectionCount();
    }
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
        free(last);
        last = strdup(pch);
        if(last == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }
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
    char *buf;
    buf = malloc(PATH_MAX);
    if(buf == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    int num = 0;
    while(fgets(buf, PATH_MAX, (FILE*) fp))
    {
        num++;
    }
    free(buf);
    fclose(fp);
    return num;
}


/*
   Displays the bookmarks in `keys_win`
*/
void displayBookmarks()
{
    FILE *fp = fopen(bookmarks_path, "r");
    char *buf;
    if(fp == NULL)
    {
        endwin();
        printf("Couldn't Open Bookmarks File!\n");
        exit(1);
    }
    buf = malloc(PATH_MAX);
    if(buf == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    wprintw(keys_win,"Key\tPath\n");
    while(fgets(buf, PATH_MAX, (FILE*) fp))
    {
        wprintw(keys_win, "%c", buf[0]);

        // Reallocate `tempdir`
        free(temp_dir);
        allocSize = snprintf(NULL,0,"%s",buf);
        temp_dir = malloc(allocSize+1);
        if(temp_dir == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }

        strncpy(temp_dir, buf + 2, strlen(buf)-2);
        strtok(temp_dir,"\n");
        wprintw(keys_win, "\t%s\n", temp_dir);
    }
    free(buf);
    fclose(fp);
}


/*
    Replaces `a` with `b` in `str`
*/
char* replace(char* str, char* a, char* b)
{
    int len  = strlen(str);
    int lena = strlen(a);
    int lenb = strlen(b);
    for (char* p = str; (p = strstr(p, a)); ++p)
    {
        if (lena != lenb) // shift end as needed
            memmove(p+lenb, p+lena, len - (p - str) + lenb);
        memcpy(p, b, lenb);
    }
    return str;
}


/*
   Opens the bookmark denoted by `secondKey`
*/
void openBookmarkDir(char secondKey)
{
    FILE *fp = fopen(bookmarks_path, "r");
    char *buf;
    if(fp == NULL)
    {
        endwin();
        printf("Couldn't Open Bookmarks File!\n");
        exit(1);
    }
    buf = malloc(PATH_MAX);
    if(buf == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    while(fgets(buf, PATH_MAX, (FILE*) fp))
    {
        if(buf[0] == secondKey)
        {
            // Reallocate `temp_dir`
            free(temp_dir);
            allocSize = snprintf(NULL,0,"%s",buf);
            temp_dir = malloc(allocSize+1);
            if(temp_dir == NULL)
            {
                endwin();
                printf("%s\n", "Couldn't allocate memory!");
                exit(1);
            }
            strncpy(temp_dir, buf + 2, strlen(buf)-2);
            strtok(temp_dir,"\n");
            replace(temp_dir,"//","\n");
            if( fileExists(temp_dir) == 1 )
            {
                // Reallocate `dir`
                free(dir);
                allocSize = snprintf(NULL,0,"%s",temp_dir);
                dir = malloc(allocSize+1);
                if(dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(dir,allocSize+1,"%s",temp_dir);
            }
            start = 0;
            selection = 0;
            break;
        }
    }
    free(buf);
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
    char *buf;
    buf = malloc(PATH_MAX);
    if(buf == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    while(fgets(buf, PATH_MAX, (FILE*) fp))
    {
        if(buf[0] == bookmark)
        {
            fclose(fp);
            free(buf);
            return 1;
        }
    }
    free(buf);
    fclose(fp);
    return 0;
}


/*
   Adds new bookmark
*/
void addBookmark(char bookmark, char *path)
{
    FILE *fp = fopen(bookmarks_path, "a+");
    if(fp == NULL)
    {
        endwin();
        printf("Couldn't Open Bookmarks File!\n");
        exit(1);
    }
    int allocSize = snprintf(NULL, 0, "%s", path);
    path = realloc(path, allocSize+2);
    char *temp = strdup(path);
    if(temp == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    fprintf(fp,"%c:%s\n", bookmark, replace(temp,"\n","//"));
    free(temp);
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
    char *temp_filepath1 = NULL;
    char *temp_filepath2 = NULL;

    // Allocate Memory and Generate full paths
    allocSize = snprintf(NULL,0,"%s/%s", sort_dir, *(char **)a);
    temp_filepath1 = malloc(allocSize+1);
    if(temp_filepath1 == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp_filepath1,PATH_MAX,"%s/%s", sort_dir, *(char **)a);
    allocSize = snprintf(NULL,0,"%s/%s", sort_dir, *(char **)b);
    temp_filepath2 = malloc(allocSize+1);
    if(temp_filepath2 == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp_filepath2,PATH_MAX,"%s/%s", sort_dir, *(char **)b);

    if(is_regular_file(temp_filepath1) == 0 && is_regular_file(temp_filepath2) == 1)
    {
        free(temp_filepath1);
        free(temp_filepath2);
        return -1;
    }
    else if(is_regular_file(temp_filepath1) == 1 && is_regular_file(temp_filepath2) == 0)
    {
        free(temp_filepath1);
        free(temp_filepath2);
        return 1;
    }
    else
    {
        free(temp_filepath1);
        free(temp_filepath2);
        return strcasecmp(*(char **)a, *(char **)b);
    }
}


/*
   Gets file MIME
*/
void getMIME(char *filepath, char mime[50])
{
    char buf[50];
    FILE *fp;
    int fd;
    pid_t pid;

    // Reallocate `temp_dir` and store path to preview file
    free(temp_dir);
    allocSize = snprintf(NULL,0,"%s/mime",cache_path);
    temp_dir = malloc(allocSize+1);
    if(temp_dir == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp_dir,allocSize+1,"%s/mime",cache_path);

    // Remove the preview file
    remove(temp_dir);

    // Create a child process to run command and store output in preview file
    pid = fork();
    if( pid == 0 )
    {
        fd = open(temp_dir, O_CREAT | O_WRONLY, 0755);
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(fd, 1);
        dup2(null_fd, 2);
        execlp("xdg-mime","xdg-mime","query","filetype",filepath,(char *)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid,&status,0);
        // Open preview file to read output
        fp = fopen(temp_dir, "r");
        if(fp == NULL)
        {
            return;
        }
        while(fgets(buf,50,(FILE *)fp) != NULL){}
        fclose(fp);
        if(strlen(buf) >= 3 && buf[0] == 'a' && buf[1] == 'p' && buf[2] == 'p')
        {
            snprintf(mime, 50, "%s", buf);
        }
        else
        {
            strtok(buf,"/");
            snprintf(mime,50,"%s",buf);
        }
    }
}


/*
   Opens a file using FILE_OPENER
*/
void openFile(char *filepath)
{
    char mime[50];
    getMIME(filepath, mime);
    if(strcmp(mime,"text") == 0)
    {
        endwin();
        // Make a child process to edit file
        pid_t pid;
        pid = fork();
        if (pid == 0)
        {
            execlp(editor, editor, filepath, (char *)0);
            exit(1);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
            return;
        }
    }
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        int null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd,2);
        execlp(FILE_OPENER, FILE_OPENER, filepath, (char *)0);
        exit(1);
    }
}


/*
   Checks if path is in clipboard
*/
int checkClipboard(char *filepath)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[PATH_MAX];
    char *temp = NULL;
    // Allocate Memory to `temp`
    allocSize = snprintf(NULL,0,"%s", filepath);
    temp = malloc(allocSize+1);
    if(temp == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp, allocSize+1, "%s", filepath);
    temp[strlen(temp)]='\0';
    if(f == NULL)
    {
        free(temp);
        return 0;
    }
    replace(temp,"\n","//");
    while(fgets(buf, PATH_MAX, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        if(strcmp(temp,buf) == 0)
        {
            free(temp);
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    free(temp);
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
    FILE *f1;
    FILE *f2;
    char buf[PATH_MAX];

    // Create `temp_clipboard` without `filepath`
    f1 = fopen(clipboard_path,"r");
    f2 = fopen(temp_clipboard_path,"a+");
    if(f1 == NULL)
    {
        endwin();
        printf("Couldn't Open Clipboard File!\n");
        exit(1);
    }
    if(f2 == NULL)
    {
        endwin();
        printf("Couldn't Create Temporary Clipboard File!\n");
        exit(1);
    }
    while(fgets(buf, PATH_MAX, (FILE*)f1))
    {
        buf[strlen(buf)-1] = '\0';
        if(strcasecmp(buf, filepath) != 0)
            fprintf(f2, "%s\n", buf);
    }
    fclose(f1);
    fclose(f2);

    // Create a child process to replace clipboard_path with temp_clipboard_path
    pid_t pid;
    pid = fork();
    if( pid == 0 )
    {
        execlp("mv", "mv", temp_clipboard_path, clipboard_path, (char *)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
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
    pid = fork();

    if (pid == 0)
    {
        // For storing arguments to the DISPLAYIMG script
        char arg1[5];
        char arg2[5];
        char arg3[5];
        char arg4[5];
        // Convert numerical arguments to string
        snprintf(arg1,5,"%d",maxx);
        snprintf(arg2,5,"%d",2);
        snprintf(arg3,5,"%d",maxx-6);
        snprintf(arg4,5,"%d",maxy);
        // Run the displayimg script with appropriate arguments
        execl(DISPLAYIMG,DISPLAYIMG,arg1,arg2,arg3,arg4,filepath,(char *)NULL);
        exit(1);
    }
}


/*
    Gets previews of PDF Documents
*/
void getPDFPreview(char *filepath, int maxy, int maxx)
{
    // Set the signal handler
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = cb_sig;

    if(sigaction(SIGUSR1, &act, NULL) == -1) 
        printf("unable to handle siguser1\n");
    if (sigaction(SIGCHLD, &act, NULL) == -1) 
        printf("unable to handle sigchild\n");

    // Target name of the PDF
    char imgout[] = "/tmp/prev.jpg";

    pid_t pid;
    pid = fork();

    if(pid == 0)
    {
        execlp("pdftoppm","pdftoppm","-l","1","-jpeg",filepath,"-singlefile", "/tmp/prev", (char *)NULL);
        exit(1);
    }
    else
    {
        //displayAlert("WORKING... PRESS ANY KEY TO ABORT");
        if (ERR == getch())
        {}
        else
        {
            raise(SIGUSR1);
        }
        if (raised_signal == SIGUSR1)
        {
            kill(pid, SIGINT);
            //displayStatus();
            return;
        }
        else if (raised_signal == SIGCHLD)
        {
            getImgPreview(imgout, maxy, maxx);
            clearFlagImg = 1;
            //displayStatus();
            return;
        }
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

    // Buffer for file reading
    char buf[250];

    // Set path to preview file
    char *preview_path = NULL;
    allocSize = snprintf(NULL, 0, "%s/preview", cache_path);
    preview_path = malloc(allocSize+1);
    if(preview_path == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(preview_path, allocSize+1, "%s/preview", cache_path);

    // Generate Hex Preview if file is a binary
    char mime[50];
    getMIME(filepath, mime);
    if(strcasecmp(mime,"application/x-executable\n") == 0 || strcasecmp(mime,"application/x-sharedlib\n") == 0 || strcasecmp(mime,"application/x-pie-executable\n") == 0)
    {
        remove(preview_path);
        pid_t pid = fork();
        if(pid == 0)
        {
            int fd = open(preview_path, O_CREAT | O_WRONLY, 0755);
            dup2(fd,1);
            execlp("hexdump","hexdump",filepath,(char*)0);
            exit(1);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
            FILE *fp = fopen(preview_path, "r");
            if(fp == NULL)
            {
                free(preview_path);
                return;
            }
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
        free(preview_path);
        return;
    }

    // Copy file to `preview_path`
    pid_t pid = fork();
    if(pid == 0)
    {
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd,2);
        execlp("cp","cp",filepath,preview_path,(char*)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }

    // Print the preview
    FILE *fp = fopen(filepath,"r");
    if(fp == NULL)
    {
        printf("Error with preview\n");
        free(preview_path);
        return;
    }
    int t=0;
    while(fgets(buf, 250, (FILE*) fp))
    {
        wmove(preview_win,t+1,2);
        wprintw(preview_win,"%.*s",maxx-4,buf);
        t++;
    }
    wrefresh(preview_win);
    free(preview_path);
    fclose(fp);
}


/*
   Gets previews of archives
*/
void getArchivePreview(char *filepath, int maxy, int maxx)
{
    pid_t pid;
    int fd;
    int null_fd;

    // Reallocate `temp_dir` and store path to preview file
    char *preview_path = NULL;
    allocSize = snprintf(NULL, 0, "%s/preview", cache_path);
    preview_path = malloc(allocSize+1);
    if(preview_path == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(preview_path, allocSize+1, "%s/preview", cache_path);

    // Set the signal handler
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = cb_sig;

    if(sigaction(SIGUSR1, &act, NULL) == -1) 
        printf("unable to handle siguser1\n");
    if (sigaction(SIGCHLD, &act, NULL) == -1) 
        printf("unable to handle sigchild\n");

    // Create a child process to run "atool -lq filepath > ~/.config/cfiles/preview"
    pid = fork();
    if( pid == 0 )
    {
        remove(preview_path);
        fd = open(preview_path, O_CREAT | O_WRONLY, 0755);
        null_fd = open("/dev/null", O_WRONLY);
        // Redirect stdout
        dup2(fd, 1);
        // Redirect errors to /dev/null
        dup2(null_fd, 2);
        execlp("atool", "atool", "-lq", filepath, (char *)0);
        exit(1);
    }
    else
    {
        //displayAlert("WORKING... PRESS ANY KEY TO ABORT");
        if (ERR == getch())
        {}
        else
        {
            raise(SIGUSR1);
        }
        if (raised_signal == SIGUSR1)
        {
            kill(pid, SIGINT);
            //displayStatus();
            free(preview_path);
            return;
        }
        else if (raised_signal == SIGCHLD)
        {
            //displayStatus();
            getTextPreview(preview_path, maxy, maxx);
            free(preview_path);
            return;
        }
    }
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
    free(temp_dir);
    allocSize = snprintf(NULL,0,"%s",filepath);
    temp_dir = malloc(allocSize+1);
    if(temp_dir == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp_dir,allocSize+1,"%s", filepath);
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
        clearFlagImg = 1;
    }
    else if(strcasecmp("mp4",last) == 0 || strcasecmp("mkv",last) == 0 || strcasecmp("avi",last) == 0 || strcasecmp("webm",last) == 0)
        getDummyVidPreview(filepath, maxy, maxx);
    else if(strcasecmp("zip",last) == 0 || strcasecmp("rar",last) == 0 || strcasecmp("tar",last) == 0 || strcasecmp("bz2",last) == 0 || strcasecmp("gz",last) == 0 || strcasecmp("xz",last) == 0 || strcasecmp("cbr",last) == 0 || strcasecmp("cbz",last) == 0)
        getArchivePreview(filepath, maxy, maxx);
    else if(strcasecmp("pdf",last) == 0 && pdfflag == 1)
        getPDFPreview(filepath, maxy, maxx);
    else
        getTextPreview(filepath, maxy, maxx);
}


/*
   Gets previews of video files
*/
void getVidPreview(char *filepath, int maxy, int maxx)
{
    pid_t pid;
    int fd;

    // Reallocate `temp_dir` and store path to preview file
    free(temp_dir);
    allocSize = snprintf(NULL,0,"%s/preview",cache_path);
    temp_dir = malloc(allocSize+1);
    if(temp_dir == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp_dir,allocSize+1,"%s/preview",cache_path);

    endwin();

    // Create a child process to run "mediainfo filepath > ~/.config/cfiles/preview"
    pid = fork();
    if( pid == 0 )
    {
        remove(temp_dir);
        fd = open(temp_dir, O_CREAT | O_WRONLY, 0755);
        // Redirect stdout
        dup2(fd, 1);
        if(is_regular_file(filepath) == 0)
            execlp("du", "du", "-s", "-h", filepath, (char *)0);
        else
            execlp("mediainfo", "mediainfo", filepath, (char *)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        pid = fork();
        if( pid == 0 )
        {
            execlp("less", "less", temp_dir, (char *)0);
            exit(1);
        }
        else
        {
            waitpid(pid, &status, 0);
        }

        refresh();
    }
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
    if(path[0] == '\0')
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
            target[i] = strdup(pDirent->d_name);
            if(target[i++] == NULL)
            {
                endwin();
                printf("%s\n", "Couldn't allocate memory!");
                exit(1);
            }
        }
    }

    closedir (pDir);
    return 1;
}


/*
    Gets write permissions
    Returns 1 is `path` is writable
*/
int getWritePermissions(char *path)
{
    if( access(path, W_OK) == 0 )
        return 1;
    else
        return 0;
}


/*
   Copy files in clipboard to `present_dir`
*/
void copyFiles(char *present_dir)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[PATH_MAX];
    int flag;
    if(f == NULL)
    {
        return;
    }

    // Check if `dir` is writable
    if(getWritePermissions(dir) == 1)
        flag = 0;
    else
        flag = 1;

    endwin();
    while(fgets(buf, PATH_MAX, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        replace(buf,"//","\n");
        // Create a child process to copy file
        pid_t pid = fork();
        if(pid == 0)
        {
            if(flag == 0)
                execlp("cp", "cp", "-r", "-v", buf, present_dir, (char *)0);
            else
                execlp("sudo", "sudo", "cp", "-r", "-v", buf, present_dir, (char *)0);
            exit(1);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
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
    char buf[PATH_MAX];
    int flag;
    if(f == NULL)
    {
        return;
    }
    endwin();
    while(fgets(buf, PATH_MAX, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        replace(buf,"//","\n");
        if( getWritePermissions(buf) == 1 )
            flag = 0;
        else
            flag = 1;
        // Create a child process to remove file
        pid_t pid = fork();
        if(pid == 0)
        {
            if( flag == 0 )
                execlp("rm", "rm", "-r", "-f", "-v", buf, (char *)0);
            else
                execlp("sudo", "sudo", "rm", "-r", "-f", "-v", buf, (char *)0);
            exit(1);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
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
    // For storing pid of children
    pid_t pid;

    // For opening clipboard and temp_clipboard
    FILE *f = fopen(clipboard_path, "r");
    FILE *f2;

    // For storing shell commands
    char *cmd = NULL;

    // Buffers for reading clipboard and temp_clipboard
    char buf[PATH_MAX];
    char buf2[PATH_MAX];

    // Counters used when reading clipboard and copylock_path
    int count = 0;
    int count2 = 0;

    // Make a child process to create `temp_clipboard`
    pid = fork();
    if( pid == 0 )
    {
        execlp("cp", "cp", clipboard_path, temp_clipboard_path, (char *)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }

    // Exit curses mode to open temp_clipboard_path in EDITOR
    endwin();

    // Allocate `cmd` to store full path of editor
    allocSize = snprintf(NULL,0,"%s", editor);
    cmd = malloc(allocSize + 1);
    if(cmd == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(cmd,allocSize+1,"%s", editor);

    // Create a child process to edit temp_clipboard
    pid = fork();
    if( pid == 0 )
    {
        execlp(cmd, cmd, temp_clipboard_path, (char *)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        free(cmd);
    }

    // Open clipboard and temp_clipboard and mv path from clipboard to adjacent entry in temp_clipboard
    while(fgets(buf, PATH_MAX, (FILE*) f))
    {
        count2=-1;
        f2 = fopen(temp_clipboard_path,"r");
        while(fgets(buf2, PATH_MAX, (FILE*) f2))
        {
            count2++;
            if(buf[strlen(buf)-1] == '\n')
                buf[strlen(buf)-1] = '\0';
            if(count2 == count)
            {
                if(buf2[strlen(buf2)-1] == '\n')
                    buf2[strlen(buf2)-1] = '\0';

                replace(buf2,"//","\n");
                replace(buf,"//","\n");

                // Create a child process to rename
                pid = fork();
                if( pid == 0 )
                {
                    execlp("mv", "mv", buf, buf2, (char *)0);
                    exit(1);
                }
                else
                {
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
        }
        count++;
        fclose(f2);
    }
    fclose(f);

    // Remove clipboard and temp_clipboard
    remove(clipboard_path);
    remove(temp_clipboard_path);

    // Start curses mode
    refresh();
}


/*
   Move files in clipboard to `present_dir`
*/
void moveFiles(char *present_dir)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[PATH_MAX];
    int flag;
    if(f == NULL)
    {
        return;
    }

    if( getWritePermissions(dir) == 1 )
        flag = 0;
    else
        flag = 1;

    endwin();
    while(fgets(buf, PATH_MAX, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        replace(buf,"//","\n");
        // Create a child process to move file
        pid_t pid = fork();
        if(pid == 0)
        {
            if(flag == 0)
                execlp("mv", "mv", "-v", buf, present_dir, (char *)0);
            else
                execlp("sudo", "sudo", "mv", "-v", buf, present_dir, (char *)0);
            exit(1);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
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
    current_win = create_newwin(maxy, maxx/2+2, 0, 0);
    preview_win = create_newwin(maxy, maxx/2-1, 0, maxx/2+1);
    status_win = create_newwin(2, maxx, maxy, 0);
    keypad(current_win, TRUE);
}


/*
    Refresh ncurses windows
*/
void refreshWindows()
{
    if(bordersFlag == 1)
    {
        box(current_win,0,0);
        box(preview_win,0,0);
    }
    wrefresh(current_win);
    wrefresh(preview_win);
}


/*
    Clears Image
*/
void clearImg()
{
        // Store arguments for CLEARIMG script
        char arg1[5];
        char arg2[5];
        char arg3[5];
        char arg4[5];

        // Convert numerical arguments to strings
        snprintf(arg1,5,"%d",maxx/2+2);
        snprintf(arg2,5,"%d",2);
        snprintf(arg3,5,"%d",maxx/2-3);
        snprintf(arg4,5,"%d",maxy+5);

        pid_t pid;
        pid = fork();

        if(pid == 0)
        {
            execl(CLEARIMG,CLEARIMG,arg1,arg2,arg3,arg4,(char *)NULL);
            exit(1);
        }
}


/*
    Opens `preview` file in pager
*/
void viewPreview()
{
    pid_t pid;
    char *preview_path = NULL;

    // Store path of preview file in `preview_path`
    allocSize = snprintf(NULL, 0, "%s/preview", cache_path);
    preview_path = malloc(allocSize + 1);
    if(preview_path == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(preview_path, allocSize+1, "%s/preview", cache_path);

    // Exit curses mode
    endwin();

    // Start a child process to display preview
    pid = fork();
    if(pid == 0)
    {
        execlp("less", "less", preview_path, (char *)0);
        exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }

    // Restart curses mode
    refresh();
    free(preview_path);
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

    // Clear the preview_win for images
    if(clearFlagImg == 1)
    {
        clearImg();
        clearFlagImg = 0;
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


/*
    Scrolls Up
*/
void scrollUp()
{
    selection--;
    selection = ( selection < 0 ) ? 0 : selection;
    // Scrolling
    if(len >= maxy-1)
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
}


/*
    Scrolls Down
*/
void scrollDown()
{
    selection++;
    selection = ( selection > len-1 ) ? len-1 : selection;
    // Scrolling
    if(len >= maxy-1)
        if(selection - 1 > maxy/2)
        {
            if(start + maxy - 2 != len)
            {
                start++;
                wclear(current_win);
            }
        }
}


/*
    Goes to next page
*/
void nextPage()
{
    if(selection + maxy-1 < len)
    {
        start = start + maxy-1;
        selection = selection + maxy-1;
        wclear(current_win);
    }
}


/*
    Goes to previous page
*/
void prevPage()
{
    start = ( start - maxy-1 > 0 ) ? start - maxy-1 : 0;
    selection = ( selection - maxy-1 > 0 ? selection - maxy-1 : 0);
    wclear(current_win);
}


/*
    Goes to child directory or opens a file
*/
void goForward()
{
    if(len_preview != -1)
    {
        free(dir);
        allocSize = snprintf(NULL,0,"%s",next_dir);
        dir = malloc(allocSize+1);
        if(dir == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }
        snprintf(dir,allocSize+1,"%s",next_dir);
        selection = 0;
        start = 0;
    }
    else
    {
        // Open file
        openFile(next_dir);
        clearFlag = 1;
    }
}


/*
    Goes to parent directory
*/
void goBack()
{
    // Reallocate `temp_dir` and Copy present directory to temp_dir to work with strtok()
    free(temp_dir);
    allocSize = snprintf(NULL,0,"%s",dir);
    temp_dir = malloc(allocSize+1);
    if(temp_dir == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(temp_dir,allocSize+1,"%s",dir);

    // Reallocate `dir` and copy `prev_dir` to `dir`
    free(dir);
    allocSize = snprintf(NULL,0,"%s",prev_dir);
    dir = malloc(allocSize+1);
    if(dir == NULL)
    {
        endwin();
        printf("%s\n", "Couldn't allocate memory!");
        exit(1);
    }
    snprintf(dir,allocSize+1,"%s",prev_dir);

    // Set appropriate flags
    selection = 0;
    start = 0;
    backFlag = 1;

    // Get the last token in `temp_dir` and store it in `last`
    getLastToken("/");
}


///////////////////
// MAIN FUNCTION //
///////////////////

int main(int argc, char* argv[])
{
    // Initialization
    init(argc, argv);
    curses_init();

    // For Storing user keypress
    int ch;

    // Main Loop
    do
    {
        // Stores length of VLA `directories`
        int temp_len;

        // Get number of files in the home directory
        len = getNumberofFiles(dir);

        // Set temp_len to 1 if `len` is less than 0
        if(len <= 0)
            temp_len = 1;
        else
            temp_len = len;

        // Array of all the files in the current directory
        char* directories[temp_len];

        // Store files in `dir` in the `directories` array
        int status;
        status = getFiles(dir, directories);

        if( status == -1 )
        {
            endwin();
            printf("Couldn't open \'%s\'", dir);
            exit(1);
        }

        // Sort files by name
        allocSize = snprintf(NULL,0,"%s",dir);
        sort_dir = malloc(allocSize+1);
        if(sort_dir == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }
        strncpy(sort_dir,dir,allocSize+1);

        if( len > 0 )
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
            if(t == maxy -1)
                break;
            free(temp_dir);
            allocSize = snprintf(NULL,0,"%s/%s",dir,directories[i]);
            temp_dir = malloc(allocSize + 1);
            if(temp_dir == NULL)
            {
                endwin();
                printf("%s\n", "Couldn't allocate memory!");
                exit(1);
            }
            snprintf(temp_dir,allocSize+1,"%s/%s",dir,directories[i]);
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
                wprintw(current_win, "%.*s\n", maxx/2, directories[i]);
            else
                wprintw(current_win, "* %.*s\n", maxx/2-3, directories[i]);
            t++;
        }

        // Check if selection is -1
        if(selection == -1)
        {
            selection = 0;
            directories[0] = malloc(6);
            if(directories[0] == NULL)
            {
                endwin();
                printf("%s\n", "Couldn't allocate memory!");
                exit(1);
            }
            snprintf(directories[0],6,"%s","Empty");
        }
        // Store name of selected file
        snprintf(selected_file,NAME_MAX,"%s",directories[selection]);

        // Display Status
        displayStatus();

        // Get path of parent directory
        allocSize = snprintf(NULL,0,"%s",dir);
        prev_dir = malloc(allocSize+1);
        if(prev_dir == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }
        snprintf(prev_dir,allocSize+1,"%s",dir);
        getParentPath(prev_dir);

        // Get path of child directory
        allocSize = snprintf(NULL,0,"%s/%s", dir, directories[selection]);
        next_dir = malloc(allocSize+1);
        if(next_dir == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }
        if(strcasecmp(dir,"/") == 0)
            snprintf(next_dir,allocSize+1,"%s%s", dir, directories[selection]);
        else
            snprintf(next_dir,allocSize+1,"%s/%s", dir, directories[selection]);

        // Stores number of files in the child directory
        len_preview = getNumberofFiles(next_dir);
        // Set temp_len to 1 if `len_preview` is less than 0
        if(len_preview <= 0)
            temp_len = 1;
        else
            temp_len = len_preview;
        // Stores files in the child directory
        char** next_directories;
        next_directories = (char **)malloc(temp_len * sizeof(char *));
        if(next_directories == NULL)
        {
            endwin();
            printf("%s\n", "Couldn't allocate memory!");
            exit(1);
        }
        status = getFiles(next_dir, next_directories);

        // Selection is a directory
        free(sort_dir);
        if(len_preview > 0)
        {
            allocSize = snprintf(NULL,0,"%s",next_dir);
            sort_dir = malloc(allocSize+1);
            if(sort_dir == NULL)
            {
                endwin();
                printf("%s\n", "Couldn't allocate memory!");
                exit(1);
            }
            snprintf(sort_dir,allocSize+1,"%s",next_dir);
            qsort(next_directories, len_preview, sizeof (char*), compare);
            free(sort_dir);
        }

        // Selection is not a directory
        if(len != 0)
        {
            if(len_preview != -1)
            {
                for( i=0; i<len_preview; i++ )
                {
                    if(i == maxy - 1)
                        break;
                    wmove(preview_win,i+1,2);
                    // Allocate `temp_dir` and store path of file in `next_dir`
                    free(temp_dir);
                    allocSize = snprintf(NULL,0,"%s/%s", next_dir, next_directories[i]);
                    temp_dir = malloc(allocSize+1);
                    if(temp_dir == NULL)
                    {
                        endwin();
                        printf("%s\n", "Couldn't allocate memory!");
                        exit(1);
                    }
                    snprintf(temp_dir, allocSize+1, "%s/%s", next_dir, next_directories[i]);
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
            }

            // Get Preview of File
            else
            {
                getPreview(next_dir,maxy,maxx/2+2);
            }
        }

        // Remove signal handler used for preview
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;
        act.sa_handler = SIG_DFL;

        if(sigaction(SIGUSR1, &act, NULL) == -1) 
            printf("unable to handle siguser1\n");
        if(sigaction(SIGCHLD, &act, NULL) == -1) 
            printf("unable to handle sigchild\n");

        // Disable STANDOUT and colors attributes if enabled
        wattroff(current_win, A_STANDOUT);
        wattroff(current_win, COLOR_PAIR(1));
        wattroff(preview_win, COLOR_PAIR(1));

        // Draw borders and refresh windows
        refreshWindows();

        // For storing pid of children
        pid_t pid;

        // For fzf
        char *buf;
        char *path;
        FILE *fp;
        int fd;
        int pfd[2];

        // For two key keybindings
        char secondKey;
        // For taking user confirmation
        char confirm;

        // Keybindings
        switch( ch = wgetch(current_win) )
        {
            // Go up
            case KEY_UP:
            case KEY_NAVUP:
                scrollUp();
                break;

            // Go down
            case KEY_DOWN:
            case KEY_NAVDOWN:
                scrollDown();
                break;

            // Go to child directory or open file
            case KEY_RIGHT:
            case KEY_NAVNEXT:
            case '\n':
                goForward();
                break;

            // Go up a directory
            case KEY_LEFT:
            case KEY_NAVBACK:
                goBack();
                break;

            // Go to next page
            case KEY_NPAGE:
                nextPage();
                break;

            // Go to previous page
            case KEY_PPAGE:
                prevPage();
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
                // Clear Image Preview (If Present)
                clearImg();

                // Reallocate `temp_dir` and store path to preview file
                free(temp_dir);
                allocSize = snprintf(NULL,0,"%s/preview",cache_path);
                temp_dir = malloc(allocSize+1);
                if(temp_dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(temp_dir,allocSize+1,"%s/preview",cache_path);

                // Remove the preview file
                remove(temp_dir);

                // Create a child process to run command and store output in preview file
                endwin();
                pid = fork();
                if( pid == 0 )
                {
                    fd = open(temp_dir, O_CREAT | O_WRONLY, 0755);
                    dup2(fd, 1);
                    chdir(dir);
                    execlp("fzf","fzf",(char *)0);
                    exit(1);
                }
                else
                {
                    int status;
                    waitpid(pid,&status,0);
                }

                // Execute fzf command and store output in buf
                buf = malloc(PATH_MAX);
                if(buf == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                memset(buf, '\0', PATH_MAX);
                fp = fopen(temp_dir, "r");
                while(fgets(buf,PATH_MAX,fp) != NULL){}
                fclose(fp);

                // Allocate Memory to `path`
                allocSize = snprintf(NULL, 0, "%s/%s",dir,buf);
                path = malloc(allocSize+1);
                if(path == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(path, allocSize+1, "%s/%s",dir,buf);

                // Copy `path` into `temp_dir` to work with strtok.
                //Then fetch the last token from `temp_dir` and store it in `last`.
                free(temp_dir);
                allocSize = snprintf(NULL,0,"%s", path);
                temp_dir = malloc(allocSize+1);
                if(temp_dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(temp_dir,allocSize+1,"%s",path);
                getLastToken("/");
                getParentPath(path);
                free(dir);
                allocSize = snprintf(NULL,0,"%s", path);
                dir = malloc(allocSize+1);
                if(dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(dir,allocSize+1,"%s", path);

                // Set appropriate flags
                selection = 0;
                start = 0;
                clearFlag = 1;
                searchFlag = 1;

                // Free Memory
                free(buf);
                free(path);
                break;

            // Search in the same directory
            case KEY_SEARCHDIR:
                // Clear Image Preview (If Present)
                clearImg();

                // Reallocate `temp_dir` and store path to preview file
                free(temp_dir);
                allocSize = snprintf(NULL,0,"%s/preview",cache_path);
                temp_dir = malloc(allocSize+1);
                if(temp_dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(temp_dir,allocSize+1,"%s/preview",cache_path);

                // Remove the preview file
                remove(temp_dir);

                // Create a child process to run command and store output in preview file
                endwin();
                pipe(pfd);
                pid = fork();
                if( pid == 0 )
                {
                    dup2(pfd[1], 1);
                    close(pfd[1]);
                    chdir(dir);
                    if( hiddenFlag == 1 )
                        execlp("ls","ls","-a",(char *)0);
                    else
                        execlp("ls","ls",(char *)0);
                    exit(1);
                }
                else
                {
                    int status;
                    waitpid(pid,&status,0);
                    close(pfd[1]);
                    pid = fork();
                    if(pid == 0)
                    {
                        fd = open(temp_dir, O_CREAT | O_WRONLY, 0755);
                        dup2(pfd[0],0);
                        dup2(fd, 1);
                        close(pfd[0]);
                        execlp("fzf","fzf",(char *)0);
                        exit(1);
                    }
                    else
                    {
                        waitpid(pid, &status, 0);
                        close(pfd[0]);
                    }
                }

                // Allocate Memory to `buf` to store output of command
                buf = malloc(PATH_MAX);
                if(buf == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                memset(buf, '\0', PATH_MAX);
                fp = fopen(temp_dir, "r");
                while(fgets(buf,PATH_MAX,fp) != NULL){}
                fclose(fp);

                // Allocate Memory to `path` to store path selected file
                allocSize = snprintf(NULL,0,"%s/%s",dir,buf);
                path = malloc(allocSize+1);
                if(path == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(path,allocSize+1,"%s/%s",dir,buf);

                // Reallocate `temp_dir`
                free(temp_dir);
                allocSize = snprintf(NULL,0,"%s",path);
                temp_dir = malloc(allocSize+1);
                if(temp_dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(temp_dir,allocSize+1,"%s",path);
                getLastToken("/");
                getParentPath(path);

                // Reallocate `dir` to store directory where the selected file is
                free(dir);
                allocSize = snprintf(NULL,0,"%s",path);
                dir = malloc(allocSize+1);
                if(dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(dir,allocSize+1,"%s",path);

                // Free Memory
                free(buf);
                free(path);

                // Set appropriate flags
                selection = 0;
                start = 0;
                clearFlag = 1;
                searchFlag = 1;
                break;

            // Opens shell in present directory
            case KEY_SHELL:
                // Clear Image Preview (If Present)
                clearImg();

                // End ncurses mode
                endwin();

                // Create a child process to run shell
                pid = fork();
                if( pid == 0 )
                {
                    chdir(dir);
                    execlp(shell, shell, (char *)0);
                    exit(1);
                }
                else
                {
                    int status;
                    waitpid(pid, &status, 0);
                }

                // Restart ncurses mode and set appropriate flags
                start = 0;
                selection = 0;
                refresh();
                break;

            // Bulk Rename
            case KEY_RENAME:
                // Clear Image if Exists
                clearImg();
                if( access( clipboard_path, F_OK ) == -1 )
                {
                    // Reallocate `temp_dir` to store full path of selected file
                    free(temp_dir);
                    allocSize = snprintf(NULL,0,"%s/%s",dir,directories[selection]);
                    temp_dir = malloc(allocSize+1);
                    if(temp_dir == NULL)
                    {
                        endwin();
                        printf("%s\n", "Couldn't allocate memory!");
                        exit(1);
                    }
                    snprintf(temp_dir,allocSize+1,"%s/%s",dir,directories[selection]);
                    char *temp = strdup(temp_dir);
                    if(temp == NULL)
                    {
                        endwin();
                        printf("%s\n", "Couldn't allocate memory!");
                        exit(1);
                    }
                    writeClipboard(replace(temp,"\n","//"));
                    free(temp);
                }
                renameFiles();
                selectedFiles = 0;
                break;

            // Write to clipboard
            case KEY_SEL:
                // Reallocate `temp_dir` to store full path of selection
                free(temp_dir);
                allocSize = snprintf(NULL,0,"%s/%s",dir,directories[selection]);
                temp_dir = malloc(allocSize+2);
                if(temp_dir == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                snprintf(temp_dir, allocSize+2, "%s/%s", dir, directories[selection]);
                char *temp = strdup(temp_dir);
                if(temp == NULL)
                {
                    endwin();
                    printf("%s\n", "Couldn't allocate memory!");
                    exit(1);
                }
                if (checkClipboard(temp_dir) == 1)
                {
                    removeClipboard(replace(temp,"\n","//"));
                    selectedFiles--;
                }
                else
                {
                    writeClipboard(replace(temp,"\n","//"));
                    selectedFiles++;
                }
                free(temp);
                scrollDown();
                break;

            // Empty Clipboard
            case KEY_EMPTYSEL:
                emptyClipboard();
                selectedFiles = 0;
                break;

            // Copy clipboard contents to present directory
            case KEY_YANK:
                copyFiles(dir);
                break;

            // Moves clipboard contents to present directory
            case KEY_MV:
                moveFiles(dir);
                selectedFiles = 0;
                break;

            // Moves clipboard contents to trash
            case KEY_REMOVEMENU:
                clearImg();
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
                        {
                            removeFiles();
                            selectedFiles = 0;
                        }
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
                // Clear Image Preview (If Present)
                clearImg();
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
                // Clear Image Preview (If Present)
                clearImg();
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
                // Exit curses mode to show clipboard
                endwin();
                if( access( clipboard_path, F_OK ) != -1 )
                {
                    // Create a child process to show clipboard
                    pid = fork();
                    if( pid == 0 )
                    {
                        execlp("less", "less", clipboard_path, (char *)0);
                        exit(1);
                    }
                    else
                    {
                        int status;
                        waitpid(pid, &status, 0);
                    }
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
                // Exit curses mode to edit clipboard
                endwin();

                if( access( clipboard_path, F_OK ) != -1 )
                {
                    // Create a child process to show clipboard
                    pid = fork();
                    if( pid == 0 )
                    {
                        execlp(editor, editor, clipboard_path, (char *)0);
                        exit(1);
                    }
                    else
                    {
                        int status;
                        waitpid(pid, &status, 0);
                        setSelectionCount();
                    }
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
                clearFlagImg = 1;
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

            // View Preview
            case KEY_PREVIEW:
                viewPreview();
                break;

            // Run External Script
            case KEY_SCRIPT:
                len_scripts = getNumberofFiles(scripts_path);
                if(len_scripts <= 0)
                {
                    displayAlert("No scripts found!");
                    sleep(1);
                }
                else
                {
                    // Clear Image Preview (If Present)
                    clearImg();
                    int status;
                    char* scripts[len_scripts];
                    status = getFiles(scripts_path, scripts);
                    if(status == -1)
                    {
                        displayAlert("Cannot fetch scripts!");
                        sleep(1);
                        break;
                    }
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
                        // Reallocate `temp_dir` to store path of script
                        free(temp_dir);
                        allocSize = snprintf(NULL, 0, "%s/%s", scripts_path, scripts[option]);
                        temp_dir = malloc(allocSize+1);
                        if(temp_dir == NULL)
                        {
                            endwin();
                            printf("%s\n", "Couldn't allocate memory!");
                            exit(1);
                        }
                        snprintf(temp_dir, allocSize+1, "%s/%s", scripts_path, scripts[option]);

                        // Allocate Memory to `buf` to store path of selection
                        allocSize = snprintf(NULL, 0, "%s/%s", dir, directories[selection]);
                        buf = malloc(allocSize+1);
                        if(buf == NULL)
                        {
                            endwin();
                            printf("%s\n", "Couldn't allocate memory!");
                            exit(1);
                        }
                        snprintf(buf, allocSize+1, "%s/%s", dir, directories[selection]);

                        // Exit ncurses mode to execute the script
                        endwin();

                        // Create a child process to execute the script
                        pid = fork();
                        if( pid == 0 )
                        {
                            chdir(dir);
                            execl(temp_dir, scripts[option], buf, (char *)0);
                            exit(1);
                        }
                        else
                        {
                            int status;
                            waitpid(pid, &status, 0);
                        }

                        // Free Memory and restart curses mode
                        free(buf);
                        refresh();
                    }
                    for(i=0; i<len_scripts; i++)
                    {
                        free(scripts[i]);
                    }
                }
                break;

            // Toggle Borders
            case KEY_TOGGLEBORDERS:
                if(bordersFlag == 1)
                    bordersFlag = 0;
                else
                    bordersFlag = 1;
                break;

            // Clear Preview Window
            case KEY_RELOAD:
                clearFlag = 1;
                break;
        }

        // Free Memory
        free(next_dir);
        free(prev_dir);
        for( i=0; i<len_preview; i++ )
        {
            free(next_directories[i]);
        }
        for( i=0; i<len; i++ )
        {
            free(directories[i]);
        }
        if(len <= 0)
            free(directories[0]);
        free(next_directories);

    } while( ch != 'q');

    // Free Memory
    free(cache_path);
    free(temp_clipboard_path);
    free(clipboard_path);
    free(bookmarks_path);
    free(scripts_path);
    free(trash_path);
    free(editor);
    free(shell);
    free(dir);
    free(temp_dir);
    if(last != NULL)
        free(last);

    // Clear Image Preview (If Present)
    clearImg();

    // End curses mode
    endwin();
    return 0;
}
