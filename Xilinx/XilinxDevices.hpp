#pragma once
#include <vector>
#include <string>
#include <map>
#include "../LogicDevice.hpp"
using namespace std;

namespace SynthFramework {
  namespace Polymer {
      //7-series fast carry logic: see UG768
      class Xilinx_Carry4 : public VendorSpecificDevice {
      public:
          Xilinx_Carry4();
          Xilinx_Carry4(Signal *ci, Signal *cinit, vector<Signal*> DI, vector<Signal*> S, vector<Signal*> O, vector<Signal*> CO);
          virtual bool OptimiseDevice(LogicDesign *topLevel);
      private:
          static int carry4count;
      };
      //7-series DSP48 configured as a multiplier only
      //Support for more advanced configurations should be considered at a later date (in particular folding multiply and add together)
      class Xilinx_DSP48Mul : public VendorSpecificDevice {
      public:
          Xilinx_DSP48Mul();
          //A : 25 bit input, B : 18 bit input, M : 43 bit output
          Xilinx_DSP48Mul(Signal *clock, vector<Signal*> A, vector<Signal*> B, vector<Signal*> M);
          //Configured pipeline latency
          int latency = 0; //0 = no reg, 1 = MREG only, 2 = A and MREG
      private:
          static int dsp48count;
      };
  };
};
