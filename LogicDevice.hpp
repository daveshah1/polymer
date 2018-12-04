#pragma once
#include <vector>
#include <string>
#include <map>
#include "Signal.hpp"
#include "LogicPort.hpp"
using namespace std;

namespace SynthFramework {
  namespace Polymer {
    /*
      All logic devices are represented with this class
      The basic idea is that these start off as fairly high level devices - adders, comparators, etc
      and eventually only very low level devices are left
    */
    class LogicDevice;
    class LogicDesign;

    class DeviceInputPort : public LogicPort {
    public:
      LogicDevice *device;
      int pin;

      bool IsDriver();
      bool IsConstantDriver();
      LogicState GetConstantState();
    };

    class DeviceOutputPort : public LogicPort {
    public:
      LogicDevice *device;
      int pin;

      bool IsDriver();
      bool IsConstantDriver();
      LogicState GetConstantState();
    };

    class LogicDevice {
    public:
      string name;
      bool pipelineDone = false;
      vector<DeviceInputPort*> inputPorts;
      vector<DeviceOutputPort*> outputPorts;
      //Disconnect all I/Os from the device, removing it from the system
      virtual void RemoveDevice();
      //Optimise the device, taking into account constant values at any inputs or outputs
      //Returning true means that optimisation of some kind occurred (so that an iterative optimisation algorithm knows when to stops)
      virtual bool OptimiseDevice(LogicDesign *topLevel);
      //Returns true if two devices are equivalent
      virtual bool IsEquivalentTo(LogicDevice* other);

    };

    //Indicates vendor-specific hardware blocks
    class VendorSpecificDevice : public LogicDevice {

    };
  }
}
