
%code requires
{
#include <string>
#include <cstdio>
#include <cstdlib>

#if __cplusplus > 199711L
#define register // Deprecated in C++11 so remove the keyword
#endif

#include "shell.hh"

void my_yyerror(const char *s);
}
%union
{
char *string_val;
// Example of using a c++ type in yacc
std::string *cpp_string;
}
%destructor { delete $$; } <cpp_string>  // Add this line

%token <cpp_string> SPECIAL_VAR
%token <cpp_string> WORD
%token GREAT LESS GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT PIPE AMPERSAND EXIT NEWLINE
%{
//#define yylex yylex
#include <cstdio>
#include "shell.hh"

void yyerror(const char * s);
int yylex();

%}

%%

// start -- can receive 1+cmd
goal:
  commands
  ;

// one command_line or multiple
commands:
  command_line
  | commands command_line
  ;

// checks for pipes and background
command_line:
  pipeline background_opt NEWLINE { 
    Shell::_currentCommand.execute();
  }
  | EXIT NEWLINE {
    //printf("Good bye!!\n");
    exit(0);
  }
  | NEWLINE // empty
  | error NEWLINE { yyerrok; } //error
  ;


// with pipes, sets simple_command
pipeline:
  simple_command
  | pipeline PIPE simple_command
  ;


// splits command_and_args and iomodifier
simple_command:
  command_and_args iomodifier_opt 
  ;

// splits command and arguments
command_and_args:
  command_word argument_list {
    Shell::_currentCommand.insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

// sepearates all arguments
argument_list:
  argument_list argument
  | /* can be empty */
  ;

// removes quotes from argument echo "world" -> world
argument:
  WORD {
    Command::_currentSimpleCommand->insertArgument($1);
  }
  | SPECIAL_VAR {
    Command::_currentSimpleCommand->insertArgument($1);
  }
  ;


// sets the command and inputs into SimpleCommand
command_word:
  WORD {
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

// gets the iomodifier 
iomodifier_opt:
  iomodifier_opt iomodifier
  | /* can be empty */
  ;

// distiguish the io modifier and its funciton
iomodifier:
  // > REDIRECT OUTPUT
  GREAT WORD {
    if (Shell::_currentCommand._outFile) {
      yyerror("Ambiguous output redirect.\n");
      YYERROR;
    }
    Shell::_currentCommand._outFile = $2;
  }

  // < REDIRECT IN FILE
  | LESS WORD {
    Shell::_currentCommand._inFile = $2;
  }

  // 2> REDIRECT ERROR FILE
  | TWOGREAT WORD {
    Shell::_currentCommand._errFile = $2;
  }

  // >& OUTPUT AND ERROR REDIRECTION
  | GREATAMPERSAND WORD {
    Shell::_currentCommand._outFile = new std::string(*$2);
    Shell::_currentCommand._errFile = new std::string(*$2);
  }

  // >> APPEND OUTPUT
  | GREATGREAT WORD {
    Shell::_currentCommand._outFile = $2;
    Shell::_currentCommand._append = true;
  }

  // >>& APPEND OUTPUT AND REDIRECT ERROR
  | GREATGREATAMPERSAND WORD {
    Shell::_currentCommand._outFile = new std::string(*$2);
    Shell::_currentCommand._errFile = new std::string(*$2);
    Shell::_currentCommand._append = true;
   }
   ;


// checks if command should run in background
background_opt:
  AMPERSAND {
    Shell::_currentCommand._background = true;
  }
  | /* can be empty */
  ;

%%

void
yyerror(const char * s)
{
fprintf(stderr,"%s", s);
}

#if 0
main()
{
yyparse();
}
#endif
