#pragma once
#include <string>
using namespace std;
namespace SynthFramework {
    enum MessageLevel {
    	MSG_DEBUG = 0,
    	MSG_NOTE,
    	MSG_WARNING,
    	MSG_ERROR
    };

    enum ConsoleColour {
      COLOUR_BLACK = 0,
      COLOUR_RED,
      COLOUR_GREEN,
      COLOUR_YELLOW,
      COLOUR_BLUE,
      COLOUR_MAGENTA,
      COLOUR_CYAN,
      COLOUR_WHITE
    };

    void Console_SetForeColour(ConsoleColour clr);
    void Console_ResetColour();
    void Console_SetBright();
    //Print a message to the console, if message level >= current verbosity level
    //Error level messages will terminate execution
    //Optionally takes a line number
    void PrintMessage(MessageLevel level, string message, int line = -1);

    extern MessageLevel verbosity;
}
