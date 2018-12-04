#include "CycloneIIITechnology.hpp"
#include "../Operations.hpp"
#include "../LogicDesign.hpp"
#include "AlteraDevices.hpp"
#include "../Util.hpp"
#include <sstream>
#include <set>
using namespace std;

namespace SynthFramework {
    namespace Polymer {
        int CycloneIIITechnology::GetLUTInputCount() {
            return 4;
        }

        string CycloneIIITechnology::GenerateInclude() {
            stringstream vhdl;
            vhdl << "LIBRARY altera;" << endl;
            vhdl << "USE altera.altera_primitives_components.all;" << endl << endl;
            vhdl << "LIBRARY altera_mf;" << endl;
            vhdl << "USE altera_mf.altera_mf_components.all;" << endl << endl;
            vhdl << "library lpm;" << endl;
            vhdl << "use lpm.lpm_components.all;" << endl << endl;
            return vhdl.str();
        }

        string CycloneIIITechnology::GenerateDevice(LogicDevice* dev) {
            LUT* lut = dynamic_cast<LUT*>(dev);
            FlipFlop *ff= dynamic_cast<FlipFlop*>(dev);
            Altera_CarrySum *cs = dynamic_cast<Altera_CarrySum*>(dev);
            Altera_ROM *rom = dynamic_cast<Altera_ROM*>(dev);
            Altera_Multiplier18 *mul18 = dynamic_cast<Altera_Multiplier18*>(dev);
            if(lut != nullptr) {
                return SynthesiseLUT(lut);
            } else if(ff != nullptr) {
                return SynthesiseFF(ff);
            } else if(cs != nullptr) {
                return SynthesiseCarrySum(cs);
            } else if(rom != nullptr) {
                return SynthesiseROM(rom);
            } else if(mul18 != nullptr) {
                return SynthesiseMultiplier(mul18);
            } else {
                return "";
            }
        }
        string CycloneIIITechnology::GenerateDeviceSignals(LogicDevice* dev) {
            LUT* lut = dynamic_cast<LUT*>(dev);
            Altera_ROM *rom = dynamic_cast<Altera_ROM*>(dev);
            Altera_Multiplier18 *mul18 = dynamic_cast<Altera_Multiplier18*>(dev);

            if(lut != nullptr) {
                return SynthesiseLUTSignals(lut);
            } else if(rom != nullptr) {
                return SynthesiseROMSignals(rom);
            } else if(mul18 != nullptr) {
                return SynthesiseMultiplierSignals(mul18);
            } else {
                return "";
            }
        }


        //Due to a lack of published data these are somewhat guesstimated
        double CycloneIIITechnology::GetLUTTpd(LUT* lut) {
            return 0.15e-9;
        }
        double CycloneIIITechnology::GetRoutingDelay_LUT_LUT() {
            return 0.25e-9;
        }
        double CycloneIIITechnology::GetRoutingDelay_LUT_FF() {
            return 0;
        }

        double CycloneIIITechnology::GetFFSetupTime() {
            return 0.12e-9;
        }
        double CycloneIIITechnology::GetFFTpd() {
            return 0;
        }
        Bus* CycloneIIITechnology::GenerateMulShift(LogicDesign* topLevel, Bus *a, Bus *b, int shiftamt, string name) {
            Bus * mul_res = new Bus(name + "_m", a->is_signed || b->is_signed, a->width + b->width);
            topLevel->AddBus(mul_res);
            Operation *mul = new Operation(OPER_B_MUL, vector<Bus*>{a, b}, mul_res);
            mul->name = name + "_mul";
            mul->Synthesise(topLevel);
            if(shiftamt == 0) {
                return mul_res;
            } else {
                Bus *shifted = new Bus(name + "_m_s",  a->is_signed || b->is_signed, a->width + b->width + shiftamt);
                topLevel->AddBus(shifted);
                Operation *lsl = new Operation(OPER_B_LS, vector<Bus*>{mul_res, topLevel->CreateConstantBus(shiftamt)}, shifted);
                lsl->name = name + "_lsl";
                lsl->Synthesise(topLevel);
                return shifted;
            }
        }
        Bus* CycloneIIITechnology::GenerateIntAdder(LogicDesign* topLevel, Bus *a, Bus *b, string name) {
            Bus *add_res = new Bus(name + "_a", a->is_signed || b->is_signed, max(a->width, b->width) + 1);
            topLevel->AddBus(add_res);
            Operation *add = new Operation(OPER_B_ADD, vector<Bus*>{a, b}, add_res);
            add->name = name + "_add";
            add->Synthesise(topLevel);
            return add_res;
        }

