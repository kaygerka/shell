#include <cstdio>
#include <unistd.h> // added = itsatty
#include <signal.h> // ctrl -c
#include <sys/wait.h> //ctrl - c
#include <termios.h> // ctrl-c
#include <limits.h> // for PATH_MAX
#include <unistd.h>  // for getpid()
#include <cstring>





#include "shell.hh"
bool promptPrinted = false;


bool yyerror_occurred = false; // yyerr flag ADDED
char *shell_path;
int Shell::_lastExitStatus = 0;
pid_t Shell::_lastBackgroundPID = 0;
std::string Shell::_lastArgument = "";
int Shell::_shellPID = getpid();
std::string Shell::_shellPath;
void source_shellrc();
extern FILE* yyin;
extern void yyrestart(FILE*);


int yyparse(void);

// ctrl-c funct signatures
void handleSigint(int sig);
void handleSigchld(int sig);

void my_yyerror(const char *s) {
  yyerror_occurred = true;
  fprintf(stderr, "%s\n", s); // Print error message with newline
}

/*
void Shell::prompt() {
  if (isatty(fileno(stdin))) {
    printf("myshell>");
    fflush(stdout);
  }
  yyerror_occurred = false;  //  reset after displaying prompt
}
*/

void Shell::prompt() {
  if (isatty(fileno(stdin))) {
    // Print error FIRST if needed
    if (_lastExitStatus != 0) {
      const char* on_error = getenv("ON_ERROR");
      if (on_error) printf("%s\n", on_error);
    }

    // Then print prompt
    if (isatty(fileno(stdin))) {
      const char* prompt = getenv("PROMPT");
     if (prompt) {
      printf("%s", prompt);
    } else {
      if (promptPrinted) {
        printf("myshell>");
      }
      promptPrinted = true;
    }
   fflush(stdout);
    }
  }
  yyerror_occurred = false;
}

/*
 * flushes and clears the input buffer and discard and typed/unread input
 */
// HANDLES SIGINT: used when ctrl-c is pressed
void handleSigint(int sig __attribute__((unused))) {
  fprintf(stderr, "\n");
  tcflush(fileno(stdin), TCIFLUSH);  // Requires <termios.h>
  yyerror_occurred = false;  // Reset error flag immediately

  if (isatty(fileno(stdin))) {
    fprintf(stderr, "myshell>");
    fflush(stderr);  // Ensure it's printed immediately
  }
}


/*
 * Terminates all child processes -- prevents zombies?
 */
// HANDLES SIGCHILD: when ctrl-c is pressed
void handleSigchld(int sig) {
  int status;
  pid_t pid;
  // removes all terminated child processes
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    // prints if the background is ctrl-c
    if (WIFSIGNALED(status)) {
      //printf("[%d] exited due to signal %d.\n", pid, WTERMSIG(status));
      Shell::prompt();
    } else {
      //printf("[%d] exited normally.\n", pid);
      Shell::prompt();
    }
    fflush(stdout);
  }
}

// MAINNNNNNNNN
int main(int argc, char *argv[]) {// ===================================
  // calls 2 sigaction structures: sigint and sigchld
  //source_shellrc();
  struct sigaction saInt, saChld;
  shell_path = argv[0];
  Shell::_shellPID = getpid();

  char resolved_path[PATH_MAX];
  // get relative path from name
  if (realpath(argv[0], resolved_path) != NULL) {
    Shell::_shellPath = resolved_path;
    setenv("SHELL", resolved_path, 1);
  }
  const char* shellrc = ".shellrc";
  if (access(shellrc, R_OK) == 0) {
    FILE* rcFile = fopen(shellrc, "r");
    if (rcFile) {
        yyin = rcFile;
        yyrestart(yyin);

        yyparse();

        fclose(rcFile);

        //  THE IMPORTANT FIX:
        yyin = stdin;
        yyrestart(yyin);
    }
  }


  //------ Configure SIGINT (Ctrl-C)
  saInt.sa_handler = handleSigint; // ---- sets saInt handler as handleSigInt which sets SIGINT
  sigemptyset(&saInt.sa_mask); // -- no other signals are blocked
  saInt.sa_flags = SA_RESTART; // -- resumes after read/write
  if (sigaction(SIGINT, &saInt, NULL)) {
    perror("sigaction(SIGINT)");
    return EXIT_FAILURE;
  }

  //------ Configure SIGCHLD (child process termination)
  saChld.sa_handler = handleSigchld;
  sigemptyset(&saChld.sa_mask);
  saChld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &saChld, NULL)) {
    perror("sigaction(SIGCHLD)");
    return EXIT_FAILURE;
  }

  
  // Print new prompt
  if (isatty(fileno(stdin))) {
    Shell::prompt();
  }
   yyparse();
}

Command Shell::_currentCommand;
