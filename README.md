# shell
simpleCommand.cc: This is a C++ source code file for a custom Unix-like command-line shell implementation. The code provides the core logic for parsing, expanding, and executing shell commands, featuring many advanced shell behaviors.

Main Features
- Built-in Commands:
- Tilde and Variable Expansion:
- Wildcard Expansion:
- Process Substitution:
- Command Execution Pipeline:
- Prompt and Error Handling:
- Memory and Resource Management:

shell.y: This is a Bison/Yacc grammar file for a C++ shell, defining how command lines are parsed and mapped into internal data structures, enabling advanced shell features like pipelines, I/O redirection, variable expansion, and background execution.
