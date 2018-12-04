#pragma once
#include <vector>
#include <string>
#include <map>
#include "Signal.hpp"
#include "LogicCore.hpp"
using namespace std;

namespace SynthFramework {
  namespace Polymer {
     /*A generic multi-input, one-output LUT, used before a technology-specific LUT is instantiated*/
     class LogicPort {
     public:
       Signal *connectedNet;
       virtual bool IsDriver() = 0; //Is driving net?
       virtual bool IsConstantDriver() = 0; //Is driving net at a constant value
       virtual LogicState GetConstantState() = 0; //If constant value, return value of driver
       virtual void Disconnect();
     };
  }
}
