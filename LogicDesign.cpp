#include "LogicDesign.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <fstream>
#include "Util.hpp"
#include "BasicDevices.hpp"
#include "Altera/CycloneIIITechnology.hpp"
#include "Altera/AlteraDevices.hpp"
#include "Xilinx/Artix7Technology.hpp"
using namespace std;

namespace SynthFramework {
    namespace Polymer {
        LogicDesign::LogicDesign() {
            gnd = new Signal();
            gnd->name = "'0'";
            ConstantDevice *gndDriver = new ConstantDevice(gnd, false);
            vcc = new Signal();
            vcc->name = "'1'";
            ConstantDevice *vccDriver = new ConstantDevice(vcc, true);
        }

        void LogicDesign::LoadDesign(string data, string prefix, const map<string, Bus*> &portmap) {
            istringstream dss(data);
            string line;
            int lno = 1;
            while(getline(dss, line)) {
                if (line.find('#') != std::string::npos) {
					line = line.substr(0, line.find('#'));
				}
				vector<string> splitLine;
				istringstream iss(line);
				std::copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(splitLine));
				if (splitLine.size() == 0) continue;
                if(splitLine[0] == "INPUT") { //Input defintiion, syntax INPUT <NAME> <TYPE> <WIDTH>
                    Bus *inputBus = ParseSignalDefinition(splitLine, prefix);
                    AddBus(inputBus);
                    if(prefix == "") {
                        DesignIO *din = new DesignIO();
                        din->is_output = false;
                        din->IOName = inputBus->name;
                        din->width = inputBus->width;
                        din->is_signed = inputBus->is_signed;
                        din->assocBus = inputBus;
                        io.push_back(din);
                        int index = 0;
                        for(auto signal : inputBus->signals) {
                            if((inputBus->name) == "clock" && (inputBus->width == 1) && (prefix == "")) {
                                clockSignal = signal;
                            }
                            DesignInputPort *dip = new DesignInputPort();
                            dip->connectedNet = signal;
                            signal->connectedPorts.push_back(dip);
                            dip->design = this;
                            dip->linkedIO = din;
                            dip->busIndex = index;
                            din->assocPorts.push_back(dip);
                            inputPorts.push_back(dip);
                            index++;
                        }
                    } else {
                        //We're in a submodule, so wire up the input to a top level signal
                        try {
                            Operation *wire = new Operation(OPER_CONNECT, vector<Bus*>{portmap.at(splitLine[1])}, inputBus);
                            wire->name = prefix + "_connect_" + splitLine[1];
                            operations.push_back(wire);
                        } catch(out_of_range e) {
                            PrintMessage(MSG_ERROR, "Unmapped input " + splitLine[1] + " in submodule " + prefix);
                        }
                    }

                } else if(splitLine[0] == "OUTPUT") { //Output defintiion, syntax OUTPUT <NAME> <TYPE> <WIDTH>
                    Bus *outputBus = ParseSignalDefinition(splitLine, prefix);
                    if(prefix == "") {
                        //Add prefix to enable output buffers
                        string origName = outputBus->name;
                        outputBus->name = origName + "_int";
                        AddBus(outputBus);
                        outputBus->name = origName;
                        DesignIO *dout = new DesignIO();
                        dout->is_output = true;
                        dout->IOName = origName;
                        dout->width = outputBus->width;
                        dout->is_signed = outputBus->is_signed;
                        dout->assocBus = outputBus;
                        io.push_back(dout);
                        int index = 0;
                        for(auto signal : outputBus->signals) {
                            DesignOutputPort *dop = new DesignOutputPort();
                            dop->connectedNet = signal;
                            signal->connectedPorts.push_back(dop);
                            dop->design = this;
                            dop->linkedIO = dout;
                            dop->busIndex = index;
                            dout->assocPorts.push_back(dop);
                            outputPorts.push_back(dop);
                            index++;
                        }
                    } else {
                        AddBus(outputBus);
                        //We're in a submodule, so wire up the output to a top level signal
                        try {
                            Operation *wire = new Operation(OPER_CONNECT, vector<Bus*>{outputBus}, portmap.at(splitLine[1]));
                            wire->name = prefix + "_connect_" + splitLine[1];
                            operations.push_back(wire);
                        } catch(out_of_range e) {
                            PrintMessage(MSG_ERROR, "Unmapped output " + splitLine[1] + " in submodule " + prefix);
                        }

                    }

                } else if(splitLine[0] == "SIGNAL") { //Internal signal definition, syntax SIGNAL <NAME> <TYPE> <WIDTH>
                    Bus *sigBus = ParseSignalDefinition(splitLine, prefix);
                    AddBus(sigBus);
                } else if(splitLine[0] == "OPER") { //Some kind of operation
                    if(splitLine.size() < 4) {
                        PrintMessage(MSG_ERROR, "Invalid operation definition");
                    }
                    OperationType type;
                    if(!GetOperationByName(splitLine[1], type)) {
                        PrintMessage(MSG_ERROR, "Invalid operation type");
                    }
                    int nOperands = OperationInfo[type].nOperands;
                    vector<Bus*> operands;
                    if(splitLine.size() < (nOperands+3)) {
                        PrintMessage(MSG_ERROR, "Insufficient operand count");
                    }
                    for(int i = 0; i < nOperands; i++) {
                        Bus* oper = FindBusByName(prefix + splitLine[i+2]);
                        if(oper == nullptr) {
                            PrintMessage(MSG_ERROR, "Can't find '" + prefix + splitLine[i+2] + "'");
                        }
                        operands.push_back(oper);
                    }
                    Bus *output = FindBusByName(prefix + splitLine[nOperands+2]);
                    if(output == nullptr) {
                        PrintMessage(MSG_ERROR, "Can't find '" + prefix + splitLine[nOperands+2] + "'");
                    }
                    Operation *operation = new Operation(type, operands, output);
                    operation->name = prefix + "oper" + to_string(lno);
                    operations.push_back(operation);
                } else if(splitLine[0] == "TARGET") {
                    if(splitLine.size() < 2) {
                        PrintMessage(MSG_ERROR, "Named target required");
                    }

                    if(splitLine[1] == "CYCLONEIII") {
                        technology = new CycloneIIITechnology();
                    } else if(splitLine[1] == "ARTIX7") {
                        technology = new Artix7Technology();
                    } else {
                        PrintMessage(MSG_ERROR, "Target " + splitLine[1] + " not supported");
                    }
                } else if(splitLine[0] == "OPTION") {
                    if(splitLine.size() < 3) {
                        PrintMessage(MSG_ERROR, "Option and value required");
                    }

                    if(splitLine[1] == "PIPELINE") {
                        allowPipeline = (splitLine[2] == "ON");
                    }
                } else if(splitLine[0] == "CONSTRAINT") {
                    bool is_int = false;
                    bool is_other = false;
                    int *iDest;
                    double *dDest;
                    if(splitLine[1] == "FREQUENCY") {
                        is_int = false;
                        dDest = &targetFrequency;
                    } else if(splitLine[1] == "BUDGET") {
                        is_int = false;
                        dDest = &timingBudget;
                    } else if(splitLine[1] == "SLACK") {
                        is_int = false;
                        dDest = &timingSlack;
                    } else if(splitLine[1] == "MAXLATENCY") {
                        is_int = true;
                        iDest = &maxLatency;
                    } else if(splitLine[1] == "MINLATENCY") {
                        is_int = true;
                        iDest = &minLatency;
                    } else if(splitLine[1] == "SLOW") {
                        is_other = true;
                        Bus *bus = FindBusByName(prefix + splitLine[2]);
                        for(auto sig : bus->signals) {
                          sig->isSlow = true;
                        }
                    } else {
                        PrintMessage(MSG_ERROR, "unknown constraint " + splitLine[1]);
                    }
                    if(!is_other) {
                      if(is_int) {
                          *iDest = stoi(splitLine[2]);
                      } else {
                          *dDest = stod(splitLine[2]);
                      }
                    }

                } else if(splitLine[0] == "CONSTANT") {
                    Bus *cnstBus = ParseSignalDefinition(splitLine, prefix);
                    AddBus(cnstBus);
                    if(splitLine.size() < 5) {
                        PrintMessage(MSG_ERROR, "constant must have value");
                    }
                    string cv = splitLine[4];
                    int bit = 0;
                    for(int i = cv.size() - 1; i >= 0; i--) {
                        if(bit >= cnstBus->width) break;
                        if(cv[i] == '1') {
                            cnstBus->signals[bit]->ConnectTo(vcc);
                        } else if(cv[i] == '0') {
                            cnstBus->signals[bit]->ConnectTo(gnd);
                        } else {
                            PrintMessage(MSG_ERROR, "unexpected character in binary constant");
                        }
                        bit++;
                    }
                } else if(splitLine[0] == "ROM") {
                    if(splitLine.size() < 6) {
                        PrintMessage(MSG_ERROR, "invalid rom statement");
                    }
                    int width = stoi(splitLine[1]);
                    int length = stoi(splitLine[2]);
                    string mifname = splitLine[3];
                    Bus *addressBus = FindBusByName(prefix + splitLine[4]);
                    if(addressBus == nullptr) {
                        PrintMessage(MSG_ERROR, "Can't find '" + prefix + splitLine[4] + "'");
                    }
                    Bus *dataBus = FindBusByName(prefix + splitLine[5]);
                    if(dataBus == nullptr) {
                        PrintMessage(MSG_ERROR, "Can't find '" + prefix + splitLine[5] + "'");
                    }
                    devices.push_back(new Altera_ROM(width, length, mifname, clockSignal, addressBus->signals, dataBus->signals));
                } else if(splitLine[0] == "DEVOPT") {
                    if(technology != nullptr) {
                      technology->SetDeviceConstraint(splitLine);
                    } else{
                      PrintMessage(MSG_ERROR, "technology must be set up before a DEVOPT statement");
                    }
                } else if(splitLine[0] == "LATCON") {
                  //latency constraint:
                  //LATCON output input ext_latency
                  LatencyConstraint *latcon = new LatencyConstraint();
                  Bus *outbus = FindBusByName(prefix + splitLine[1]);
                  Bus *inbus = FindBusByName(prefix + splitLine[2]);
                  latcon->ext_latency = stoi(splitLine[3]);

                  for(auto outsig : outbus->signals) {
                    DesignOutputPort *dop = nullptr;
                    for(auto port : outsig->connectedPorts) {
                      if(dynamic_cast<DesignOutputPort*>(port) != nullptr) {
                        dop = dynamic_cast<DesignOutputPort*>(port);
                        break;
                      }
                    }
                    if(dop != nullptr) {
                      dop->latcons.push_back(latcon);
                      latcon->outputs.push_back(dop);
                    }

                  }

                  for(auto insig : inbus->signals) {
                    DesignInputPort *dip = nullptr;
                    for(auto port : insig->connectedPorts) {
                      if(dynamic_cast<DesignInputPort*>(port) != nullptr) {
                        dip = dynamic_cast<DesignInputPort*>(port);
                        break;
                      }
                    }
                    if(dip != nullptr) {
                      dip->latcon = latcon;
                      latcon->inputs.push_back(dip);
                    }
                  }
                  latcons.push_back(latcon);
                } else if(splitLine[0] == "INCLUDE") {
                  //Include a submodule
                  //Syntax:
                  //INCLUDE submodule_name instance_name SUBMODULE_IO1=>DESIGN_BUS1 SUBMODULE_IO2=>DESIGN_BUS2 ...
                  ifstream infs;
                  vector<string> allowedExtensions{".polynet.enc", ".ecc.polynet.enc", ".polynet", ".ecc.polynet"};
                  bool found = false;
                  for(auto dir : searchPath) {
                      for(auto ext : allowedExtensions) {
                          infs.open(dir + "/" + splitLine[1] + ext);
                          if(infs.is_open()) {
                              found = true;
                              break;
                          }
                      }
                      if(found)
                        break;
                  }
                  if(!found) {
                      PrintMessage(MSG_ERROR, "failed to find submodule " + splitLine[1] + " in any current directories");
                  }
                  string inst_name = splitLine[2];
                  map<string, Bus*> portmap;
                  for(int i = 3; i < splitLine.size(); i++) {
                      string assoc = splitLine[i];
                      int delimPos = assoc.find("=>");
                      if(delimPos == string::npos) {
                          PrintMessage(MSG_ERROR, "invalid port mapping definition");
                      }
                      string modulePort = assoc.substr(0, delimPos);
                      string topBusName = assoc.substr(delimPos+2);
                      Bus * topBus = FindBusByName(prefix + topBusName);
                      if(topBus == nullptr) {
                          PrintMessage(MSG_ERROR, "bus " + topBusName + " in port map not found");
                      }
                      portmap[modulePort] = topBus;
                   }
                   string moduleCode = string((istreambuf_iterator<char>(infs)), istreambuf_iterator<char>());
                   LoadDesign(moduleCode, prefix + inst_name + "__" , portmap);
                } else {
                    PrintMessage(MSG_ERROR, "syntax error in netlist");
                }
                lno++;
            }
        };