        bool CycloneIIITechnology::DeviceSpecificSynthesis(Operation *oper, LogicDesign* topLevel) {
            if(oper->type == OPER_B_ADD) {
                GenerateAdderChain(oper, false, topLevel);
                return true;
            } else if(oper->type == OPER_B_SUB) {
                GenerateAdderChain(oper, true, topLevel);
                return true;
            } else if(oper->type == OPER_T_ADD_CIN) {
                GenerateAdderChain(oper, false, topLevel, oper->GetInputSignal(2, 0, topLevel));
                return true;
            } else if(oper->type == OPER_B_MUL) {
                //TODO : this section is a crude proof of concept. refactor into multiple functions

                //multiplication where one operand is 3 bits or below doesn't need hardware
                if((oper->inputs[0]->width + oper->inputs[1]->width) < 10) {
                    return false;
                }
                //same for multiplication where one operand is a constant
                long long x1, x2;
                if(oper->inputs[0]->GetConstantValue(x1) || oper->inputs[1]->GetConstantValue(x2)) {
                    return false;
                }
                //otherwise generate using hard elements.
                //for now only support up to 36x36 (don't really care about more)
                if((oper->inputs[0]->width <= 18) && (oper->inputs[1]->width <= 18)) {
                    //trivial case: fits into a single multiplier block
                    vector<Signal*> inputs[2], output;
                    for(int j = 0; j < 18; j++) {
                        for(int i = 0; i < 2; i++) {
                            inputs[i].push_back(oper->GetInputSignal(i, j, topLevel));
                        }
                    }
                    for(int j = 0; j < 36; j++) {
                        if(j < oper->output->width) {
                            output.push_back(oper->output->signals[j]);
                        } else {
                            output.push_back(topLevel->CreateSignal(oper->name + "_O" + to_string(j)));
                        }
                    }
                    //deal with cases where the output signal is bigger than the multiplier
                    for(int j = 36; j < oper->output->width; j++) {
                        if(oper->inputs[0]->is_signed || oper->inputs[1]->is_signed) {
                            oper->output->signals[j]->ConnectTo(output.back());
                        } else {
                            oper->output->signals[j]->ConnectTo(topLevel->gnd);
                        }
                    }
                    topLevel->devices.push_back(new Altera_Multiplier18(inputs[0], inputs[1], output, oper->inputs[0]->is_signed, oper->inputs[1]->is_signed));
                } else {
                    //(a + b)(c + d) = ac + ad + bc + bd
                    //(2^18a' + b')(2^18c' + d') = 2^36a'c' + 2^18a'd' + 2^18b'c' + b'd'

                    //Create buses for the higher parts if needed
                    Bus *ah, *bh;
                    //And the lower parts
                    Bus *al, *bl;

                    if(oper->inputs[0]->width <= 18) {
                        al = oper->inputs[0];
                        ah = nullptr;
                    } else {
                        //TODO: check signedness handling is correct
                        al = new Bus(oper->name + "_al", false, 18);
                        topLevel->AddBus(al);
                        ah = new Bus(oper->name + "_ah", oper->inputs[0]->is_signed, oper->inputs[0]->width - 18);
                        topLevel->AddBus(ah);
                        for(int i = 0 ; i < oper->inputs[0]->width; i++) {
                            if(i < 18) {
                                al->signals[i]->ConnectTo(oper->inputs[0]->signals[i]);
                            } else {
                                ah->signals[i-18]->ConnectTo(oper->inputs[0]->signals[i]);
                            }
                        }
                    }

                    if(oper->inputs[1]->width <= 18) {
                        bl = oper->inputs[1];
                        bh = nullptr;
                    } else {
                        //TODO: check signedness handling is correct
                        bl = new Bus(oper->name + "_bl", false, 18);
                        topLevel->AddBus(bl);
                        bh = new Bus(oper->name + "_bh", oper->inputs[1]->is_signed, oper->inputs[1]->width - 18);
                        topLevel->AddBus(bh);
                        for(int i = 0 ; i < oper->inputs[1]->width; i++) {
                            if(i < 18) {
                                bl->signals[i]->ConnectTo(oper->inputs[1]->signals[i]);
                            } else {
                                bh->signals[i-18]->ConnectTo(oper->inputs[1]->signals[i]);
                            }
                        }
                    }

                    Bus *albl = GenerateMulShift(topLevel, al, bl, 0, oper->name + "_albl");

                    Bus *ahbl = nullptr;
                    if(ah != nullptr) {
                        ahbl = GenerateMulShift(topLevel, ah, bl, 18, oper->name + "_ahbl");
                    }
                    Bus *albh = nullptr;
                    if(bh != nullptr) {
                        albh = GenerateMulShift(topLevel, al, bh, 18, oper->name + "_albh");
                    }
                    Bus *ahbh = nullptr;
                    if((ah != nullptr) && (bh != nullptr)) {
                        ahbh = GenerateMulShift(topLevel, ah, bh, 36, oper->name + "_ahbh");
                    }

                    Bus *addint1, *addint2;
                    if(ahbh != nullptr) {
                        //All 4 terms are present, need a total of four adders
                        addint1 = GenerateIntAdder(topLevel, albl, albh, oper->name + "_adda");
                        addint2 = GenerateIntAdder(topLevel, ahbl, ahbh, oper->name + "_addb");

                    } else {
                        //Only 2 terms present, pick and add them
                        addint1 = albl;
                        if(ahbl != nullptr) {
                            addint2 = ahbl;
                        } else {
                            addint2 = albh;
                        }
                    }
                    Bus *result = GenerateIntAdder(topLevel, addint1, addint2, oper->name + "_r");
                    for(int i = 0; i < oper->output->width; i++) {
                        if(i < result->width) {
                            oper->output->signals[i]->ConnectTo(result->signals[i]);
                        } else if(result->is_signed) {
                            oper->output->signals[i]->ConnectTo(result->signals.back());
                        } else {
                            oper->output->signals[i]->ConnectTo(topLevel->gnd);
                        }
                    }
                }
                return true;
            } else {
                return false;
            }
        }

