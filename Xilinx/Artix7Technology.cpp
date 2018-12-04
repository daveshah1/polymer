#include "Artix7Technology.hpp"
#include "../Operations.hpp"
#include "../LogicDesign.hpp"
#include "../BasicDevices.hpp"
#include "../Util.hpp"
#include "XilinxDevices.hpp"
#include <typeinfo>
#include <sstream>
using namespace std;

namespace SynthFramework {
    namespace Polymer {
        int Artix7Technology::GetLUTInputCount() {
            return 6;
        }


        string Artix7Technology::GenerateInclude() {
            stringstream vhdl;
            vhdl << "library UNISIM;" << endl;
            vhdl << "use UNISIM.vcomponents.all;" << endl << endl;
            return vhdl.str();
        }

        string Artix7Technology::GenerateDevice(LogicDevice* dev) {
            LUT* lut = dynamic_cast<LUT*>(dev);
            FlipFlop *ff= dynamic_cast<FlipFlop*>(dev);
            Xilinx_Carry4 *ca4 = dynamic_cast<Xilinx_Carry4*>(dev);
            if(lut != nullptr) {
                return SynthesiseLUT(lut);
            } else if(ff != nullptr) {
                return SynthesiseFF(ff);
            } else if(ca4 != nullptr) {
                return SynthesiseCarry4(ca4);
            } else {
                return "";
            }

        }

        string Artix7Technology::GenerateDeviceSignals(LogicDevice* dev) {
            Xilinx_Carry4 *ca4 = dynamic_cast<Xilinx_Carry4*>(dev);
            if(ca4 != nullptr) {
                return SynthesiseCarry4Signals(ca4);
            } else {
                return "";
            }
        }
        //These are based on datasheet values for the -2 speed grade device
        double Artix7Technology::GetLUTTpd(LUT* lut) {
            return 0.11e-9;
        }
        double Artix7Technology::GetRoutingDelay_LUT_LUT() {
            return 0.5e-9; //this value needs refining
        }
        double Artix7Technology::GetRoutingDelay_LUT_FF() {
            return 0.05e-9;
        }

        double Artix7Technology::GetFFSetupTime() {
            return 0.09e-9;
        }
        double Artix7Technology::GetFFTpd() {
            return 0.44e-9;
        }

        bool Artix7Technology::DeviceSpecificSynthesis(Operation *oper, LogicDesign* topLevel) {
            if(oper->type == OPER_B_ADD) {
                GenerateAdderChain(oper, false, topLevel);
                return true;
            } else if(oper->type == OPER_B_SUB) {
                GenerateAdderChain(oper, true, topLevel);
                return true;
            } else {
                return false; //TODO: multiply using DSP block
            }

        }

        string Artix7Technology::SynthesiseLUT(LUT *lut) {
            //More advanced synthesis would generate 2-output LUTs, and also pack LUTs into CLBS thus using the LO pins
            //and enabling more accurate timing analysis
            string entType = "";
            if(lut->inputPorts.size() <= 6) {
                entType = "LUT" + to_string(lut->inputPorts.size());
            } else {
                PrintMessage(MSG_ERROR, "LUT " + lut->name + " too large for technology!");
            }
            stringstream vhdl;
            vhdl << "\t" << lut->name << " : " << entType << " generic map(" << endl;
            vhdl << "\t\t\tINIT => \"";
            for(int i = lut->lutContent.size() - 1; i >= 0; i--) {
                if(lut->lutContent[i]) {
                    vhdl << "1";
                } else {
                    vhdl << "0";
                }
            }
            vhdl << "\") port map(" << endl;
            for(int i = 0; i < lut->inputPorts.size(); i++) {
                vhdl << "\t\t\tI" << i << " => " << lut->inputPorts[i]->connectedNet->name << ", " << endl;
            }
            vhdl << "\t\t\tO => " << lut->outputPorts[0]->connectedNet->name << ");" << endl << endl;
            return vhdl.str();
        }

        string Artix7Technology::SynthesiseFF(FlipFlop *ff) {
            stringstream vhdl;
            vhdl << "\t" << ff->name << " : FDCE generic map(" << endl;
            vhdl << "\t\t\tINIT => '0') port map(" << endl;
            vhdl << "\t\t\tQ => " << ff->outputPorts[0]->connectedNet->name << ", " << endl;
            vhdl << "\t\t\tC => " << ff->inputPorts[1]->connectedNet->name << ", " << endl;
            vhdl << "\t\t\tD => " << ff->inputPorts[0]->connectedNet->name << ", " << endl;
            int resetPortIndex = 2;
            if(ff->hasEnable) {
                vhdl << "\t\t\tCE => " << ff->inputPorts[2]->connectedNet->name << ", " << endl;
                resetPortIndex++;
            } else {
                vhdl << "\t\t\tCE => '1', " << endl;
            }
            if(ff->hasReset) {
                vhdl << "\t\t\tCLR => " << ff->inputPorts[resetPortIndex]->connectedNet->name;
                resetPortIndex++;
            } else {
                vhdl << "\t\t\tCLR => '0'";
            }
            vhdl << ");" <<  endl << endl;
            return vhdl.str();
        }

