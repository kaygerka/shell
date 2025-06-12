
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>   // For DIR, struct dirent, opendir, readdir, closedir

#define MAX_BUFFER_LINE 2048
#define MAX_HISTORY 1000
#define _POSIX_C_SOURCE 200809L


// SETS TERMINAL TO RAW MODE
extern void tty_raw_mode(void); 

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

//-- MY OWN STRDUP
char *my_strdup(const char *s) {
  char *copy = malloc(strlen(s) + 1);
  if (copy) strcpy(copy, s);
  return copy;
}

// HISTORY VARIABLES ----------------------------
char *my_history[MAX_HISTORY];
int my_history_length = 0;
int history_index = 0;
int current_history_index = -1;

//--- CLEARS CURRENT LINE
void clear_line() {
  char ch ;
  // backspace to start
  for (int i = 0; i < line_length; i++) {
    ch = 8;
    write(1, &ch, 1);
  }
  // overwrite each char with ' '
  for (int i = 0; i < line_length; i++) {
    ch = ' ';
    write(1, &ch, 1);
  }
  // move basck to start
  for (int i = 0; i < line_length; i++) {
    ch = 8;
    write(1, &ch, 1);
  }
}

//--- helper to print? 
void read_line_print_usage() {
  char * usage = "\n"
  " ctrl-? Print usage\n"
  " Backspace Deletes last character\n"
  " Up/Down arrow Navigate command history\n";

  write(1, usage, strlen(usage));
}

