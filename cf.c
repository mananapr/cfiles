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


//////////////////////
// GLOBAL VARIABLES //
//////////////////////
/*
    Base directory to be used for sorting
    `dir` for current_win
    `next_dir` for preview_win
*/
char sort_dir[250];

/*
    Stores the path for the cache directory
*/
char cache_path[250];

/*
    stores the path for the clipboard file
*/
char clipboard_path[250];

/*
    stores the path for trash
*/
char trash_path[250];


//////////////////////
// HELPER FUNCTIONS //
//////////////////////
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
    For qsort
*/
int compare (const void * a, const void * b )
{
    // They store the full paths of the arguments
    char temp_filepath1[250]="";
    char temp_filepath2[250]="";

    // Generate full paths
    strcat(temp_filepath1,sort_dir);
    strcat(temp_filepath1,"/");
    strcat(temp_filepath1,*(char **)a);
    strcat(temp_filepath2,sort_dir);
    strcat(temp_filepath2,"/");
    strcat(temp_filepath2,*(char **)b);

    if(is_regular_file(temp_filepath1) == 0 && is_regular_file(temp_filepath2) == 1)
        return -1;
    else if(is_regular_file(temp_filepath1) == 1 && is_regular_file(temp_filepath2) == 0)
        return 1;
    else
        return strcasecmp(*(char **)a, *(char **)b);
}


/*
    Opens a file using xdg-open
*/
void openFile(char *filepath)
{
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
    if( checkClipboard(filepath) == 1 )
        return;
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
    Gets previews of files
*/
void getPreview(char *filepath, int maxy, int maxx)
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
        sprintf(getdimensions_command,"echo -e '5;%s' | /usr/lib/w3m/w3mimgdisplay",filepath);
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
    Creates a new window with dimensions `height` and `width` starting at `starty` and `startx`
*/
WINDOW *create_newwin(int height, int width, int starty, int startx)
{	
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    return local_win;
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
        // Skip hidden files
      if(pDirent->d_name[0] != '.' )
        len++;
    }
    return len;
}


