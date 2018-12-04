#pragma once
#include <vector>
#include <string>
#include <map>
using namespace std;

namespace SynthFramework {
  namespace Polymer {
     class LogicPort;
     class Bus;
     class LogicDevice;
     /*Represents a single signal*/
     class Signal {
     public:
       string name;
       vector<Bus*> parentBuses; //Buses the signal belongs to
       vector<LogicPort*> connectedPorts;
       int GetFanout(); //Get number of inputs the signal connects to
       void ConnectTo(Signal* other); //Move all connected ports to some other signal
       bool GetConstantValue(bool &val); //Returns true and sets val if signal has constant 0/1 value (currently don't care is forced to zero)
       LogicDevice* GetDriver(); //Return the device driving the signal; or nullptr if the signal is not driven or driven by a top-level input
       //Computed clock-to-valid delay for timing analysis purposes
       double delay = 0;
       //Pipeline latency for balancing purposes
       int latency = 0;

       bool avoidPipeline = false; //if true pipeline registers will not break this signal unless necessary (for local logic connections etc.)

       bool isSlow = false; //Signals that change rarely can be declared 'slow' and therefore aren't bothered with during pipelining as a speedhack
     };
     /*Represents a group of signals*/
     class Bus {
     public:
        Bus();
        Bus(string _name, bool _is_signed, int _width);
        string name;
        vector<Signal*> signals;
        bool is_signed = false;
        int width = 1;
        bool GetConstantValue(long long &val); //Returns true and sets val if all signals are constant (don't cares are forced to zero)
     };
  }
}
