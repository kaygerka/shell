#ifndef shell_hh
#define shell_hh

#include "command.hh"
extern bool yyerror_occurred;
//#include "y.tab.hh"

struct Shell {

  static void prompt();

  static Command _currentCommand;

  static int _lastExitStatus;
  static pid_t _lastBackgroundPID;
  static std::string _lastArgument;
  static int _shellPID;
  static std::string _shellPath;

};

#endif