        string Artix7Technology::SynthesiseCarry4(Xilinx_Carry4 *ca4) {
            stringstream vhdl;
            vhdl << "\t" << ca4->name << "_DI <= " << ca4->inputPorts[5]->connectedNet->name << " & "
                    << ca4->inputPorts[4]->connectedNet->name << " & "
                    << ca4->inputPorts[3]->connectedNet->name << " & "
                    << ca4->inputPorts[2]->connectedNet->name << ";" << endl;
            vhdl << "\t" << ca4->name << "_S <= " << ca4->inputPorts[9]->connectedNet->name << " & "
                    << ca4->inputPorts[8]->connectedNet->name << " & "
                    << ca4->inputPorts[7]->connectedNet->name << " & "
                    << ca4->inputPorts[6]->connectedNet->name << ";" << endl;

            for(int i = 0; i < 4; i++) {
                vhdl << "\t" << ca4->outputPorts[i]->connectedNet->name << " <= " << ca4->name << "_O(" << i << ");" << endl;
                vhdl << "\t" << ca4->outputPorts[i+4]->connectedNet->name << " <= " << ca4->name << "_CO(" << i << ");" << endl;

            }
            vhdl << endl;
            vhdl << "\t" << ca4->name << " : CARRY4 port map(" << endl;
            vhdl << "\t\t\tDI => " << ca4->name << "_DI, " << endl;
            vhdl << "\t\t\tS => " << ca4->name << "_S, " << endl;
            vhdl << "\t\t\tO => " << ca4->name << "_O, " << endl;
            vhdl << "\t\t\tCO => " << ca4->name << "_CO, " << endl;


            vhdl << "\t\t\tCI => " << ca4->inputPorts[0]->connectedNet->name << ", " << endl;
            vhdl << "\t\t\tCYINIT => " << ca4->inputPorts[1]->connectedNet->name << endl;
            vhdl << "\t\t);" << endl << endl;
            return vhdl.str();
        }

        string Artix7Technology::SynthesiseCarry4Signals(Xilinx_Carry4 *ca4) {
            stringstream vhdl;
            vhdl << "\tsignal " << ca4->name << "_DI : std_logic_vector(3 downto 0);" << endl;
            vhdl << "\tsignal " << ca4->name << "_S : std_logic_vector(3 downto 0);" << endl;
            vhdl << "\tsignal " << ca4->name << "_O : std_logic_vector(3 downto 0);" << endl;
            vhdl << "\tsignal " << ca4->name << "_CO : std_logic_vector(3 downto 0);" << endl;
            return vhdl.str();
        }


        void Artix7Technology::GenerateAdderChain(Operation *oper, bool isSub, LogicDesign *topLevel) {
            int adderSize = (oper->output->width + 3) / 4; //round up to nearest multiple of 4
            Signal *carryChain = nullptr;
            for(int i = 0; i < adderSize; i++) {
                vector<Signal*> DI, S, O, CO;
                Signal *ci, *cinit;
                for(int j = 0 ; j < 4; j++) {
                    int bit = i * 4 + j;
                    vector<Signal*> inPins;
                    for(int k = 0; k < 2; k++) {
                        Signal *inPin = oper->GetInputSignal(k, bit, topLevel);
                        if((k == 1) && isSub) { //need to invert B input for a subtractor
                            Signal *invPin = topLevel->CreateSignal(oper->name + "_invB_" + to_string(bit));
                            topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{inPin}, invPin));
                            inPins.push_back(invPin);
                        } else {
                            inPins.push_back(inPin);
                        }
                    }
                    DI.push_back(topLevel->CreateSignal(oper->name + "_DI" + to_string(bit)));
                    S.push_back(topLevel->CreateSignal(oper->name + "_S" + to_string(bit)));
                    topLevel->devices.push_back(new LUT(Device_AND2, inPins, DI[j]));
                    topLevel->devices.push_back(new LUT(Device_XOR2, inPins, S[j]));
                    O.push_back(topLevel->CreateSignal(oper->name + "_O" + to_string(bit)));
                    CO.push_back(topLevel->CreateSignal(oper->name + "_CO" + to_string(bit)));
                    if(bit < oper->output->width) {
                        oper->output->signals[bit]->ConnectTo(O[j]);
                    }
                }
                if(carryChain == nullptr) {
                    ci = topLevel->gnd;
                    if(isSub) {
                        cinit = topLevel->vcc;
                    } else {
                        cinit = topLevel->gnd;
                    }
                } else {
                    cinit = topLevel->gnd;
                    ci = carryChain;
                }

                topLevel->devices.push_back(new Xilinx_Carry4(ci, cinit, DI, S, O, CO));

                carryChain = CO[3];
            }
        }

        void Artix7Technology::AnalyseTiming(VendorSpecificDevice *dev, LogicDesign *topLevel) {
            Xilinx_Carry4 *ca4 = dynamic_cast<Xilinx_Carry4*>(dev);
            Xilinx_DSP48Mul *dsp48 = dynamic_cast<Xilinx_DSP48Mul*>(dev);

            double worst_input_tpd = 0;
            for(auto input : dev->inputPorts) {
                LogicDevice* outputDriver = input->connectedNet->GetDriver();
                if(outputDriver != nullptr) {
                    if(dynamic_cast<LUT*>(outputDriver) != nullptr) {
                        worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay + GetRoutingDelay_LUT_LUT());
                    } else {
                        worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay);
                    }
                } else {
                    worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay);
                }
            }

            if(ca4 != nullptr) {
                for(auto output : ca4->outputPorts) {
                    //Rough timing figure, not in datasheet for some reason
                    output->connectedNet->delay = worst_input_tpd + 0.3e-9;
                }
            } else if(dsp48 != nullptr) {
                for(auto output : dsp48->outputPorts) {
                    if(dsp48->latency == 0) {
                        output->connectedNet->delay = worst_input_tpd + 4.3e-9;
                    } else if(dsp48->latency == 1) {
                        output->connectedNet->delay = 1.9e-9;
                    } else if(dsp48->latency == 2) {
                        output->connectedNet->delay = 1.9e-9;
                    }
                }
            }
       }

       string Artix7Technology::PrintResourceUsage(LogicDesign *topLevel) {
           //TODO
           return "";
       }
       void Artix7Technology::SetDeviceConstraint(const vector<string>& line) {
         //TODO
       }
    }
}
