#include "BasicDevices.hpp"
#include "Util.hpp"
#include "LogicDesign.hpp"
#include <typeinfo>
#include <sstream>
using namespace std;

namespace SynthFramework {
    namespace Polymer {
         //LUT initialisation values for standard gates
         map<LUTDeviceType, vector<bool> > LUT::initialContents {
            { Device_AND2, vector<bool> {0, 0, 0, 1} },
            { Device_OR2, vector<bool> {0, 1, 1, 1} },
            { Device_XOR2, vector<bool> {0, 1, 1, 0} },
            { Device_NAND2, vector<bool> {1, 1, 1, 0} },
            { Device_NOR2, vector<bool> {1, 0, 0, 0} },
            { Device_XNOR2, vector<bool> {1, 0, 0, 1} },
            { Device_AND3, vector<bool> {0, 0, 0, 0, 0, 0, 0, 1}},
            { Device_NOT, vector<bool> {1, 0} },
            { Device_FULLADD_SUM, vector<bool> {0, 1, 1, 0, 1, 0, 0, 1} },
            { Device_FULLADD_CARRY, vector<bool> {0, 0, 0, 1, 0, 1, 1, 1} },
            { Device_MUX2_1, vector<bool> {0, 1, 0, 1, 0, 0, 1, 1} } };

         //LUT related helper functions

         //Return true if a given input has no bearing on the output value
         bool IsInputRedundant(const vector<bool> &lut, int input) {
             //in theory only half the checks are needed, but that makes the loop more complicated
             for(int i = 0; i < lut.size(); i++) {
                 //compare lut contents at this point with lut contents with input inverted
                 if(lut[i] != lut[i ^ (1 << input)]) {
                     return false; //inverting input changes output, thus input not redundant
                 }
             }
             return true;
         }

         //Create a new LUT content array with one input eliminated and forced to a constant value
         void EliminateInput(const vector<bool> &initialLut, int input, bool value, vector<bool> &newLut) {
             //again this loop could in the future be optimised
             newLut.clear();
             for(int i = 0; i < initialLut.size(); i++) {
                 //pick LUT entry only if specified input is equal to value
                 if(((i & (1 << input)) != 0) == value) {
                     newLut.push_back(initialLut[i]);
                 }
             }
         }

         LUT::LUT() {
             name = "lut_" + to_string(lutCount);
             lutCount++;
         };

         LUT::LUT(LUTDeviceType type, vector<Signal*> inputs, Signal* output) {
             name = "lut_" + to_string(lutCount);
             lutCount++;
             lutContent = initialContents[type];
             for(int i = 0; i < inputs.size(); i++) {
                 DeviceInputPort *inp = new DeviceInputPort();
                 inp->device = this;
                 inp->pin = i;
                 inp->connectedNet = inputs[i];
                 inputs[i]->connectedPorts.push_back(inp);
                 inputPorts.push_back(inp);
             }

             DeviceOutputPort *outp = new DeviceOutputPort();
             outp->device = this;
             outp->pin = 0;
             outp->connectedNet = output;
             output->connectedPorts.push_back(outp);
             outputPorts.push_back(outp);
         }

         void LUT::MergeWith(LUT* other, int pin) {
             //Note the inputs from the 'other' LUT are put at the start of this one
             vector<bool> newContent;
             for(int i = 0; i < lutContent.size(); i++) {
                //only processing things when the value of the replaced pin is zero prevents the newContent array being duplicated
                if((i & (1 << pin)) == 0) {
                    for(int j = 0; j < other->lutContent.size(); j++) {
                        //evaluate all input combinations in the other LUT
                        if(other->lutContent[j]) {
                            //output of other LUT is high, thus pick from row in this LUT equivalent to i but where pin is high
                            newContent.push_back(lutContent[i | (1 << pin)]);
                        } else {
                            //output of other LUT is low, thus pick from row i where pin is low
                            newContent.push_back(lutContent[i]);
                        }
                    }
                }
             }
             inputPorts[pin]->Disconnect();
             inputPorts.erase(inputPorts.begin() + pin);
             //add inputs of 'other' LUT to this one at the start but in the correct order
             for(int i = other->inputPorts.size() - 1; i >= 0; i--) {
                 DeviceInputPort *port = new DeviceInputPort();
                 port->pin = i;
                 port->device = this;
                 port->connectedNet = other->inputPorts[i]->connectedNet;
                 other->inputPorts[i]->connectedNet->connectedPorts.push_back(port);
                 inputPorts.insert(inputPorts.begin(), port);
             }
             lutContent = newContent;
         }


