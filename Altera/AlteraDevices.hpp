#pragma once
#include <vector>
#include <string>
#include <map>
#include "../LogicDevice.hpp"
using namespace std;

namespace SynthFramework {
  namespace Polymer {
      //Altera fast carry logic
      class Altera_CarrySum : public VendorSpecificDevice {
      public:
          Altera_CarrySum();
          Altera_CarrySum(Signal *sin, Signal *cin, Signal *sout, Signal *cout);
          virtual bool OptimiseDevice(LogicDesign *topLevel);
      private:
          static int cscount;
      };
      //Altera single-port ROM primitive
      class Altera_ROM : public VendorSpecificDevice {
      public:
          Altera_ROM();
          Altera_ROM(int _width, int _length, string _filename, Signal *clock, vector<Signal*> address, vector<Signal*> data);
          int width, length;
          string filename;
      private:
          static int romcount;
      };
      //Altera 18x18 embedded multiplier (9x9 mode and registers NYI at the moment)
      class Altera_Multiplier18 : public VendorSpecificDevice {
      public:
        Altera_Multiplier18();
        Altera_Multiplier18(vector<Signal*> a, vector<Signal*> b, vector<Signal*> out, bool _sign_a, bool _sign_b);
        virtual bool OptimiseDevice(LogicDesign *topLevel);
        bool sign_a, sign_b;
      private:
        static int mulcount;
        int actualsize = 36; //for optimisation purposes
    };
  };
};
