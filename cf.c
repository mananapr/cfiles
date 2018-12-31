// HEADERS
#include <stdio.h>
#include <dirent.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>


/*
    For qsort
*/
int compare (const void * a, const void * b ) {
  return strcmp(*(char **)a, *(char **)b);
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
    char ch;
    int start = 0;
    
    // Main Loop
    do 
    {
        // Get number of files in the home directory
        len = getNumberofFiles(dir);
        
        // Array of all the files in the current directory
        char* directories[len];
        getFiles(dir, directories);
        // Sort files by name
        qsort (directories, len, sizeof (char*), compare);
        
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
            if(i==selection)
                wattron(current_win, A_STANDOUT);
            else
                wattroff(current_win, A_STANDOUT);
            wmove(current_win,t+1,2);
            wprintw(current_win, "%.*s\n", maxx/2+2, directories[i]);
            t++;
        }

        // Name of selected file
        char* selected_file = directories[selection];
        // Stores files in the selected directory
        char next_dir[250] = "";
        // Stores path of parent directory
        char prev_dir[250] = "";
        
        // Get path of parent directory
        char *p;
        strcat(prev_dir, dir);
        p = strrchr(dir,'/');
        prev_dir[p-dir] = '\0';
        // Parent directory is root
        if(prev_dir[0] != '/')
        {
            prev_dir[0] = '/';
            prev_dir[1] = '\0';
        }
        
        // Get path of child directory
        strcat(next_dir, dir);
        strcat(next_dir, "/");
        strcat(next_dir, directories[selection]);
        // Stores number of files in the child directory
        int len_preview = getNumberofFiles(next_dir);
        char* next_directories[len_preview];
        getFiles(next_dir, next_directories);
        
        // Selection is a directory
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
            // Todo
        }
       
        // Draw borders and refresh windows
        wattroff(current_win, A_STANDOUT);
        box(current_win,0,0);
        box(preview_win,0,0);
        wrefresh(current_win);
        wrefresh(preview_win);
        

        // Keybindings
        switch( ch = wgetch(current_win) ) {
            case 'k':
                selection--;
                selection = ( selection < 0 ) ? 0 : selection;
                // Scrolling
                if(len > maxy)
                  if(selection > maxy/2)
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
            case 'j':
                selection++;
                selection = ( selection > len-1 ) ? len-1 : selection;
                // Scrolling
                if(len > maxy)
                  if(selection - 1 > maxy/2)
                  {
                    if(start + maxy -2 != len)
                    {
                    start++;
                    wclear(current_win);
                 }   
                }

                break;
            case 'l':
                strcpy(dir, next_dir);
                selection = 0;
                start = 0;
                break;
            case 'h':
                strcpy(dir, prev_dir);
                selection = 0;
                start = 0;
                break;
            case 'g':
                selection = 0;
                start = 0;
                break;
            case 'G':
                selection = len - 1;
                if(len > maxy - 2)
                    start = len - maxy - 1;
                // Needs to be fixed
                else
                    start = len - 1 - maxy - 4 + 3;
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
