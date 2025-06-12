%{

#include <cstring>
#include "y.tab.hh"
#include <unistd.h>    // For pipe(), fork(), dup2(), close(), read()
#include <sys/wait.h>  // For wait()
//#include <cstring>     // Existing include
//#include "y.tab.hh"    // Existing include
#include <limits.h> // For PATH_MAX
#include "command.hh"
#include <string.h>


extern "C" char * read_line(); // edit line
static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

// ------ SUBSHELL ----------
void handleSubshell(char *command) {
  // create vars
  int pipe_in[2], pipe_out[2];
  pid_t pid;
  // CREATE PIPES
  if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
    perror("pipe");
    exit(1);
  }
  // FORK
  if ((pid = fork()) == -1) {
    perror("fork");
    exit(1);
  }
    
  // CHILD
  if (pid == 0) {
    // REDIRECT stdin/stdout
    dup2(pipe_in[0], STDIN_FILENO);
    dup2(pipe_out[1], STDOUT_FILENO);
    // Close unused ends of pipes
    close(pipe_in[1]);
    close(pipe_out[0]);
    // Execute the command in the subshell
    execlp("sh", "sh", "-c", command, NULL);
    perror("execlp");
    exit(1);

    // PARENT
  } else {
    char buffer[1024];
    int bytes_read;
    // Close unused ends of pipes
    close(pipe_in[0]);
    close(pipe_out[1]);
    // Read output and inject into input buffer
    int last_char_was_newline = 0;
    while ((bytes_read = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
      for (int i = bytes_read - 1; i >= 0; i--) {
        if (buffer[i] == '\n') {
          if (i != 0 || !last_char_was_newline) {
            unput(' ');
          }
          last_char_was_newline = 1;
        } else {
          unput(buffer[i]);
          last_char_was_newline = 0;
        }
      }
    } 
  }
  // Close pipes and wait for child
  close(pipe_in[1]);
  close(pipe_out[0]);
  wait(NULL);
}

int mygetc(FILE * f) { // -----------------------------------------------
  static char *p;
  char ch;
  if (!isatty(0)) {
  // stdin is not a tty. Call real getc
    return getc(f);
  }
  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }
  ch = *p;
  p++;
  return ch;
}

#undef getc
#define getc(f) mygetc(f)







%}

%option noyywrap

%%

