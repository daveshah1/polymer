#pragma once
#include "../DeviceTechnology.hpp"
#include "../Signal.hpp"
#include "../BasicDevices.hpp"
#include "XilinxDevices.hpp"

namespace SynthFramework {
    namespace Polymer {
        class Artix7Technology : public DeviceTechnology {
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
        private:
            string SynthesiseLUT(LUT *lut);
            string SynthesiseFF(FlipFlop *ff);
            string SynthesiseCarry4(Xilinx_Carry4 *ca4);
            string SynthesiseCarry4Signals(Xilinx_Carry4 *ca4);

            //Generate adder chain using Carry4s
            void GenerateAdderChain(Operation *oper, bool isSub, LogicDesign *topLevel);
        };
    }
}
