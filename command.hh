#ifndef command_hh
#define command_hh

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;
  bool _append;

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  static SimpleCommand *_currentSimpleCommand;

  private:
        std::string expandEnvVars(const std::string &input);

};

void builtin_printenv(const std::string &var = "");
void builtin_setenv(const std::string &var, const std::string &value);
void builtin_unsetenv(const std::string &var);
void builtin_source(const std::string &filename);
void builtin_cd(std::string path);
extern int get_last_return_code();
extern pid_t get_last_bg_pid();
extern std::string get_last_arg();
//std::vector<std::string> expandWildcard(const std::string &pattern);
#endif