         bool LUT::OptimiseDevice(LogicDesign *topLevel) {
             int redundantInput = -1, constantInput = -1;
             bool deviceOptimised = false;

             //Eliminate driven constant inputs
             do {
                 constantInput = -1;
                 bool inputVal = false;
                 for(int i = 0; i < inputPorts.size(); i++) {
                    if(inputPorts[i]->connectedNet->GetConstantValue(inputVal)) {
                        constantInput = i;
                        break;
                    }
                 }
                 if(constantInput != -1) {
                     deviceOptimised = true;
                     vector<bool> newLutContent;
                     EliminateInput(lutContent, constantInput, inputVal, newLutContent);
                     inputPorts[constantInput]->Disconnect();
                     inputPorts.erase(inputPorts.begin() + constantInput);
                     lutContent = newLutContent;
                 }

             } while(constantInput != -1);
             //Eliminate redundant inputs
             do {
                 redundantInput = -1;
                 for(int i = 0; i < inputPorts.size(); i++) {
                    if(IsInputRedundant(lutContent, i)) {
                        redundantInput = i;
                        break;
                    }
                 }
                 if(redundantInput != -1) {
                     deviceOptimised = true;
                     vector<bool> newLutContent;
                     EliminateInput(lutContent, redundantInput, false, newLutContent);
                     inputPorts[redundantInput]->Disconnect();
                     inputPorts.erase(inputPorts.begin() + redundantInput);
                     lutContent = newLutContent;
                 }

             } while(redundantInput != -1);

             //Eliminate duplicated inputs
             int firstInput = -1, duplicateInput = -1;
             do {
               firstInput = -1;
               duplicateInput = -1;
               for(int i = 0; i < inputPorts.size(); i++) {
                 for(int j = 0; j < inputPorts.size(); j++) {
                   if(i != j) {
                     if(inputPorts[i]->connectedNet == inputPorts[j]->connectedNet) {
                       firstInput = i;
                       duplicateInput = j;
                     }
                   }
                 }
               }
               if(duplicateInput != -1) {
                 PrintMessage(MSG_DEBUG, "inputs " + to_string(firstInput) + " and " + to_string(duplicateInput) + " to ===" + name + "=== are fed from the same net and will be merged");
                 vector<bool> newContent;
                 for(int i = 0; i < lutContent.size(); i++) {
                   //Only pick entries where the two inputs have the same value
                   bool stateA = ((i & (1 << firstInput)) != 0);
                   bool stateB = ((i & (1 << duplicateInput)) != 0);
                   if(stateA == stateB) {
                     newContent.push_back(lutContent[i]);
                   }
                 }
                 inputPorts[duplicateInput]->Disconnect();
                 inputPorts.erase(inputPorts.begin() + duplicateInput);
                 lutContent = newContent;
               }
             } while(duplicateInput != -1);
             if(inputPorts.size() == 0) {
                 deviceOptimised = true;

                 //0-size LUT is a constant value
                 outputPorts[0]->Disconnect();
                 PrintMessage(MSG_DEBUG, "net ===" + outputPorts[0]->connectedNet->name + "=== is stuck at " + to_string(lutContent[0]));
                 if(lutContent[0]) {
                     outputPorts[0]->connectedNet->ConnectTo(topLevel->vcc);
                 } else {
                     outputPorts[0]->connectedNet->ConnectTo(topLevel->gnd);
                 }
             } else {
                 if((inputPorts.size() == 1) && (!lutContent[0]) && (lutContent[1])) {
                     //Eliminate buffers
                     deviceOptimised = true;
                     outputPorts[0]->Disconnect();
                     PrintMessage(MSG_DEBUG, "elimated buffer between ===" + inputPorts[0]->connectedNet->name + "=== and ===" + outputPorts[0]->connectedNet->name + "===");
                     outputPorts[0]->connectedNet->ConnectTo(inputPorts[0]->connectedNet);
                 } else {
                     //See if we can merge any other LUTs into this one
                     int mergeInput = -1;
                     LUT *drivingLut;
                     do {
                         mergeInput = -1;
                         for(int i = 0; i < inputPorts.size(); i++) {
                            LogicDevice *drivingDev = inputPorts[i]->connectedNet->GetDriver();
                            if(drivingDev != nullptr) { //port is being driven by a LogicDevice
                                drivingLut = dynamic_cast<LUT*>(drivingDev);
                                if(drivingLut != nullptr) { //port is being driven by a LUT
                                    int maxSize = topLevel->technology->GetLUTInputCount();
                                    if(maxSizeOverride != -1) {
                                        if(maxSizeOverride < maxSize) {
                                            maxSize = maxSizeOverride;
                                        }
                                    }
                                    if((drivingLut->inputPorts.size() + inputPorts.size() - 1) <= maxSize) { //does merge fit?
                                        mergeInput = i;
                                        break;
                                    }
                                }
                            }

                         }
                         if(mergeInput != -1) {
                             deviceOptimised = true;
                             MergeWith(drivingLut, mergeInput);
                             PrintMessage(MSG_DEBUG, "merging LUT ===" + drivingLut->name + "=== into ===" + name + "===");

                         }
                     } while(mergeInput != -1);
                 }


             }

             return deviceOptimised;
         }

