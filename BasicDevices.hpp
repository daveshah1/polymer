#pragma once
#include <vector>
#include <string>
#include <map>
#include "LogicDevice.hpp"
using namespace std;

namespace SynthFramework {
  namespace Polymer {
     //Standard devices that can be used to intialise a LUT
     enum LUTDeviceType {
       Device_AND2,
       Device_OR2,
       Device_XOR2,
       Device_NAND2,
       Device_NOR2,
       Device_XNOR2,
       Device_AND3,
       Device_NOT,
       Device_FULLADD_SUM, //SUM generator for a full adder
       Device_FULLADD_CARRY, //CARRY generator for a full adder
       Device_MUX2_1,
     };

     /*A generic multi-input, one-output LUT, used before a technology-specific LUT is instantiated*/
     class LUT : public LogicDevice {
     public:
       //Initialise an empty LUT
       LUT();
       //Initialise a LUT from a standard device, and given input and output signals
       LUT(LUTDeviceType type, vector<Signal*> inputs, Signal* output);
       //The contents of the LUT, one entry for each input permutation
       vector<bool> lutContent;
       //Returns whether or not two LUTs are logically equivalent
       bool IsEquivalentTo(LogicDevice* other);
       //Move another LUT into the current one
       void MergeWith(LUT* other, int pin);

       bool OptimiseDevice(LogicDesign *topLevel);

       //Generate sum of products VHDL for the LUT given names for all the inputs
       string GenerateSOP(const vector<string>& pinNames);

       //This forces the maximum size of the LUT, used due to restrictions in Altera's carry logic
       int maxSizeOverride = -1;
    private:
       static map<LUTDeviceType, vector<bool> > initialContents;
       static int lutCount;
     };

     /*
     A generic D flip flop with optional enable and reset
     Inputs: D, CLK, EN, RST
     Outputs: Q
     */
     class FlipFlop : public LogicDevice {
     public:
       FlipFlop();
       //Set enable and reset to nullptr if not needed
       FlipFlop(Signal *D, Signal *CLK, Signal *Q, Signal *enable = nullptr, Signal *reset = nullptr);

       bool hasEnable = false;
       bool hasReset = false;

       bool IsEquivalentTo(LogicDevice* other);

       bool isShift = false; //If true then flipflop is not counted for latency purposes
    private:
       static int ffCount;
     };

     /*
     A multi-bit multiplier
     */
     class Multiplier : public LogicDevice {
     public:
      int a_size;
      int b_size;
      int q_size;
      bool is_signed = false;
     };

     /*
     A device that outputs a constant 0/1 value
     */
     class ConstantDevice : public LogicDevice {
     public:
       ConstantDevice();
       ConstantDevice(Signal *sig, bool _value);
       bool value = false;
     };
  }
}