        void LogicDesign::SynthesiseAndOptimiseDesign() {
            for(auto oper : operations) {
                oper->Synthesise(this);
            }
            PrintMessage(MSG_NOTE, "basic synthesis produced " + to_string(devices.size()) + " devices before optimisation");

            //Run connectivity check before optimisation
            for(auto sig : signals) {
                int numDrivers = 0, numDriven = 0;
                for(auto port : sig->connectedPorts) {
                    if(port->IsDriver())
                       numDrivers++;
                    else
                       numDriven++;
                }
                if((numDrivers == 0)  && (numDriven > 0)) {
                    PrintMessage(MSG_WARNING, "net ===" + sig->name + "=== has non-zero fanout and no driver");
                } else if(numDrivers > 1) {
                    PrintMessage(MSG_WARNING, "net ===" + sig->name + "=== has multiple drivers");
                }
            }

            //Keep optimising until no more optimisations are possible
            bool didOptimise = false;
            do {
                didOptimise = false;
            //    PrintMessage(MSG_NOTE, "optimising");

                //Run device specific optimisations
                for(auto device : devices) {
                    if(device->OptimiseDevice(this)) {
                        didOptimise = true;
                    }
                }
                vector<LogicDevice*> devicesToPurge;
                //Optimise away devices with no fanout
                for(int i = 0; i < devices.size(); i++) {
                    bool hasFanout = false;
                    for(auto outPin : devices[i]->outputPorts) {
                        if(outPin->connectedNet->GetFanout() > 0) {
                            hasFanout = true;
                        }
                    }
                    if(!hasFanout) {
                        didOptimise = true;
                        PrintMessage(MSG_DEBUG, "device ===" + devices[i]->name + "=== has no fanout and will be removed");
                        devicesToPurge.push_back(devices[i]);
                    }
                }

                //Optimise away duplicate devices
                for(auto sig : signals) {
                    //Look for devices with a common input pin to reduce the cost of the search
                    for(auto a : sig->connectedPorts) {
                        for(auto b : sig->connectedPorts) {
                            if(a != b) {
                                DeviceInputPort *dia = dynamic_cast<DeviceInputPort*>(a), *dib = dynamic_cast<DeviceInputPort*>(b);
                                if((dia != nullptr) && (dib != nullptr)) {
                                   if(dia->device != dib->device) {
                                       if(dia->device->IsEquivalentTo(dib->device)) {
                                           if((find(devicesToPurge.begin(), devicesToPurge.end(), dia->device) == devicesToPurge.end())
                                                && (find(devicesToPurge.begin(), devicesToPurge.end(), dib->device) == devicesToPurge.end())) {
                                                    didOptimise = true;
                                                    for(int i = 0; i < dia->device->outputPorts.size(); i++) {
                                                        Signal *oldNet = dib->device->outputPorts[i]->connectedNet;
                                                        PrintMessage(MSG_DEBUG, "nets ===" + oldNet->name + "=== and ===" + dia->device->outputPorts[i]->connectedNet->name + "=== are driven by identical devices and will be merged");
                                                        dib->device->outputPorts[i]->Disconnect();
                                                        oldNet->ConnectTo(dia->device->outputPorts[i]->connectedNet);
                                                    }
                                                    devicesToPurge.push_back(dib->device);
                                                }

                                       }
                                   }
                                }
                             }
                        }
                    }
                }

                for(auto dtp : devicesToPurge) {
                    int indexInArray = -1;
                    for(int i = 0; i < devices.size(); i++) {
                        if(devices[i] == dtp) {
                            indexInArray = i;
                        }
                    }
                    if(indexInArray != -1) {
                        devices.erase(devices.begin() + indexInArray);
                    }
                    for(auto inp : dtp->inputPorts) {
                        inp->Disconnect();
                    }
                    for(auto outp : dtp->outputPorts) {
                        outp->Disconnect();
                    }
                }
            } while(didOptimise);

            int lutTotal = 0, ffTotal = 0;
            for(auto dev : devices) {
                if(dynamic_cast<LUT*>(dev) != nullptr) {
                    lutTotal++;
                } else if(dynamic_cast<FlipFlop*>(dev) != nullptr) {
                    ffTotal++;
                }
            }
            PrintMessage(MSG_NOTE, "optimised design contains " + to_string(lutTotal) + " LUT and " + to_string(ffTotal) + " FF");
        }

