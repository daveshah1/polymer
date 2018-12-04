#pragma once
#include <vector>
#include <string>
#include <map>
#include <set>
#include "BasicDevices.hpp"
#include "LogicCore.hpp"
#include "LogicPort.hpp"
#include "Signal.hpp"
#include "DeviceTechnology.hpp"
#include "Operations.hpp"
using namespace std;

namespace SynthFramework {
  namespace Polymer {

    class LogicDesign;
    //Describes a top level input/output, which may be a bus
    struct DesignIO {
      string IOName;
      bool is_output = false;
      int width = 0;
      bool is_signed = false;
      Bus* assocBus = nullptr;
      vector<LogicPort*> assocPorts;
    };

    class LatencyConstraint;

    //An input to the design (an output from the internal synthesis point of view)
    class DesignInputPort : public LogicPort {
    public:
      LogicDesign *design;
      DesignIO *linkedIO;
      int busIndex;

      bool IsDriver();
      bool IsConstantDriver();
      LogicState GetConstantState();

      LatencyConstraint *latcon = nullptr;
    };

    class DesignOutputPort : public LogicPort {
    public:
      LogicDesign *design;
      DesignIO *linkedIO;
      int busIndex;

      bool IsDriver();
      bool IsConstantDriver();
      LogicState GetConstantState();

      vector<LatencyConstraint*> latcons;
    };

    //A latency constraint forces the latency of some inputs to be related to the latency some outputs
    //This is used when there are external ROMs, etc
    class LatencyConstraint {
    public:
      vector<DesignOutputPort*> outputs;
      vector<DesignInputPort*> inputs;
      int ext_latency = 1;
    };

    class LogicDesign {
    public:
      LogicDesign();

      string designName = "";
      vector<DesignIO*> io;
      vector<DesignInputPort*> inputPorts; //individual ports for input and output signals
      vector<DesignOutputPort*> outputPorts;
      vector<LogicDevice*> devices; //post-synthesis logic devices
      vector<Operation*> operations; //pre-synthesis operations
      vector<Bus*> buses; //all buses in the design
      vector<Signal*> signals; //all signals in the design
      vector<LatencyConstraint*> latcons;
      DeviceTechnology* technology;

      //Search path for submodules (searched in order)
      vector<string> searchPath;

      //Build the LogicDesign from input data (some kind of intermediate language optionally output by ElasticC)
      //This populates io, inputPorts, outputPorts, operations, buses and signals but does not generate any LUTs
      //or other low-level devices
      //prefix is set if including a submodule instead of loading the top level design
      //portmap is a map between submodule IO names and top level buses if in a submodule
      void LoadDesign(string data, string prefix = "", const map<string, Bus*> &portmap = map<string, Bus*>());
      //Perform optimisations on the design and synthesise it to LUTs, etc of the correct size and type for the FPGA
      void SynthesiseAndOptimiseDesign();
      //Run timing analysis on the design
      void AnalyseTiming();
      //Insert pipeline registers as necessary
      void PipelineDesign();
      //Run a post-pipeline timing analysis
      void AnalysePostPipelineTiming();
      //Convert the design to a VHDL netlist
      string GenerateVHDL();

      //Design constraints
      double targetFrequency = 50e6; //design target frequency in Hz
      double timingSlack = 0; //slack to subtract from clock period in seconds
      double timingBudget = 0.9; //fraction of clock period after which signals should be valid
      bool allowPipeline = true;
      int maxLatency = -1; //max pipeline latency, -1 = unlimited
      int minLatency = 0; //min pipeline latency

      //Special purpose signals
      Signal* gnd, *vcc;

      //Miscellaneous helper functions
      void AddBus(Bus *b);
      Signal *CreateSignal(string name);
      Bus *CreateConstantBus(int value); //Create a bus with a fixed value (for now value must be positive)
      //Global pipeline controls
      bool globalHasEnable = false, globalHasReset = false;
      Signal *clockSignal = nullptr, *globalEnable = nullptr, *globalReset = nullptr;
    private:
      Bus* ParseSignalDefinition(const vector<string> &splitLine, string prefix); //Parse any signal definition - input, output or internal signal
      Bus* FindBusByName(string name);
      //Setting stopAtRegister to true improves performance by only considering timing upto the first register
      //The set is used to avoid analysing devices more than once thus improving performance
      void AnalyseTimingRecursive(LogicDevice* target, bool stopAtRegister, set<LogicDevice*> &analysed);
      void PipelineDesignRecursive(LogicDevice* target);
      Signal* PipelineSignal(Signal* source);
      void PipelinePin(DeviceInputPort *pin); //shortcut to add a pipeline before a pin

    };
  }
}
