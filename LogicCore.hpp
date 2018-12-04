#pragma once
#include <vector>
#include <string>
#include <map>
using namespace std;


namespace SynthFramework {
  namespace Polymer {
    enum LogicState {
      LogicState_Unknown,
      LogicState_Low,
      LogicState_High,
      LogicState_DontCare
    };
  }
}
