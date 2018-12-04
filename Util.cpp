#include "Util.hpp"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
namespace SynthFramework {
    /*
    If you are using a terminal that does not support VT100 control codes, you will need to
    define BORING_MODE
    */

    MessageLevel verbosity = MSG_NOTE;


    void Console_SetForeColour(ConsoleColour clr) {
      #ifndef BORING_MODE
      cerr << "\033[" << (clr + 30) << "m";
      #endif
    }

    void Console_ResetColour() {
      #ifndef BORING_MODE
      cerr << "\033[0m";
      #endif
    }

    void Console_SetBright() {
      #ifndef BORING_MODE
      cerr << "\033[1m";
      #endif
    }

    void PrintMessage(MessageLevel level, string message, int line) {
        if(level >= verbosity) {
            ConsoleColour clr;
            switch(level) {
            case MSG_DEBUG:
                clr = COLOUR_BLUE;
                Console_SetForeColour(clr);
                cerr << "[DEBUG]";
                break;
            case MSG_NOTE:
                clr = COLOUR_GREEN;
                Console_SetForeColour(clr);
                cerr << "[NOTE ]";
                break;
            case MSG_WARNING:
                clr = COLOUR_YELLOW;
                Console_SetForeColour(clr);
                cerr << "[WARN ]";
                break;
            case MSG_ERROR:
                clr = COLOUR_RED;
                Console_SetForeColour(clr);
                cerr << "[ERROR]";
                break;
            }
            cerr << " ";
            if(line > 0) {
                cerr << "[" << setw(4) << line << "]";
            } else {
                cerr << "      ";
            }
            cerr << " ";

            bool is_bold = false;
            for(int i = 0; i < message.size(); i++) {
              if(message[i] == '\n') {
                  //indent lines nicely
                  cerr << endl << "               ";
              } else {
                  if((i < (message.size() - 2))) {
                    if((message[i] == '=') && (message[i+1] == '=') && (message[i+2] == '=')) {
                      is_bold  = !is_bold;
                      if(is_bold) {
                        Console_SetBright();
                      } else {
                        Console_ResetColour();
                        Console_SetForeColour(clr);
                      }
                      i += 2;
                    } else {
                      cerr << message[i];
                    }

                  } else {
                    cerr << message[i];
                  }
              }

            }
            cerr << endl;


        }
        Console_ResetColour();
        if(level == MSG_ERROR) {
          //  cerr << "compilation terminated due to error" << endl;
            exit(EXIT_FAILURE);
        }
    }

}
