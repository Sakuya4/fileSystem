#ifndef UI_H
#define UI_H


int options();
int getCommand();

/* base command */
int lsCommand(); // list
int cdCommand(); // change directory
int rmCommand(); // remove file 
int mkdirCommand(); // make directory
int rmdirCommand(); // remove directory
int putsCommand(); // upload file
int getsCommand(); // download file
int renameCommand(); // rename file 
int catCommand(); // display file content
int statusCommand(); // display file status
int helpCommand(); // display help information
int exitCommand(); // exit program
/* base command done */

/* adv command */
int chmodCommand(); // change file permission
int chownCommand(); // change file owner
int vimCommand();// VIM editor command
/* adv command done */

/* cybersecurity command */
int hashCommand(); // file hash command
/* cybersecurity command done */

#endif



