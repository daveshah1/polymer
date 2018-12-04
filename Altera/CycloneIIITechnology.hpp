#pragma once
#include "../DeviceTechnology.hpp"
#include "../Signal.hpp"
#include "../BasicDevices.hpp"
#include "AlteraDevices.hpp"

namespace SynthFramework {
    namespace Polymer {
        class CycloneIIITechnology : public DeviceTechnology {
        public:
            int GetLUTInputCount();

            string GenerateInclude();
            string GenerateDevice(LogicDevice* dev);
            string GenerateDeviceSignals(LogicDevice* dev);

            double GetLUTTpd(LUT* lut);
            double GetRoutingDelay_LUT_LUT();
            double GetRoutingDelay_LUT_FF();

            double GetFFSetupTime();
            double GetFFTpd();

            bool DeviceSpecificSynthesis(Operation *oper, LogicDesign* topLevel);

            void AnalyseTiming(VendorSpecificDevice *dev, LogicDesign *topLevel);

            string PrintResourceUsage(LogicDesign *topLevel);

            void SetDeviceConstraint(const vector<string>& line);

            bool lite_mode = false; //Reduce size of VHDL at the expense of less explicitly specifying LUT boundaries
        private:
            string SynthesiseLUT(LUT *lut);
            string SynthesiseLUTSignals(LUT *lut);
            string SynthesiseFF(FlipFlop *ff);
            string SynthesiseCarrySum(Altera_CarrySum *cs);
            string SynthesiseROM(Altera_ROM *rom);
            string SynthesiseROMSignals(Altera_ROM *rom);
            string SynthesiseMultiplier(Altera_Multiplier18 *mul);
            string SynthesiseMultiplierSignals(Altera_Multiplier18 *mul);

            //Generate adder chain using CARRYSUMs
            void GenerateAdderChain(Operation *oper, bool isSub, LogicDesign *topLevel, Signal *cin = nullptr);

            //Helper functiond for multiplier synthesis
            Bus* GenerateMulShift(LogicDesign* topLevel, Bus *a, Bus *b, int shiftamt, string name);
            Bus* GenerateIntAdder(LogicDesign* topLevel, Bus *a, Bus *b, string name);
        };
    }
}
