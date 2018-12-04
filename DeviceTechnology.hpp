#pragma once
#include <vector>
#include <string>
#include <map>
#include "BasicDevices.hpp"
using namespace std;
namespace SynthFramework {
  namespace Polymer {
    class Operation;
    class LogicDesign;
    /*
    Starting with a very simple model which only generates basic LUTs and flip flops
    Over time this should be extended to a model which supports advanced device features
    such as multi-output LUTs, multipliers/DSP slices, muxes, etc
    */
    class DeviceTechnology {
    public:
      //Capability related

      //Return the number of inputs a native LUT has
      virtual int GetLUTInputCount() = 0;

      //Generation related

      //Generates the VHDL needed to include device primitive libraries
      virtual string GenerateInclude() = 0;
      //Generate the VHDL for a device
      virtual string GenerateDevice(LogicDevice* dev) = 0;
      //Generate any necessary signals for a device
      virtual string GenerateDeviceSignals(LogicDevice* dev) = 0;
      //Timing related

      //Return the propagation delay for a LUT
      virtual double GetLUTTpd(LUT* lut) = 0;
      //Return the routing delay for a intra-LUT connection
      virtual double GetRoutingDelay_LUT_LUT() = 0;
      //Return the routing delay for a LUT-FF connection
      virtual double GetRoutingDelay_LUT_FF() = 0;
      //Return the setup time for a FF
      virtual double GetFFSetupTime() = 0;
      //Return the propagation delay for a FF
      virtual double GetFFTpd() = 0;

      //This enables devices to specify some native optimisation for a given operation,
      //for example by using dedicated adders/carry resources or multipliers
      //It returns true if device-specific synthesis is performed
      virtual bool DeviceSpecificSynthesis(Operation *oper, LogicDesign* topLevel) = 0;

      //Analyse timing for a vendor specific device
      virtual void AnalyseTiming(VendorSpecificDevice *dev, LogicDesign *topLevel) = 0;
      //Print resource utilisation summary to the console
      virtual string PrintResourceUsage(LogicDesign *topLevel) = 0;

      //Process a device-specific constraint
      //Given a array representing the entire constraint line split by spaces, indices 1 and up being important
      virtual void SetDeviceConstraint(const vector<string>& line) = 0;

    };
  }
}