        string CycloneIIITechnology::SynthesiseLUT(LUT *lut) {
            stringstream vhdl;
            vector<string> pinNames;
            if(lite_mode)  {
              for(int i = 0; i < lut->inputPorts.size(); i++) {
                  pinNames.push_back(lut->inputPorts[i]->connectedNet->name);
              }
              vhdl << "\t" << lut->outputPorts[0]->connectedNet->name << " <= " << lut->GenerateSOP(pinNames) << ";" << endl;
            } else {
              for(int i = 0; i < lut->inputPorts.size(); i++) {
                  vhdl << "\t" << lut->name << "_ibuf" << i << " : lut_input port map(";
                  vhdl << lut->inputPorts[i]->connectedNet->name << ", " << lut->name << "_i" << i << ");" << endl;
                  pinNames.push_back(lut->name + "_i" + to_string(i));
              };
              vhdl << "\t" << lut->name << "_obuf : lut_output port map(";
              vhdl << lut->name << "_o, " << lut->outputPorts[0]->connectedNet->name << ");" << endl << endl;
              vhdl << "\t" << lut->name << "_o <= " << lut->GenerateSOP(pinNames) << ";" << endl;
            }

            return vhdl.str();
        }

        string CycloneIIITechnology::SynthesiseLUTSignals(LUT *lut) {
            if(lite_mode) {
              return "";
            } else {
              stringstream vhdl;
              for(int i = 0; i < lut->inputPorts.size(); i++) {
                  vhdl << "\tsignal " << lut->name << "_i" << i << " : std_logic;" << endl;
              }
              vhdl << "\tsignal " << lut->name << "_o : std_logic;" << endl << endl;
              return vhdl.str();
            }

        }

        string CycloneIIITechnology::SynthesiseFF(FlipFlop *ff) {
            stringstream vhdl;
            //Todo: enable and sync reset
            vhdl << "\t" << ff->name << " : DFF port map(" << endl;
            vhdl << "\t\t\td => " << ff->inputPorts[0]->connectedNet->name << ", " << endl;
            vhdl << "\t\t\tclk => " << ff->inputPorts[1]->connectedNet->name << ", " << endl;
            vhdl << "\t\t\tclrn => '1', " << endl;
            vhdl << "\t\t\tprn => '1', " << endl;
            vhdl << "\t\t\tq => " << ff->outputPorts[0]->connectedNet->name << endl;
            vhdl << "\t\t);" << endl << endl;
            return vhdl.str();
         }