/*
* Input a line with some basic editing.
*/
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  current_history_index = -1; 
  int cursor_pos = 0;


  // Read one line until enter is typed ----------------
  while (1) {

  // Read one character in raw mode.
  char ch;
  read(0, &ch, 1);

  if (ch>=32 && ch != 127) { // PRINTABLE CHARACTER --------------

    // If max number of character reached return
    if (line_length < MAX_BUFFER_LINE-2) {
      // REPOSITION - to insert move right
      for (int i = line_length; i > cursor_pos; i--) {
        line_buffer[i] = line_buffer[i - 1];
      }
      line_buffer[cursor_pos] = ch;
      line_length++;
      cursor_pos++;

      // ACTUALLY INSERT from cursor_pos - 1
      write(1, &line_buffer[cursor_pos - 1], line_length - (cursor_pos - 1));

      // RETURN TO CORRECT POSITION WHERE IT WAS INSERTED
      for (int i = 0; i < line_length - cursor_pos; i++) {
        char bs = 8;
        write(1, &bs, 1);
      }
    }
  }
  else if (ch==10) { // ENTER --------------------------
    // PRINT NEWLINE
    write(1,&ch,1);
    break;
  }
  else if (ch == 31) { // CTRL-? ----------------------------
    // PRINTS THAT HELPER FUNCTION && clear line buffer
    read_line_print_usage();
    line_buffer[0]=0;
    break;
  }
  else if (ch == 8 || ch == 127) { // BACKSPACE ---------------
    if (cursor_pos > 0) {
      // REPOSITION - shift left
      for (int i = cursor_pos - 1; i < line_length - 1; i++) {
        line_buffer[i] = line_buffer[i + 1];
      }
      line_length--;
      cursor_pos--;

      // REPOSITION - shift to start
      char bs = 8;
      write(1, &bs, 1);

      // WRITE THE REST - DELETED LAST CHAR VISUALLY
      write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
      write(1, " ", 1);

      // REPOSITION - shift to start
      for (int i = 0; i < (line_length - cursor_pos + 1); i++) {
        write(1, &bs, 1);
      }
    }  
  }
  else if (ch==27) { // ESCAPE SEQUENCE FOR ARROW KEYS -----------------
    // Escape sequence. Read two chars more
    char ch1; // [
    char ch2; // actual key
    read(0, &ch1, 1);
    read(0, &ch2, 1);

    // [
    if (ch1==91 ) {

      // ---- UP ARROW - A - 65
      if (ch2==65) {
        // checks if there are old commmands 
        if (current_history_index < my_history_length - 1) {

          // move "up" -- array stores in reverse order
          current_history_index++;
          clear_line();
          write(1, "myshell>", 8);
          // line_buffer - latest command, line_length = length of latest command, set cursor
          strcpy(line_buffer, my_history[my_history_length - 1 - current_history_index]);
          line_length = strlen(line_buffer);
          cursor_pos = line_length;
          // print old command
          write(1, line_buffer, line_length);
        }
      } // ----- DOWN ARROW - B - 66
      else if (ch2 == 66) {
        // checks if there are new commands
        if (current_history_index > 0) {
          // move "down"
          current_history_index--;
          clear_line();
          write(1, "myshell>", 8);
          // update line buffer line move up
          strcpy(line_buffer, my_history[my_history_length - 1 - current_history_index]);
          line_length = strlen(line_buffer);
          cursor_pos = line_length;

          // print new command
          write(1, line_buffer, line_length);
        }// LATEST/MOST RECENT COMMAND
        else if (current_history_index == 0) {
          // print blankc line
          current_history_index = -1;
          clear_line();
          line_length = 0;
          cursor_pos = 0;
          line_buffer[0] = 0;
        }
      }// ---- LEFT ARROW - C- 67
      else if (ch2 == 68) { 
        // move left with backspace
        if (cursor_pos > 0) {
          char bs = 8;
          write(1, &bs, 1);
          cursor_pos--;
        }
      }// ---- RIGHT ARROW - D- 68
      else if (ch2 == 67) { 
        // move right and print char under cursor
        if (cursor_pos < line_length) {
          write(1, &line_buffer[cursor_pos], 1);
          cursor_pos++;
        }
      }
    }
  }// CTRL - D DELETE CHAR AT CURSOR
  else if (ch == 4) { //    
    if (cursor_pos < line_length) {
      // REPOSITION - shift char left
      for (int i = cursor_pos; i < line_length - 1; i++) {
        line_buffer[i] = line_buffer[i + 1];
      }
      line_length--;

      // WRITE CURR CHAR WITH WHATS AFTER IT -- and print space after last char
       write(1, &line_buffer[cursor_pos], line_length - cursor_pos);
       write(1, " ", 1);

      // REPOSITION - back to spot where delted
      for (int i = 0; i <= (line_length - cursor_pos); i++) {
        char bs = 8;
        write(1, &bs, 1);
      }
      
    }
  } // CTRL A - RETURN TO BEGGIN OF LINE
  else if (ch == 1) {
    while (cursor_pos > 0) {
      char bs = 8;
      write(1, &bs, 1);
      cursor_pos--;
    }
  } // CTRL E - GO TO END OF LINE
  else if (ch == 5) {
    while (cursor_pos < line_length) {
      write(1, &line_buffer[cursor_pos], 1);
      cursor_pos++;
    }
  } // TAB - FINISH WORD
  else if (ch == 9) {
    int start = cursor_pos - 1;
    while (start >= 0 && line_buffer[start] != ' ' && line_buffer[start] != '/') {
      start--;
    }
    start++; // Move to start of the word

    char partial[256];
    int plen = cursor_pos - start;
    strncpy(partial, &line_buffer[start], plen);
    partial[plen] = '\0';

    // Get directory to search (simplified to ".")
    DIR *dir = opendir(".");
    if (dir == NULL) {
      perror("opendir");
      continue;
    }

    struct dirent *entry;
    char *match = NULL;
    int match_count = 0;

    while ((entry = readdir(dir)) != NULL) {
      if (strncmp(entry->d_name, partial, plen) == 0) {
        if (match_count == 0) {
          match = my_strdup(entry->d_name); // First match
        } else {
          // Shorten match to common prefix
          int i = 0;
          while (match[i] && entry->d_name[i] && match[i] == entry->d_name[i]) {
            i++;
          }
          match[i] = '\0'; // Trim to common prefix
        }
        match_count++;
      }
    }
    closedir(dir);

    if (match_count > 0 && match) {
      // Clear current word
      for (int i = cursor_pos; i > start; i--) {
        char bs = 8;
        write(1, &bs, 1);
      }

      // Shift line buffer content after cursor
      int mlen = strlen(match);
      int delta = mlen - plen;
      for (int i = line_length; i >= cursor_pos; i--) {
        line_buffer[i + delta] = line_buffer[i];
      }

      // Insert completion
      for (int i = 0; i < mlen; i++) {
        line_buffer[start + i] = match[i];
      }

      line_length += delta;
      cursor_pos = start + mlen;

      // Redraw rest of the line
      write(1, &line_buffer[start], line_length - start);
      // Move cursor back if needed
      for (int i = 0; i < line_length - cursor_pos; i++) {
        char bs = 8;
        write(1, &bs, 1);
      }

      free(match);
    }
  }
 }// while(1)

  // add newline and \0
  line_buffer[line_length] = '\n';
  line_length++;
  line_buffer[line_length]=0;

  //add line to history
  if (line_length > 1) { 
    my_history[my_history_length] = my_strdup(line_buffer);
    my_history_length++;
  }
  return line_buffer;
}