        void LogicDesign::AnalyseTiming() {
            set<LogicDevice*> analysed;
            for(auto output : outputPorts) {
                LogicDevice* outputDriver = output->connectedNet->GetDriver();
                if(outputDriver != nullptr) {
                    AnalyseTimingRecursive(outputDriver, false, analysed);
                }
            }
        }

        void LogicDesign::AnalyseTimingRecursive(LogicDevice* target, bool stopAtRegister, set<LogicDevice*> &analysed) {
            if(analysed.find(target) != analysed.end()) {
                return;
            }
            analysed.insert(target);
            LUT* lut = dynamic_cast<LUT*>(target);
            FlipFlop *ff = dynamic_cast<FlipFlop*>(target);
            for(auto inp : target->inputPorts) {
                LogicDevice* outputDriver = inp->connectedNet->GetDriver();
                if(outputDriver != nullptr) {
                    if(stopAtRegister && (ff != nullptr))
                        continue;
                    AnalyseTimingRecursive(outputDriver, stopAtRegister, analysed);
                }
            }

            if(lut != nullptr) {
                //Consider routing delays, different depending on input origin
                double worst_input_tpd = 0;
                for(auto input : lut->inputPorts) {
                    LogicDevice* outputDriver = input->connectedNet->GetDriver();
                    if(outputDriver != nullptr) {
                        if(dynamic_cast<LUT*>(outputDriver) != nullptr) {
                            worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay + technology->GetRoutingDelay_LUT_LUT());
                        } else {
                            worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay);
                        }
                    } else {
                        worst_input_tpd = max(worst_input_tpd, input->connectedNet->delay);
                    }
                }
                lut->outputPorts[0]->connectedNet->delay = worst_input_tpd + technology->GetLUTTpd(lut);
            } else if(ff != nullptr) {
                ff->outputPorts[0]->connectedNet->delay = technology->GetFFTpd();
            } else {
                VendorSpecificDevice *vsd = dynamic_cast<VendorSpecificDevice*>(target);
                if(vsd != nullptr) {
                    technology->AnalyseTiming(vsd, this);
                }
            }
        }

        void LogicDesign::AnalysePostPipelineTiming() {
            set<LogicDevice*> analysed;
            double worst_slack = numeric_limits<double>::infinity();
            double TNS = 0;

            for(auto dev : devices) {
                FlipFlop *ff = dynamic_cast<FlipFlop*>(dev);
                if(ff != nullptr) {
                    LogicDevice *drv = ff->inputPorts[0]->connectedNet->GetDriver();
                    if(drv != nullptr) {
                        AnalyseTimingRecursive(drv, true, analysed);
                    }
                    double budget = (1.0 / targetFrequency) - technology->GetFFSetupTime();
                    double slack = budget - ff->inputPorts[0]->connectedNet->delay;
                    if(slack < worst_slack) {
                        worst_slack = slack;
                    }
                    if(slack < 0) {
                        TNS += -slack;
                    }
                }
            }

            if(worst_slack >= 0) {
                PrintMessage(MSG_NOTE,"pipelined design meets timing requirements\nworst setup slack = " + to_string(worst_slack * 1e9) + "ns");
            } else {
                PrintMessage(MSG_WARNING,"pipelined design fails to meet timing requirements\nworst setup slack = " + to_string(worst_slack * 1e9) + "ns\n"
                        + "TNS = " + to_string(TNS * 1e9) + "ns");

            }
        }

        void LogicDesign::PipelineDesign() {
            if(!allowPipeline) return;
            if(clockSignal == nullptr) {
                PrintMessage(MSG_WARNING, "pipeline not possible without global clock signal");
                return;
            }

            //Process latency constraints first (TODO: complex chains of constraints)
            for(auto latcon : latcons) {
              int maxLatency = 0;
              for(auto output : latcon->outputs) {
                  LogicDevice* outputDriver = output->connectedNet->GetDriver();
                  if(outputDriver != nullptr) {
                      PipelineDesignRecursive(outputDriver);
                  }
                  maxLatency = max(maxLatency, output->connectedNet->latency);
              }
              PrintMessage(MSG_DEBUG, "latency for latcon == " + to_string(maxLatency));
              for(auto output : latcon->outputs) {
                  if((output->connectedNet != gnd) && (output->connectedNet != vcc) && (!output->connectedNet->isSlow)) {
                      while(output->connectedNet->latency < maxLatency) {
                          Signal *pipelined = PipelineSignal(output->connectedNet);
                          output->Disconnect();
                          output->connectedNet = pipelined;
                          pipelined->connectedPorts.push_back(output);
                      }
                  }
              }

              for(auto input : latcon->inputs) {
                input->connectedNet->latency = maxLatency + latcon->ext_latency;
              }
            }

            for(auto output : outputPorts) {
                if(output->latcons.size() == 0) {
                  LogicDevice* outputDriver = output->connectedNet->GetDriver();
                  if(outputDriver != nullptr) {
                      PipelineDesignRecursive(outputDriver);
                  }
                }

            }
            int maxLatency = 0;
            for(auto output : outputPorts) {
                maxLatency = max(maxLatency, output->connectedNet->latency);
            }
            PrintMessage(MSG_NOTE, "pipeline latency is " + to_string(maxLatency));

            for(auto output : outputPorts) {
                if((output->connectedNet != gnd) && (output->connectedNet != vcc) && (!output->connectedNet->isSlow) && (output->latcons.size() == 0)) {
                    while(output->connectedNet->latency < maxLatency) {
                        Signal *pipelined = PipelineSignal(output->connectedNet);
                        output->Disconnect();
                        output->connectedNet = pipelined;
                        pipelined->connectedPorts.push_back(output);
                    }
                }
            }

            int lutTotal = 0, ffTotal = 0;
            for(auto dev : devices) {
                if(dynamic_cast<LUT*>(dev) != nullptr) {
                    lutTotal++;
                } else if(dynamic_cast<FlipFlop*>(dev) != nullptr) {
                    ffTotal++;
                }
            }
            PrintMessage(MSG_NOTE, "pipelined design contains " + to_string(lutTotal) + " LUT and " + to_string(ffTotal) + " FF");
        }

        void LogicDesign::PipelineDesignRecursive(LogicDevice* target) {
            LUT* lut = dynamic_cast<LUT*>(target);
            FlipFlop *ff = dynamic_cast<FlipFlop*>(target);
            //TODO: make a generic interface for pipelining ROMs etc
            Altera_ROM *rom = dynamic_cast<Altera_ROM*>(target);
            int maxInputLatency = 0;
            if(target->pipelineDone)
                return;
            target->pipelineDone = true;
            bool allAreSlow = true;
            for(auto inp : target->inputPorts) {
                LogicDevice* outputDriver = inp->connectedNet->GetDriver();
                if(outputDriver != nullptr) {
                    PipelineDesignRecursive(outputDriver);
                }
                if(!inp->connectedNet->isSlow) {
                  allAreSlow = false;
                }
                maxInputLatency = max(maxInputLatency, inp->connectedNet->latency);
            }

            if(ff != nullptr) {
                if(ff->isShift) {
                    target->outputPorts[0]->connectedNet->latency = target->inputPorts[0]->connectedNet->latency;
                } else {
                    target->outputPorts[0]->connectedNet->latency = target->inputPorts[0]->connectedNet->latency + 1;
                }
                return; //don't pipeline flip flops
            } else if(rom != nullptr) {
                for(int i = 1; i < rom->inputPorts.size(); i++) {
                    DeviceInputPort *inp = rom->inputPorts[i];
                    if((inp->connectedNet != gnd) && (inp->connectedNet != vcc) && (!inp->connectedNet->isSlow)) {
                        while(inp->connectedNet->latency < maxInputLatency) {
                            PipelinePin(inp);
                        }
                    }
                }
                for(auto op : rom->outputPorts) {
                    op->connectedNet->latency = maxInputLatency + 1;
                }
                return;
            }

            for(auto inp : target->inputPorts) {
                if((inp->connectedNet != gnd) && (inp->connectedNet != vcc) && (!inp->connectedNet->isSlow)) {
                    while(inp->connectedNet->latency < maxInputLatency) {
                        PipelinePin(inp);
                    }
                }

            }
            set<LogicDevice*> analysed;
            AnalyseTimingRecursive(target, true, analysed);


            double totalBudget = (timingBudget / targetFrequency) - (timingSlack + technology->GetFFSetupTime());
            bool needPipeline = false;
            for(auto outp : target->outputPorts) {
                outp->connectedNet->latency = maxInputLatency;
                if(outp->connectedNet->delay >= totalBudget) {
                    needPipeline = true;
                }
            }
            if(allAreSlow) {
              //Propogate 'slow' status through gates
              for(auto outp : target->outputPorts) {
                  outp->connectedNet->isSlow = true;
              }
            } else {
              if(needPipeline) {
                  for(auto inp : target->inputPorts) {
                      PipelinePin(inp);
                  }

                  for(auto outp : target->outputPorts) {
                      outp->connectedNet->latency = maxInputLatency + 1;
                  }
              }
            }


        }

        Signal* LogicDesign::PipelineSignal(Signal* source) {
            if(source == gnd) {
                return gnd;
            } else if(source == vcc) {
                return vcc;
            }
            //This avoids creating a duplicate pipeline register if a perfectly good one already exists
            for(auto port : source->connectedPorts) {
                DeviceInputPort *dip = dynamic_cast<DeviceInputPort*>(port);
                if(dip != nullptr) {
                    FlipFlop *ff = dynamic_cast<FlipFlop*>(dip->device);
                    if(ff != nullptr) {
                        //Check the flip flop is not in any way funky. Once these signals start being used it should also check
                        //that if global enable/reset are used then these are connected correctly
                        if((dip == ff->inputPorts[0]) && (ff->inputPorts[1]->connectedNet == clockSignal) && (ff->hasReset == globalHasReset) && (ff->hasEnable == globalHasEnable)) {
                            return ff->outputPorts[0]->connectedNet;
                        }
                    }
                }
            }
            //Assume at this point no suitable register exists already, so create one
            Signal *reg = CreateSignal(source->name + "_reg");
            FlipFlop *newff = new FlipFlop(source, clockSignal, reg);
            devices.push_back(newff);
            reg->latency = source->latency + 1;
            return reg;
        }

        void LogicDesign::PipelinePin(DeviceInputPort* port) {
            Signal *pipelined = PipelineSignal(port->connectedNet);
            port->Disconnect();
            port->connectedNet = pipelined;
            pipelined->connectedPorts.push_back(port);
        }

        Bus* LogicDesign::ParseSignalDefinition(const vector<string> &splitLine, string prefix) {
            if(splitLine.size() < 4) {
                PrintMessage(MSG_ERROR, "invalid signal definition");
            }
            string signalName = prefix + splitLine[1];
            string signalType = splitLine[2];
            int signalWidth = stoi(splitLine[3]);
            Bus *signalBus = new Bus();
            signalBus->name = signalName;
            signalBus->is_signed = (signalType == "SIGNED");
            signalBus->width = signalWidth;
            return signalBus;
        }

        Bus* LogicDesign::FindBusByName(string name) {
            bool usingRange = false;
            int rangeStart, rangeEnd;
            if(name.find('[') != string::npos) {
                usingRange = true;
                string rangeSpec = name.substr(name.find('[') + 1);
                name = name.substr(0, name.find('['));
                if(rangeSpec.back() != ']') {
                    PrintMessage(MSG_ERROR, "invalid range specifier");
                }
                rangeSpec = rangeSpec.substr(0, rangeSpec.length() - 1);
                if(rangeSpec.find(':') == string::npos) {
                    PrintMessage(MSG_ERROR, "invalid range specifier");
                }
                string rangeEndStr = rangeSpec.substr(0, rangeSpec.find(':'));
                string rangeStartStr = rangeSpec.substr(rangeSpec.find(':')+1);
                rangeStart = stoi(rangeStartStr);
                rangeEnd = stoi(rangeEndStr);
            }
            Bus *foundBus = nullptr;
            for(auto bus : buses) {
                if(bus->name == name) {
                  foundBus = bus;
                  break;
                }
            }
            if(foundBus == nullptr) {
                return nullptr;
            } else if(usingRange) {
                Bus *rangeBus = new Bus();
                rangeBus->is_signed = foundBus->is_signed;
                rangeBus->width = (rangeEnd - rangeStart) + 1;
                rangeBus->name = name;
                for(int i = rangeStart; i <= rangeEnd; i++) {
                    if(i >= foundBus->signals.size()) {
                        PrintMessage(MSG_ERROR, "subrange out of bounds");
                    }
                    rangeBus->signals.push_back(foundBus->signals[i]);
                    foundBus->signals[i]->parentBuses.push_back(rangeBus);
                }
                return rangeBus;
            } else {
                return foundBus;
            }
        }

        void LogicDesign::AddBus(Bus *b) {
            for(int i = 0; i < b->width; i++) {
                Signal *sig;
                if(b->width > 1) {
                    sig = CreateSignal(b->name + "_bus_" + to_string(i));
                } else {
                    sig = CreateSignal(b->name);
                }
                sig->parentBuses.push_back(b);
                b->signals.push_back(sig);
            }
            buses.push_back(b);
        }

        Signal *LogicDesign::CreateSignal(string name) {
            Signal *sig = new Signal();
            sig->name = name;
            signals.push_back(sig);
            return sig;
        }

        Bus *LogicDesign::CreateConstantBus(int value) {
            Bus *cBus = new Bus();
            cBus->name = "cnst_" + to_string(value);
            cBus->is_signed = false;
            cBus->width = 0;
            do {
                if((value & 0x1) != 0) {
                    cBus->signals.push_back(vcc);
                } else {
                    cBus->signals.push_back(gnd);
                }
                value = value >> 1;
                cBus->width++;
            } while(value > 0);
            return cBus;
        }

        string LogicDesign::GenerateVHDL() {
            stringstream vhdl;
            vhdl << "--Generated from Polymer design " << designName << endl;
            vhdl << "library ieee ;" << endl;
            vhdl << "use ieee.std_logic_1164.all;" << endl;
            vhdl << "use ieee.std_logic_misc.all;" << endl;
            vhdl << "use ieee.numeric_std.all;" << endl << endl;
            vhdl << technology->GenerateInclude() << endl;

            vhdl << "entity " << designName << " is" << endl;
            vhdl << "\tport(" << endl;

            //a seperate stringstream is used so we can remove the trailing end semicolon
            stringstream port;
            int ioCount = 0;
            //one bit inputs don't need another signal
            vector<Signal*> onebits;
            for(auto iospec : io) {
                port << "\t\t" << iospec->IOName;
                if(iospec->is_output) {
                    port << " : out ";
                } else {
                    port << " : in ";
                }
                if(iospec->width == 1) {
                    port << "std_logic;" << endl;
                    if(!iospec->is_output) {
                        onebits.push_back(iospec->assocBus->signals[0]);
                    }
                } else {
                    port << "std_logic_vector(" << (iospec->width - 1) << " downto 0);" << endl;
                }
                ioCount++;
            }
            if(ioCount == 0) {
                PrintMessage(MSG_ERROR, "entity " + designName + " must have at least one input or output");
            }

            //Trim final semicolon
            string portdef = port.str();
            portdef = portdef.substr(0,portdef.find_last_of(';'));
            vhdl << portdef << endl << "\t\t);" << endl;
            vhdl << "end " << designName << ";" << endl << endl;

            vhdl << "architecture polymer_synth of " << designName << " is" << endl;
            for(auto signal : signals) {
                if((signal->connectedPorts.size() > 0) && (find(onebits.begin(), onebits.end(), signal) == onebits.end())) {
                    vhdl << "\tsignal " << signal->name << " : std_logic;" << endl;
                }
            }
            for(auto dev : devices) {
                vhdl << technology->GenerateDeviceSignals(dev); //some devices will during synthesis require extra signals to be created
            }

            vhdl << "begin" << endl;

            //Input and output statements
            for(auto iospec : io) {
                if(iospec->is_output) {
                    if(iospec->width > 1) {
                        for(int i = 0; i < iospec->width; i++) {
                            vhdl << "\t" << iospec->IOName << "(" << i << ") <= " << iospec->assocPorts[i]->connectedNet->name << ";" << endl;
                        }
                    } else {
                        vhdl << "\t" << iospec->IOName << " <= " << iospec->assocPorts[0]->connectedNet->name << ";" << endl;
                    }
                } else {
                    if(iospec->width > 1) {
                        for(int i = 0; i < iospec->width; i++) {
                            vhdl << "\t" << iospec->assocPorts[i]->connectedNet->name << " <= " << iospec->IOName << "(" << i << ");" << endl;
                        }
                    }
                }

                ioCount++;
            }

            //Devices themselves
            for(auto dev : devices) {
                vhdl << technology->GenerateDevice(dev) << endl;
            }

            vhdl << "end polymer_synth;" << endl;
            return vhdl.str();
        }

        bool DesignInputPort::IsDriver() {
            return true;
        }

        bool DesignInputPort::IsConstantDriver() {
            return false;
        }

        LogicState DesignInputPort::GetConstantState() {
            return LogicState_Unknown;
        }

        bool DesignOutputPort::IsDriver() {
            return false;
        }

        bool DesignOutputPort::IsConstantDriver() {
            return false;
        }

        LogicState DesignOutputPort::GetConstantState() {
            return LogicState_Unknown;
        }

    }
}