/*
    Stores all the file names in `char* directory` to `char *target[]`
*/
void getFiles(char* directory, char* target[])
{
    int i = 0;
    DIR *pDir;
    struct dirent *pDirent;

    pDir = opendir (directory);
    if (pDir == NULL) {
        return;
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        // Skip hidden files
        if(pDirent->d_name[0] != '.')
          target[i++] = strdup(pDirent->d_name);
    }

    closedir (pDir);
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
    while(fgets(buf, 250, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        pid = fork();
        if(pid == 0)
        {
            execl("/usr/bin/cp","cp","-r",buf,present_dir,(char *)0);
            exit(1);
        }
    }
    fclose(f);
}

/*
    Move files in clipboard to `present_dir`
*/
void moveFiles(char *present_dir)
{
    FILE *f = fopen(clipboard_path, "r");
    char buf[250];
    pid_t pid;
    int status;
    if(f == NULL)
    {
        return;
    }
    while(fgets(buf, 250, (FILE*) f))
    {
        buf[strlen(buf)-1] = '\0';
        pid = fork();
        if(pid == 0)
        {
            execl("/usr/bin/mv","mv",buf,present_dir,(char *)0);
            exit(1);
        }
    }
    fclose(f);
}


///////////////////
// MAIN FUNCTION //
///////////////////

int main(int argc, char* argv[])
{
    // To store number of files in directory
    int len=0;
    // Counter variable
    int i = 0;
    // Direcotry to be opened
    char* dir;

    // Get UID of user
    uid_t uid = getuid();
    // Get home directory of user from UID
    struct passwd *info = getpwuid(uid);
    
    // Make the cache directory
    struct stat st = {0};
    sprintf(cache_path,"%s/.cache/cfiles",info->pw_dir);
    if (stat(cache_path, &st) == -1) {
        mkdir(cache_path, 0751);
    }
    // Set the path for the clipboard file
    sprintf(clipboard_path,"%s/clipboard",cache_path);
    // Set the path for trash
    sprintf(trash_path,"%s/.local/share/Trash/files",info->pw_dir);

    // No Path is given in arguments
    // Set Path as $HOME
    if(argc == 1)
    {
        dir = info->pw_dir;
    }

    // Path is given in arguments
    // Set Path as the argument
    else if(argc == 2)
    {
        dir = argv[1];

        // Relative Path Given
        if(dir[0] != '/')
        {
            // Add path of $HOME before the Relative Path
            char temp[250] = "";
            strcat(temp,info->pw_dir);
            strcat(temp,"/");
            strcat(temp,dir);
            dir = temp;
        }   
    }

    // Incorrect Useage
    else
    {
        printf("Incorrect Useage\n");
        exit(0);
    }

    // Stores number of files in the present directory
    len = 0;

    // ncurses initialization
    initscr();
    noecho();
    curs_set(0);
   

    // Shows current directory
    WINDOW *current_win;
    // Shows child directory preview
    WINDOW *preview_win;
    // Shows status bar
    WINDOW *status_win;

    int startx, starty, midx, midy, maxx, maxy;
    
    // Index of currently selected item in `char* directories`
    int selection = 0;
    // For Storing user keypress
    char ch;
    // Index to start printing from `directories` array
    int start = 0;
    // Flag to clear preview_win
    int clearFlag = 0;
    // Flag is set to 1 when returning from `fzf`
    int searchFlag = 0;
    // Flag is set to 1 when user goes up a directory
    int backFlag = 0;
    // Stores the last token in the path. For eg, it will store 'a' is path is /b/a
    char *last;

    
    // Main Loop
    do 
    {
        // char array to work with strtok()
        char temp_dir[250];
        
        // Clear the preview_win
        if(clearFlag == 1)
        {
            wclear(preview_win);
            wrefresh(preview_win);
            clearFlag = 0;
        }

        // Get number of files in the home directory
        len = getNumberofFiles(dir);
        
        // Array of all the files in the current directory
        char* directories[len];
        getFiles(dir, directories);
        // Sort files by name
        strcpy(sort_dir,dir);
        qsort (directories, len, sizeof (char*), compare);
        
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

        // Get Size of terminal
        getmaxyx(stdscr, maxy, maxx);
        // Save last two rows for status_win
        maxy = maxy - 2;
    
        // Make the two windows side-by-side
        current_win = create_newwin(maxy, maxx/2+3, 0, 0);
        preview_win = create_newwin(maxy, maxx/2 -1, 0, maxx/2 + 1);
        status_win = create_newwin(2, maxx, maxy, 0);
        
        // Display Status
        wmove(status_win,1,1);
        wprintw(status_win, "%s@%s\t%s", getenv("USER"), getenv("HOSTNAME"), dir);
        wrefresh(status_win);

        // Print all the elements and highlight the selection
        int t = 0;
        for( i=start; i<len; i++ )
        {
            strcpy(temp_dir,dir);
            strcat(temp_dir,"/");
            strcat(temp_dir,directories[i]);
            if(i==selection)
                wattron(current_win, A_STANDOUT);
            else
                wattroff(current_win, A_STANDOUT);
            wmove(current_win,t+1,2);
            if( checkClipboard(temp_dir) == 0)
                wprintw(current_win, "%.*s\n", maxx/2+2, directories[i]);
            else
                wprintw(current_win, "* %.*s\n", maxx/2, directories[i]);
            t++;
        }

        // Name of selected file
        char* selected_file = directories[selection];
        // Stores files in the selected directory
        char next_dir[250] = "";
        // Stores path of parent directory
        char prev_dir[250] = "";
        
        // Get path of parent directory
        strcat(prev_dir, dir);
        getParentPath(prev_dir);
        
        // Get path of child directory
        strcat(next_dir, dir);
        strcat(next_dir, "/");
        strcat(next_dir, directories[selection]);
        // Stores number of files in the child directory
        int len_preview = getNumberofFiles(next_dir);
        char* next_directories[len_preview];
        getFiles(next_dir, next_directories);
        
        // Selection is a directory
        strcpy(sort_dir,next_dir);
        if(len_preview > 0)
            qsort(next_directories, len_preview, sizeof (char*), compare);
        
        // Selection is not a directory
        if(len_preview != -1)
            for( i=0; i<len_preview; i++ )
            {
                wmove(preview_win,i+1,2);
                wprintw(preview_win, "%.*s\n", maxx/2 - 2, next_directories[i]);
            }

        // Get Preview of File
        else
        {
            getPreview(next_dir,maxy,maxx/2+2);
            clearFlag = 1;
        }
       
        // Draw borders and refresh windows
        wattroff(current_win, A_STANDOUT);
        box(current_win,0,0);
        box(preview_win,0,0);
        wrefresh(current_win);
        wrefresh(preview_win);
        
        // For fzf file search
        char cmd[250];
        char buf[250];
        FILE *fp;

        // For two key keybindings
        char secondKey;
        
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
                char *pch;
                pch = strtok(temp_dir,"/");
                while (pch != NULL)
                {
                    last = pch;
                    pch = strtok(NULL,"/");
                }
                break;

            // Goto start
            case 'g':
                selection = 0;
                start = 0;
                break;

            // Goto end
            case 'G':
                selection = len - 1;
                if(len > maxy - 2)
                    start = len - maxy + 2;
                else
                    start = 0;
                break;

            // Search using fzf
            case 'F':
                sprintf(cmd,"cd %s && fzf",info->pw_dir);
                if((fp = popen(cmd,"r")) == NULL)
                {
                    exit(0);
                }
                while(fgets(buf,250,fp) != NULL){}
                char path[250];
                sprintf(path, "%s/%s",info->pw_dir,buf);
                // Copy `path` into `temp_dir` to work with strtok.
                //Then store the last token in `temp_dir` and store it in `last`.
                strcpy(temp_dir,path);
                pch = strtok(temp_dir,"/");
                while (pch != NULL)
                {
                    last = pch;
                    pch = strtok(NULL,"/");
                }
                getParentPath(path);
                strcpy(dir,path);
                selection = 0;
                start = 0;
                clearFlag = 1;
                searchFlag = 1;
                break;
            
            // Search in the same directory
            case 'f':
                sprintf(cmd,"cd %s && ls | fzf",dir);
                if((fp = popen(cmd,"r")) == NULL)
                {
                    exit(0);
                }
                while(fgets(buf,250,fp) != NULL){}
                sprintf(path, "%s/%s",info->pw_dir,buf);
                strcpy(temp_dir,path);
                pch = strtok(temp_dir,"/");
                while (pch != NULL)
                {
                    last = pch;
                    pch = strtok(NULL,"/");
                }
                getParentPath(path);
                strcpy(dir,path);
                selection = 0;
                start = 0;
                clearFlag = 1;
                searchFlag = 1;
                break;
            
            // Opens bash shell in present directory
            case 'S':
                strcpy(temp_dir,"cd ");
                strcat(temp_dir,dir);
                strcat(temp_dir," && bash");
                endwin();
                system(temp_dir);
                start = 0;
                selection = 0;
                refresh();
                break;

            // Write to clipboard
            case ' ':
                strcpy(temp_dir,dir);
                strcat(temp_dir,"/");
                strcat(temp_dir,directories[selection]);
                writeClipboard(temp_dir);
                break;

            // Empty Clipboard
            case 'u':
                emptyClipboard();
                break;

            // Copy clipboard contents to present directory
            case 'y':
                copyFiles(dir);
                break;

            // Moves clipboard contents to present directory
            case 'v':
                moveFiles(dir);
                break;

            // Moves clipboard contents to trash
            case 'd':
                secondKey = wgetch(current_win);
                if( secondKey == 'D'  )
                    moveFiles(trash_path);
                break;

            // Clear Preview Window
            case 'r':
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

    endwin();
    return 0;
}