         string CycloneIIITechnology::SynthesiseCarrySum(Altera_CarrySum *cs) {
             stringstream vhdl;
             vhdl << "\t" << cs->name << " : carry_sum port map(" << endl;
             vhdl << "\t\t\tsin => " << cs->inputPorts[0]->connectedNet->name << ", " << endl;
             vhdl << "\t\t\tcin => " << cs->inputPorts[1]->connectedNet->name << ", " << endl;
             vhdl << "\t\t\tsout => " << cs->outputPorts[0]->connectedNet->name << ", " << endl;
             vhdl << "\t\t\tcout => " << cs->outputPorts[1]->connectedNet->name << endl;
             vhdl << "\t\t);" << endl << endl;
             return vhdl.str();
         }

        void CycloneIIITechnology::AnalyseTiming(VendorSpecificDevice *dev, LogicDesign *topLevel) {
            //TODO more accurately
            double worst_input_tpd = 0;
            for(auto input : dev->inputPorts) {
                LogicDevice* outputDriver = input->connectedNet->GetDriver();
                if(outputDriver != nullptr) {
                    if(dynamic_cast<LUT*>(outputDriver) != nullptr) {
                        worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay + 0.15e-9);
                    } else {
                        worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay);
                    }
                } else {
                    worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay);
                }
            }

            Altera_Multiplier18 *mul18 = dynamic_cast<Altera_Multiplier18*>(dev);
            if(mul18 != nullptr) {
                for(auto op : dev->outputPorts) {
                    op->connectedNet->delay = worst_input_tpd + 4e-9;
                }
            } else {
                for(auto op : dev->outputPorts) {
                    op->connectedNet->delay = worst_input_tpd;
                }
            }

        }

        void CycloneIIITechnology::GenerateAdderChain(Operation *oper, bool isSub, LogicDesign *topLevel, Signal *cin) {
            int busSize = oper->output->width;

            //Carry chain signal: this is initialised with the CIN value
            Signal *carryChain;
            if(cin != nullptr) {
                carryChain = cin;
            } else {
                if(isSub) {
                    carryChain = topLevel->vcc;
                } else {
                    carryChain = topLevel->gnd;
                }
            }

            for(int j = 0; j < busSize; j++) {
                Signal *outPin = oper->GetOutputSignal(j, topLevel);
                if(outPin != nullptr) {
                    vector<Signal*> inPins;
                    for(int i = 0; i < 2; i++) {
                        Signal *inPin = oper->GetInputSignal(i, j, topLevel);
                        if((i == 1) && isSub) { //need to invert B input for a subtractor
                            Signal *invPin = topLevel->CreateSignal(oper->name + "_invB_" + to_string(j));
                            topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{inPin}, invPin));
                            inPins.push_back(invPin);
                        } else {
                            inPins.push_back(inPin);
                        }
                    }
                    inPins.push_back(carryChain); //carry in
                    Signal *carryOut = topLevel->CreateSignal(oper->name + "_CO_" + to_string(j));
                    Signal *carryOutInt = topLevel->CreateSignal(oper->name + "_COINT_" + to_string(j));
                    Signal *sumOutInt = topLevel->CreateSignal(oper->name + "_SOINT_" + to_string(j));
                    LUT *sum_lut = new LUT(Device_FULLADD_SUM, inPins, sumOutInt);
                    LUT *carry_lut = new LUT(Device_FULLADD_CARRY, inPins, carryOutInt);
                    sum_lut->maxSizeOverride = 3;
                    carry_lut->maxSizeOverride = 3;
                    topLevel->devices.push_back(sum_lut);
                    topLevel->devices.push_back(carry_lut);
                    topLevel->devices.push_back(new Altera_CarrySum(sumOutInt, carryOutInt, outPin, carryOut));

                    carryChain = carryOut;
                }
            }
        }

        string CycloneIIITechnology::SynthesiseROM(Altera_ROM *rom) {
            stringstream vhdl;
            vhdl << "\t" << rom->name << " : altsyncram generic map(" << endl;
            vhdl << "\t\t\t" << "OPERATION_MODE => \"ROM\"," << endl;
            vhdl << "\t\t\t" << "INIT_FILE => \"" << rom->filename << "\"," << endl;
            vhdl << "\t\t\t" << "WIDTH_A => " << rom->width << "," << endl;
            vhdl << "\t\t\t" << "WIDTHAD_A => " << (rom->inputPorts.size() - 1) << "," << endl;
            vhdl << "\t\t\t" << "NUMWORDS_A => " << rom->length << endl;
            vhdl << "\t\t) port map(" << endl;
            vhdl << "\t\t\tclock0 => " << rom->inputPorts[0]->connectedNet->name << ", " << endl;
            vhdl << "\t\t\taddress_a => ";
            for(int i = rom->inputPorts.size() - 1; i >= 1; i--) {
                vhdl << rom->inputPorts[i]->connectedNet->name << " ";
                if(i > 1) vhdl << "& ";
            }
            vhdl << ", " << endl;
            vhdl << "\t\t\tq_a => " << rom->name << "_q);" << endl;
            for(int i = 0 ; i < rom->outputPorts.size(); i++) {
                vhdl << "\t" << rom->outputPorts[i]->connectedNet->name << " <= " << rom->name + "_q(" << i << ");" << endl;
            }
            return vhdl.str();
        }

        string CycloneIIITechnology::SynthesiseROMSignals(Altera_ROM *rom) {
            stringstream vhdl;
            vhdl << "\tsignal " << rom->name << "_q : std_logic_vector(" << (rom->width - 1) << " downto 0);" << endl;
            return vhdl.str();
        }


        string CycloneIIITechnology::SynthesiseMultiplierSignals(Altera_Multiplier18 *mul) {
            stringstream vhdl;
            vhdl << "\tsignal " << mul->name << "_q : std_logic_vector(35 downto 0);" << endl;
            return vhdl.str();
        }

        string CycloneIIITechnology::SynthesiseMultiplier(Altera_Multiplier18 *mul) {
            //Using ALTMULT_ADD as its better documented, even though a lower-level primitive would be more 'correct'
            stringstream vhdl;
            vhdl << "\t" << mul->name << " : ALTMULT_ADD generic map(" << endl;
            vhdl << "\t\t\t" << "addnsub_multiplier_pipeline_register1 => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "addnsub_multiplier_register1 => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "dedicated_multiplier_circuitry => \"YES\"," << endl;
            vhdl << "\t\t\t" << "input_register_a0 => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "input_register_b0 => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "input_source_a0 => \"DATAA\"," << endl;
            vhdl << "\t\t\t" << "input_source_b0 => \"DATAB\"," << endl;
            vhdl << "\t\t\t" << "intended_device_family => \"Cyclone III\"," << endl;
            vhdl << "\t\t\t" << "lpm_type => \"altmult_add\"," << endl;
            vhdl << "\t\t\t" << "multiplier1_direction => \"ADD\"," << endl;
            vhdl << "\t\t\t" << "multiplier_register0 => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "number_of_multipliers => 1," << endl;
            vhdl << "\t\t\t" << "output_register => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "port_addnsub1 => \"PORT_UNUSED\"," << endl;
            vhdl << "\t\t\t" << "port_signa => \"PORT_UNUSED\"," << endl;
            vhdl << "\t\t\t" << "port_signb => \"PORT_UNUSED\"," << endl;
            vhdl << "\t\t\t" << "representation_a => ";
            if(mul->sign_a) {
                vhdl << "\"SIGNED\"";
            } else {
                vhdl << "\"UNSIGNED\"";
            }
            vhdl << "," << endl;
            vhdl << "\t\t\t" << "representation_b => ";
            if(mul->sign_b) {
                vhdl << "\"SIGNED\"";
            } else {
                vhdl << "\"UNSIGNED\"";
            }
            vhdl << "," << endl;
            vhdl << "\t\t\t" << "signed_pipeline_register_a => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "signed_pipeline_register_b => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "signed_register_a => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "signed_register_b => \"UNREGISTERED\"," << endl;
            vhdl << "\t\t\t" << "width_a => 18," << endl;
            vhdl << "\t\t\t" << "width_b => 18," << endl;
            vhdl << "\t\t\t" << "width_result => 36" << endl;

            vhdl << "\t\t) port map(" << endl;
            vhdl << "\t\t\tclock0 => '1'," << endl;
            vhdl << "\t\t\tdataa => ";
            for(int i = 17; i >= 0; i--) {
                vhdl << mul->inputPorts[i]->connectedNet->name << " ";
                if(i > 0) vhdl << "& ";
            }
            vhdl << ", " << endl;
            vhdl << "\t\t\tdatab => ";
            for(int i = 35; i >= 18; i--) {
                vhdl << mul->inputPorts[i]->connectedNet->name << " ";
                if(i > 18) vhdl << "& ";
            }
            vhdl << ", " << endl;
            vhdl << "\t\t\tresult => " << mul->name << "_q);" << endl;
            for(int i = 0 ; i < mul->outputPorts.size(); i++) {
                vhdl << "\t" << mul->outputPorts[i]->connectedNet->name << " <= " << mul->name + "_q(" << i << ");" << endl;
            }
            return vhdl.str();
        }


        string CycloneIIITechnology::PrintResourceUsage(LogicDesign *topLevel) {
            //Make sure we don't count devices twice
            set<LogicDevice*> counted;
            int totalLEcount = 0;
            int combLEcount = 0;
            int regLEcount = 0;
            int ramBitCount = 0;
            int mul9count  = 0;
            for(auto dev : topLevel->devices) {
                Altera_CarrySum *cs = dynamic_cast<Altera_CarrySum*>(dev);
                if(cs != nullptr) {
                    //Both LUTs driving a CarrySum are merged into one LE and therefore
                    //only need to be counted once
                    totalLEcount++;
                    combLEcount++;
                    for(int i = 0; i < 2; i++) {
                        LogicDevice *drivingDev = cs->inputPorts[i]->connectedNet->GetDriver();
                        if(drivingDev != nullptr) { //port is being driven by a LogicDevice
                            LUT *drivingLut = dynamic_cast<LUT*>(drivingDev);
                            if(drivingLut != nullptr) { //port is being driven by a LUT
                                if(drivingLut->inputPorts.size() <= 3) {
                                    counted.insert(drivingLut);
                                }
                            }
                        }
                    }

                    //Up to one register can also be included
                    for(auto driven : cs->outputPorts[0]->connectedNet->connectedPorts) {
                        DeviceInputPort *dip = dynamic_cast<DeviceInputPort*>(driven);
                        if(dip != nullptr) {
                            FlipFlop *ff = dynamic_cast<FlipFlop*>(dip->device);
                            if(ff != nullptr) {
                                counted.insert(ff);
                                regLEcount++;
                            }
                        }
                    }
                }
                Altera_ROM *rom = dynamic_cast<Altera_ROM*>(dev);
                if(rom != nullptr) {
                    ramBitCount += (rom->width * rom->length);
                }
            }

            for(auto dev : topLevel->devices) {
                if(counted.find(dev) == counted.end()) {
                    LUT *lut = dynamic_cast<LUT*>(dev);
                    if(lut != nullptr) {
                        totalLEcount++;
                        combLEcount++;
                        //Up to one register can also be included with a lut
                        for(auto driven : lut->outputPorts[0]->connectedNet->connectedPorts) {
                            DeviceInputPort *dip = dynamic_cast<DeviceInputPort*>(driven);
                            if(dip != nullptr) {
                                FlipFlop *ff = dynamic_cast<FlipFlop*>(dip->device);
                                if(ff != nullptr) {
                                    counted.insert(ff);
                                    regLEcount++;
                                }
                            }
                        }
                    }

                }
            }

            //final remaining extra flipflops
            for(auto dev : topLevel->devices) {
                if(counted.find(dev) == counted.end()) {
                    FlipFlop *ff = dynamic_cast<FlipFlop*>(dev);
                    if(ff != nullptr) {
                        totalLEcount++;
                        regLEcount++;
                    }
                }
            }

            //count multipliers
            for(auto dev : topLevel->devices) {
                Altera_Multiplier18 *mul18 = dynamic_cast<Altera_Multiplier18*>(dev);
                if(mul18 != nullptr) {
                    mul9count += 2; //each 18x18 is two 9x9s
                }
            }
            stringstream message;
            message << "Cyclone III resource utilisation\n";
            message << "  " <<  totalLEcount << " logic elements\n";
            message << "     " <<  combLEcount << " combinational\n";
            message << "     " <<  regLEcount << " registers\n";
            message << "  " <<  ramBitCount << " RAM bits\n";
            message << "  " <<  mul9count << " 9-bit multipliers";

            return message.str();
        }

        void CycloneIIITechnology::SetDeviceConstraint(const vector<string>& line) {
          if(line[1] == "LITEMODE") {
            if(line[2] == "ON") {
              lite_mode = true;
            } else {
              lite_mode = false;
            }
          }
        }
    }
}