\n { return NEWLINE; }
[ \t] { /* Discard spaces and tabs */ }
">" { return GREAT; }
"<" { return LESS; }
">>" { return GREATGREAT; }
">&" { return GREATAMPERSAND; }
">>&" { return GREATGREATAMPERSAND; }
"2>" { return TWOGREAT; }
"|" { return PIPE; }
"&" { return AMPERSAND; }
"exit" { return EXIT; }
\$\(([^)]+)\) { // SUBSHELL
  char *subshellCommand = strdup(yytext + 2); // Skip "$("
  subshellCommand[strlen(subshellCommand) - 1] = '\0'; // Remove trailing ")"
  handleSubshell(subshellCommand);
  free(subshellCommand);
  return WORD;  // Ensure a token is returned
}
\\$\{?\?\}? {
  yylval.cpp_string = new std::string("?"); // capture it as a placeholder
  return SPECIAL_VAR;
}
\\$\{?[a-zA-Z_][a-zA-Z0-9_]*\}? { // ENVIRONMENT VARIABLES
  std::string str(yytext);
  std::string varName;

  if (str.length() == 3 && str == "${$}") {
    varName = "$";
  } else {
    varName = str.substr(2, str.length() - 3);
  }
  std::string value;
  if (varName == "$") {
    value = std::to_string(Shell::_shellPID);
  } else if (varName == "?") {
    value = std::to_string(Shell::_lastExitStatus);
  } else if (varName == "!") {
    value = std::to_string(Shell::_lastBackgroundPID);
  } else if (varName == "_") {
    value = Shell::_lastArgument;
  } else if (varName == "SHELL") {
    char buf[1024];
    if (getcwd(buf, sizeof(buf))) {
      value = std::string(buf) + "/shell";
    } else {
      value = "shell";
    }
  } else {
    char *val = getenv(varName.c_str());
    value = val ? std::string(val) : "";
  }
  yylval.cpp_string = new std::string(value);
  return WORD;
}
source[ \t]+([^ \t\n]+) { // SOURCE
  // yytext = full matched text "source blah.sh"
  // + 7 skips source to get filename -> "blah.sh"
  std::string filename(yytext + 7); 
  // open file to read
  FILE *fp = fopen(filename.c_str(), "r");
  if (!fp) {
    perror("Error opening file for source");
    return WORD;
  }
  // create new flex buffer that holds contents of file to be lexed 
  // YY_BUF_SIZE defined constant around 16KB?
  YY_BUFFER_STATE new_buffer = yy_create_buffer(fp, YY_BUF_SIZE);
  if (!new_buffer) {
    perror("Error creating buffer");
    fclose(fp);
    return WORD;
  }    
  // tells FLEX to pause, switch to new file, while saving curr buffer
  yypush_buffer_state(new_buffer);
  // calls parser and parses current flex buffer (new_buffer)
  int parse_result = yyparse();
  if (parse_result != 0) {
    printf("Error in parsing the source file.\n");
  }
  //pop the buffer state to restore original/prev input
  yypop_buffer_state();
  // Close the source file
  fclose(fp);
  return NEWLINE;
}
<<EOF>> {
  yypop_buffer_state();
  if (!YY_CURRENT_BUFFER) yyterminate();
}
[a-zA-Z_][a-zA-Z0-9_]*=(\"(\\.|[^\"])*\"|\'(\\.|[^\'])*\'|[^ \t\n|><&]*) {
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
\'([^\']|\\\')*\' {  // Single quotes
  std::string *s = new std::string(yytext + 1, yyleng - 2);  // Remove the quotes
  // Process escape sequences within the string
  size_t pos = 0;
  while ((pos = s->find("\\'", pos)) != std::string::npos) {
    s->replace(pos, 2, "'");
    pos += 1;
  }    
  // Handle escape sequences like \n
  pos = 0;
  while ((pos = s->find("\\n", pos)) != std::string::npos) {
    s->replace(pos, 2, "\n");  // Replace \n with actual newline
    pos += 1;
  }
  // Handle any other escape sequences (like \\ for backslashes, etc.)
  pos = 0;
  while ((pos = s->find("\\\\", pos)) != std::string::npos) {
    s->replace(pos, 2, "\\");  // Replace \\ with actual backslash
    pos += 1;
  }
  yylval.cpp_string = s;
  return WORD;
}
\"(\\.|[^\"\\])*\" {  // Double quotes
  std::string processed;
  const char *p = yytext + 1; // Skip opening quote
  const char *end = yytext + yyleng - 1;
  while (p < end) {
    if (*p == '\\' && p+1 < end) {
      if (*(p+1) == 'n') {  // Check for the literal \n
        processed += '\n';  // Replace \n with actual newline
        p += 2;  // Skip past the \n
        continue;  // Skip the regular processing and continue to next character
      } else {
        processed += *(++p); // Handle other escaped characters
      }
    } else {
      processed += *p;
    }
    p++;
  }
  yylval.cpp_string = new std::string(processed);
  return WORD;
}
\$\'([^\\']|\\.)*\' {  // Handle single-quoted strings with escape sequences
  std::string processed;
  const char *p = yytext + 2; // Skip "$'"
  const char *end = yytext + yyleng - 1;
  while (p < end) {
    if (*p == '\\' && p + 1 < end) {
      if (*(p + 1) == 'n') {  // Handle '\n' as newline
        processed += '\n';
        p += 2;
        continue;
      } else {
        processed += *(++p);  // Handle other escaped characters
      }
    } else {
      processed += *p;
    }
    p++;
  }
  yylval.cpp_string = new std::string(processed);
  return WORD;
}
\\([^]|[ \t\n|><&]) {
  yylval.cpp_string = new std::string(1, yytext[1]);
  return WORD;
}
 
([^|><& \t\n\\'"]|\\.)+ {
  std::string processed;
  for (const char *p = yytext; *p; ++p) {
    if (*p == '\\' && *(p+1)) processed += *(++p);
      else processed += *p;
  }
  yylval.cpp_string = new std::string(processed);
  return WORD;
}



%%