         string LUT::GenerateSOP(const vector<string>& pinNames) {
             stringstream sop;
             bool firstTerm = true;
             for(int i = 0; i < lutContent.size(); i++) {
                 if(lutContent[i]) {
                     if(!firstTerm) {
                         sop << " or ";
                     } else {
                         firstTerm = false;
                     }
                     sop << "(";
                     for(int j = 0; j < inputPorts.size(); j++) {
                         if(j > 0) {
                             sop << " and ";
                         }
                         if((i & (1 << j)) != 0) {
                             sop << pinNames[j];
                         } else {
                             sop << "(not " << pinNames[j] + ")";
                         }
                     }
                     sop << ")";
                 }
             }
             if(firstTerm) { //never high
                  sop << "'0'";
             }
             return sop.str();
         }

         bool LUT::IsEquivalentTo(LogicDevice* other) {
             LUT *lut = dynamic_cast<LUT*>(other);
             if(lut != nullptr) {
                 if(lut->inputPorts.size() != inputPorts.size()) return false;
                 //At some point this should be improved to consider the case where pins are the
                 //same but in a different order
                 for(int i = 0; i < inputPorts.size(); i++) {
                     if(inputPorts[i]->connectedNet != lut->inputPorts[i]->connectedNet)
                        return false;
                 }

                 for(int i = 0; i < lutContent.size(); i++) {
                     if(lutContent[i] != lut->lutContent[i])
                        return false;
                 }

                 return true;
             } else {
                 return false;
             }
         }
         int LUT::lutCount;


         FlipFlop::FlipFlop() {
             name = "ff_" + to_string(ffCount);
             ffCount++;
         }

         FlipFlop::FlipFlop(Signal *D, Signal *CLK, Signal *Q, Signal *enable, Signal *reset)  {
             name = "ff_" + to_string(ffCount);
             ffCount++;
             DeviceInputPort *dp = new DeviceInputPort();
             dp->device = this;
             dp->pin = 0;
             dp->connectedNet = D;
             D->connectedPorts.push_back(dp);
             inputPorts.push_back(dp);

             DeviceInputPort *cp = new DeviceInputPort();
             cp->device = this;
             cp->pin = 1;
             cp->connectedNet = CLK;
             CLK->connectedPorts.push_back(cp);
             inputPorts.push_back(cp);

             int resetPin = 2;

             if(enable != nullptr) {
                 hasEnable = true;
                 resetPin++;
                 DeviceInputPort *ep = new DeviceInputPort();
                 ep->device = this;
                 ep->pin = 2;
                 ep->connectedNet = enable;
                 enable->connectedPorts.push_back(ep);
                 inputPorts.push_back(ep);
             } else {
                 hasEnable = false;
             }

             if(reset != nullptr) {
                 hasReset = true;
                 DeviceInputPort *rp = new DeviceInputPort();
                 rp->device = this;
                 rp->pin = resetPin;
                 rp->connectedNet = reset;
                 reset->connectedPorts.push_back(rp);
                 inputPorts.push_back(rp);
             } else {
                 hasReset = false;
             }

             DeviceOutputPort *qp = new DeviceOutputPort();
             qp->device = this;
             qp->pin = 0;
             qp->connectedNet = Q;
             Q->connectedPorts.push_back(qp);
             outputPorts.push_back(qp);
         }

         bool FlipFlop::IsEquivalentTo(LogicDevice* other) {
             FlipFlop *ff = dynamic_cast<FlipFlop*>(other);
             if(ff != nullptr) {
                if((hasReset != ff->hasReset) || (hasEnable != ff->hasEnable))
                    return false;

                for(int i = 0; i < inputPorts.size(); i++) {
                    if(inputPorts[i]->connectedNet != ff->inputPorts[i]->connectedNet)
                       return false;
                }

                return true;
             } else {
                 return false;
             }
         }

         ConstantDevice::ConstantDevice() {

         }

         ConstantDevice::ConstantDevice(Signal *sig, bool _value) {
             DeviceOutputPort *cp = new DeviceOutputPort();
             cp->device = this;
             cp->pin = 0;
             cp->connectedNet = sig;
             sig->connectedPorts.push_back(cp);
             outputPorts.push_back(cp);
             value = _value;
         }

         int FlipFlop::ffCount;

    }
}
