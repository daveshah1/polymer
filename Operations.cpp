#include "Operations.hpp"
#include "DeviceTechnology.hpp"
#include "LogicDesign.hpp"
#include "BasicDevices.hpp"
#include "Util.hpp"
using namespace std;

namespace SynthFramework {
    namespace Polymer {
        Operation::Operation() {

        }

        Operation::Operation(OperationType _type, vector<Bus*> ins, Bus *out) :
            type(_type), inputs(ins), output(out) {

        }


        void Operation::Synthesise(LogicDesign* topLevel) {
            if(!topLevel->technology->DeviceSpecificSynthesis(this, topLevel)) {
                switch(type) {
                case OPER_B_BWOR:
                    GenerateBitwiseLUTs(Device_OR2, topLevel);
                    break;
                case OPER_B_BWAND:
                    GenerateBitwiseLUTs(Device_AND2, topLevel);
                    break;
                case OPER_B_BWXOR:
                    GenerateBitwiseLUTs(Device_XOR2, topLevel);
                    break;
                case OPER_U_BWNOT:
                    GenerateBitwiseLUTs(Device_NOT, topLevel);
                    break;
                case OPER_B_ADD:
                    GenerateAdderLUTs(false, topLevel);
                    break;
                case OPER_T_ADD_CIN:
                    GenerateAdderLUTs(false, topLevel, GetInputSignal(2, 0, topLevel));
                    break;
                case OPER_B_SUB:
                    GenerateAdderLUTs(true, topLevel);
                    break;
                case OPER_B_LOR:
                    GenerateLogicalLUTs(Device_OR2, topLevel);
                    break;
                case OPER_B_LAND:
                    GenerateLogicalLUTs(Device_AND2, topLevel);
                    break;
                case OPER_U_LNOT:
                    GenerateLogicalLUTs(Device_NOT, topLevel);
                    break;
                case OPER_B_LT:
                    GenerateComparisonLUTs(true, false, topLevel);
                    break;
                case OPER_B_GT:
                    GenerateComparisonLUTs(false, false, topLevel);
                    break;
                case OPER_B_LTE:
                    GenerateComparisonLUTs(true, true, topLevel);
                    break;
                case OPER_B_GTE:
                    GenerateComparisonLUTs(false, true, topLevel);
                    break;
                case OPER_B_EQ:
                    GenerateEqualityLUTs(false, topLevel);
                    break;
               case OPER_B_NEQ:
                    GenerateEqualityLUTs(true, topLevel);
                    break;
                case OPER_T_COND:
                    GenerateConditionalLUTs(topLevel);
                    break;
                case OPER_CONNECT:
                    for(int j = 0; j < output->width; j++) {
                        output->signals[j]->ConnectTo(GetInputSignal(0, j, topLevel));
                    }
                    break;
                case OPER_B_LS: {
                    long long constVal = 0;
                    if(inputs[1]->GetConstantValue(constVal)) {
                        GenerateFixedShiftLUTs(constVal, false, topLevel);
                    } else {
                        GenerateBarrelShiftLUTs(false, topLevel);
                    }
                    } break;
                case OPER_B_RS: {
                    long long constVal = 0;
                    if(inputs[1]->GetConstantValue(constVal)) {
                        GenerateFixedShiftLUTs(constVal, true, topLevel);
                    } else {
                        GenerateBarrelShiftLUTs(true, topLevel);
                    }
                    } break;
                case OPER_B_MUL: {
                    long long constVal = 0;
                    if(inputs[0]->GetConstantValue(constVal) || inputs[1]->GetConstantValue(constVal) ) {
                        GenerateMultiplyByConstLUTs(topLevel);
                    } else {
                        GenerateMultiplyLUTs(topLevel);
                    }
                    } break;
                case OPER_U_REG:
                    for(int j = 0; j < output->width; j++) {
                        topLevel->devices.push_back(new FlipFlop(GetInputSignal(0, j, topLevel), topLevel->clockSignal, output->signals[j]));
                    }
                    break;
                case OPER_U_SREG:
                    for(int j = 0; j < output->width; j++) {
                        FlipFlop *ff = new FlipFlop(GetInputSignal(0, j, topLevel), topLevel->clockSignal, output->signals[j]);
                        ff->isShift = true;
                        topLevel->devices.push_back(ff);
                    }
                    break;
                case OPER_U_SEREG:
                    for(int j = 0; j < output->width; j++) {
                        FlipFlop *ff = new FlipFlop(GetInputSignal(0, j, topLevel), topLevel->clockSignal, output->signals[j], GetInputSignal(1, 0, topLevel));
                        ff->isShift = true;
                        topLevel->devices.push_back(ff);
                    }
                    break;
                case OPER_B_DIV:
                    GenerateDivisionLUTs(topLevel);
                    break;
                default:
                    PrintMessage(MSG_ERROR, "Unsupported operation type " + OperationInfo[type].name);
                    break;
                }
            }

        };

        void Operation::GenerateBitwiseLUTs(LUTDeviceType lutType, LogicDesign* topLevel) {
            int busSize = GetMaxInputSize();
            for(int j = 0; j < busSize; j++) {
                Signal *outPin = GetOutputSignal(j, topLevel);
                if(outPin != nullptr) {
                    vector<Signal*> inPins;
                    for(int i = 0; i < inputs.size(); i++) {
                        inPins.push_back(GetInputSignal(i, j, topLevel));
                    }
                    topLevel->devices.push_back(new LUT(lutType, inPins, outPin));
                }
            }
        };
        void Operation::GenerateEqualityLUTs(bool inv, LogicDesign* topLevel) {
          int inputSize = max(inputs[0]->width, inputs[1]->width);
          Bus *compareOuts = new Bus();
          compareOuts->name = name + "_comp";
          compareOuts->is_signed = false;
          compareOuts->width = inputSize;
          topLevel->AddBus(compareOuts);
          for(int j = 0; j < inputSize; j++) {
            vector<Signal*> inPins;
            for(int i = 0; i < inputs.size(); i++) {
              inPins.push_back(GetInputSignal(i, j, topLevel));
            }
            if(inv) {
              topLevel->devices.push_back(new LUT(Device_XOR2, inPins, compareOuts->signals[j]));
            } else {
              topLevel->devices.push_back(new LUT(Device_XNOR2, inPins, compareOuts->signals[j]));
            }
          }
          Signal *result = ConvertToBoolean(compareOuts, topLevel, Device_AND2);
          if(output->width > 0) {
            output->signals[0]->ConnectTo(result);
            for(int j = 1; j < output->width; j++) {
              output->signals[j]->ConnectTo(topLevel->gnd);
            }
          }
        }

        void Operation::GenerateAdderLUTs(bool isSubtract, LogicDesign* topLevel, Signal *cin) {
            int busSize = GetMaxInputSize() + 1;

            //Carry chain signal: this is initialised with the CIN value
            Signal *carryChain;
            if(cin != nullptr) {
                carryChain = cin;
            } else {
                if(isSubtract) {
                    carryChain = topLevel->vcc;
                } else {
                    carryChain = topLevel->gnd;
                }
            }


            for(int j = 0; j < busSize; j++) {
                Signal *outPin = GetOutputSignal(j, topLevel);
                if(outPin != nullptr) {
                    vector<Signal*> inPins;
                    for(int i = 0; i < 2; i++) {
                        Signal *inPin = GetInputSignal(i, j, topLevel);
                        if((i == 1) && isSubtract) { //need to invert B input for a subtractor
                            Signal *invPin = topLevel->CreateSignal(name + "_invB_" + to_string(j));
                            topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{inPin}, invPin));
                            inPins.push_back(invPin);
                        } else {
                            inPins.push_back(inPin);
                        }
                    }
                    inPins.push_back(carryChain); //carry in
                    Signal *carryOut = topLevel->CreateSignal(name + "_CO_" + to_string(j));
                    topLevel->devices.push_back(new LUT(Device_FULLADD_SUM, inPins, outPin));
                    topLevel->devices.push_back(new LUT(Device_FULLADD_CARRY, inPins, carryOut));
                    carryChain = carryOut;
                }
            }
        };

        int Operation::GetMaxInputSize() {
            int maxSize = 0;
            for(auto in : inputs) {
                if(in->width > maxSize) {
                    maxSize = in->width;
                }
            }
            return maxSize;
        };

        Signal *Operation::GetInputSignal(int i, int j, LogicDesign* topLevel) {
            if(j < inputs[i]->width) {
                return inputs[i]->signals[j];
            } else {
                if(inputs[i]->is_signed) {
                    return inputs[i]->signals[inputs[i]->width - 1]; //sign extend
                } else {
                    return topLevel->gnd; //zero extend
                }
            }
        };

        Signal *Operation::GetOutputSignal(int j, LogicDesign* topLevel) {
            if(j < output->width) {
                return output->signals[j];
            } else {
                return nullptr;
            }
        };

        Signal *Operation::ConvertToBoolean(Bus *bus, LogicDesign* topLevel, LUTDeviceType opType) {
            vector<Signal*> currentSet = bus->signals;
            int treeLevel = 0;
            while(currentSet.size() > 1) {
                vector<Signal*> nextLevel;
                for(int i = 0; i < currentSet.size(); i+=2) {
                    if((i+1) >= currentSet.size()) { //signal is 'spare' due to an odd bus size, pass straight through
                        nextLevel.push_back(currentSet[i]);
                    } else {
                        Signal *or_outp = topLevel->CreateSignal(name + "_" + bus->name + "_OP" + to_string(treeLevel) + "_" + to_string(i/2));
                        topLevel->devices.push_back(new LUT(opType, vector<Signal*>{currentSet[i], currentSet[i+1]}, or_outp));
                        nextLevel.push_back(or_outp);
                    }
                }
                treeLevel++;
                currentSet = nextLevel;
            }
            return currentSet[0];
        }

        void Operation::GenerateLogicalLUTs(LUTDeviceType lutType, LogicDesign* topLevel) {
            vector<Signal*> inputBools;
            for(int i = 0; i < inputs.size(); i++) { //convert all operands to booleans
                inputBools.push_back(ConvertToBoolean(inputs[i], topLevel));
            }
            if(output->width > 0) { //output bit 0 is result of logical operation
                topLevel->devices.push_back(new LUT(lutType, inputBools, output->signals[0]));
            }
            for(int j = 1; j < output->width; j++) { //output bits 1..n-1 are 0
                output->signals[j]->ConnectTo(topLevel->gnd);
            }
        }

        void Operation::GenerateConditionalLUTs(LogicDesign* topLevel) {
            Signal *condition = ConvertToBoolean(inputs[0], topLevel);
            for(int j = 0; j < output->width; j++) {
                vector<Signal*> inputSignals;
                inputSignals.push_back(GetInputSignal(2, j, topLevel)); //input 0 is mux input A, i.e. if false
                inputSignals.push_back(GetInputSignal(1, j, topLevel)); //input 1 is mux input B, i.e. if true
                inputSignals.push_back(condition); //input 2 is mux input S, i.e. condition
                topLevel->devices.push_back(new LUT(Device_MUX2_1, inputSignals, output->signals[j]));
            }
        }

        void Operation::GenerateFixedShiftLUTs(int amount, bool isRightShift, LogicDesign* topLevel) {
            for(int j = 0; j < output->width; j++) {
                int inputIndex = j;
                if(isRightShift) {
                    inputIndex += amount;
                } else {
                    inputIndex -= amount;
                }
                if(inputIndex >= 0) {
                    output->signals[j]->ConnectTo(GetInputSignal(0, inputIndex, topLevel));
                } else {
                    output->signals[j]->ConnectTo(topLevel->gnd);
                }
            }
        }

        void Operation::GenerateBarrelShiftLUTs(bool isRightShift, LogicDesign* topLevel) {
            vector<Signal*> intSignals;
            intSignals = inputs[0]->signals;
            int shiftWidth = inputs[1]->width;
            for(int j = shiftWidth - 1; j >= 0; j--) {
                vector<Signal*> muxOutputs;
                for(int k = 0; k < inputs[0]->width; k++) {
                    vector<Signal*> muxIn;
                    Signal *muxOut = topLevel->CreateSignal(name + "_shifto_" + to_string(j) + "_" + to_string(k));
                    muxIn.push_back(intSignals[k]);
                    int shiftK = k;
                    if(isRightShift) {
                        shiftK += (1 << j);
                    } else {
                        shiftK -= (1 << j);
                    }

                    if(shiftK < 0) {
                        muxIn.push_back(topLevel->gnd);
                    } else if(shiftK >= intSignals.size()) {
                        if(inputs[0]->is_signed) {
                            muxIn.push_back(intSignals.back());
                        } else {
                            muxIn.push_back(topLevel->gnd);
                        }
                    } else {
                        muxIn.push_back(intSignals[shiftK]);
                    }

                    topLevel->devices.push_back(new LUT(Device_MUX2_1, muxIn, muxOut));

                    muxOutputs.push_back(muxOut);
                }
                intSignals = muxOutputs;
            }

            for(int j = 0; j < output->width; j++) {
                if(j < intSignals.size()) {
                    output->signals[j]->ConnectTo(intSignals[j]);
                } else {
                    if(output->is_signed) {
                        output->signals[j]->ConnectTo(intSignals.back());
                    } else {
                        output->signals[j]->ConnectTo(topLevel->gnd);
                    }
                }
            }
        }

        void Operation::GenerateComparisonLUTs(bool lt, bool eq, LogicDesign* topLevel) {
            //This method seems inefficient and I'm sure can be improved upon
            int in1 = 0, in2 = 1;
            if(lt) {
                in1 = 1;
                in2 = 0;
            }
            bool is_signed = inputs[0]->is_signed || inputs[1]->is_signed;
            int inputSize = max(inputs[0]->width, inputs[1]->width);
            Signal *eqChain = topLevel->vcc;
            Bus *compareOuts = new Bus();
            compareOuts->name = name + "_comp";
            compareOuts->is_signed = false;
            compareOuts->width = inputSize;
            topLevel->AddBus(compareOuts);
            for(int j = inputSize - 1; j >= 0; j--) {
                //Greater than at this point (lt swaps inputs): invert B input
                Signal *inv_b = topLevel->CreateSignal(name + "_invb_" + to_string(j));
                //2's complement compare requires MSB comparison to be reversed
                if(is_signed && (j == (inputSize - 1))) {
                    topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{GetInputSignal(in1, j, topLevel)}, inv_b));
                    topLevel->devices.push_back(new LUT(Device_AND3, vector<Signal*>{GetInputSignal(in2, j, topLevel), inv_b, eqChain}, compareOuts->signals[j]));
                } else {
                    topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{GetInputSignal(in2, j, topLevel)}, inv_b));
                    topLevel->devices.push_back(new LUT(Device_AND3, vector<Signal*>{GetInputSignal(in1, j, topLevel), inv_b, eqChain}, compareOuts->signals[j]));
                }

                Signal *xnoro = topLevel->CreateSignal(name + "_xnor_" + to_string(j));
                topLevel->devices.push_back(new LUT(Device_XNOR2, vector<Signal*>{GetInputSignal(0, j, topLevel), GetInputSignal(1, j, topLevel)}, xnoro));
                Signal *eqOut = topLevel->CreateSignal(name + "_eq_" + to_string(j));
                topLevel->devices.push_back(new LUT(Device_AND2, vector<Signal*>{eqChain, xnoro}, eqOut));
                eqChain = eqOut;
            }
            if(output->width > 0) { //output bit 0 is boolean conversion compareOuts
                Signal *compareResult = ConvertToBoolean(compareOuts, topLevel);
                Signal *result;
                if(eq) {
                    result = topLevel->CreateSignal(name + "_res");
                    topLevel->devices.push_back(new LUT(Device_OR2, vector<Signal*>{compareResult, eqChain}, result));
                } else {
                    result = compareResult;
                }
                output->signals[0]->ConnectTo(result);
            }
            for(int j = 1; j < output->width; j++) { //output bits 1..n-1 are 0
                output->signals[j]->ConnectTo(topLevel->gnd);
            }
        }

        void Operation::GenerateMultiplyByConstLUTs(LogicDesign* topLevel) {
            int ina, inb;
            long long constval;
            if(inputs[1]->GetConstantValue(constval)) {
                ina = 0;
                inb = 1;
            } else if(inputs[0]->GetConstantValue(constval)) {
                ina = 1;
                inb = 0;
            }
            bool inv = false;
            if(constval < 0) {
                inv = true;
                constval = -constval;
            }
            Bus *tempBus = nullptr;

            for(int i = 0; i < 63; i++) {
                //no more bits left, stop
                if((constval >> i) == 0) break;
                //if bit present in constval, need to add appropriately shifted input to result
                if(((constval >> i) & 0x1) != 0) {
                    Bus *shifted;
                    if(i > 0) {
                        shifted = new Bus();
                        shifted->name = name + "_SHIFTED" + to_string(i);
                        shifted->is_signed = inputs[ina]->is_signed;
                        shifted->width = output->width;
                        topLevel->AddBus(shifted);
                        Bus *amt = topLevel->CreateConstantBus(i);
                        Operation *ls = new Operation(OPER_B_LS, vector<Bus*>{inputs[ina], amt}, shifted);
                        ls->name = name + "_SHIFT" + to_string(i);
                        ls->Synthesise(topLevel);

                    } else {
                        shifted = inputs[ina];
                    }

                    if(tempBus == nullptr) {
                        if(inv) {
                            Bus *neg = new Bus();
                            neg->name = name + "_NEG" + to_string(i);
                            neg->is_signed = inputs[ina]->is_signed;
                            neg->width = output->width;
                            topLevel->AddBus(neg);
                            Operation *neg_op;
                            neg_op = new Operation(OPER_B_SUB, vector<Bus*>{topLevel->CreateConstantBus(0), shifted}, neg);
                            neg_op->name = name + "_NEG" + to_string(i);
                            neg_op->Synthesise(topLevel);
                            tempBus = neg;
                        } else {
                            tempBus = shifted;
                        }
                    } else {
                        Bus *added = new Bus();
                        added->name = name + "_ADDED" + to_string(i);
                        added->is_signed = inputs[ina]->is_signed;
                        added->width = output->width;
                        topLevel->AddBus(added);
                        Operation *add;
                        if(inv) {
                            add = new Operation(OPER_B_SUB, vector<Bus*>{tempBus, shifted}, added);
                        } else {
                            add = new Operation(OPER_B_ADD, vector<Bus*>{tempBus, shifted}, added);
                        }
                        add->name = name + "_ADDED" + to_string(i);
                        add->Synthesise(topLevel);
                        tempBus = added;
                   }
                }
            }

            if(tempBus == nullptr) {
                for(int i = 0; i < output->width; i++) {
                    output->signals[i]->ConnectTo(topLevel->gnd);
                }
            } else {
                for(int i = 0; i < output->width; i++) {
                    if(i < tempBus->width) {
                        output->signals[i]->ConnectTo(tempBus->signals[i]);
                    } else {
                        if(output->is_signed) {
                            output->signals[i]->ConnectTo(tempBus->signals.back());
                        } else {
                            output->signals[i]->ConnectTo(topLevel->gnd);
                        }
                    }
                }
            }
        }

        void Operation::GenerateMultiplyLUTs(LogicDesign* topLevel) {
            int ina, inb;
            //we want inb to be the smaller of the two inputs
            if(inputs[1]->width <= inputs[0]->width) {
                ina = 0;
                inb = 1;
            } else {
                ina = 1;
                inb = 0;
            }

            Bus *tempBus = nullptr;

            for(int i = 0; i < inputs[inb]->width; i++) {

                Bus *shifted;
                if(i > 0) {
                    shifted = new Bus();
                    shifted->name = name + "_SHIFTED" + to_string(i);
                    shifted->is_signed = inputs[ina]->is_signed;
                    shifted->width = output->width;
                    topLevel->AddBus(shifted);
                    Bus *amt = topLevel->CreateConstantBus(i);
                    Operation *ls = new Operation(OPER_B_LS, vector<Bus*>{inputs[ina], amt}, shifted);
                    ls->name = name + "_SHIFT" + to_string(i);
                    ls->Synthesise(topLevel);

                } else {
                    shifted = inputs[ina];
                }

                Bus *anded = new Bus();
                anded->name = name + "_ANDED" + to_string(i);
                anded->is_signed = inputs[ina]->is_signed;
                anded->width = output->width;
                topLevel->AddBus(anded);
                for(int j = 0; j < anded->width; j++) {
                    Signal *bitin;
                    if(j >= shifted->width) {
                        if(shifted->is_signed) {
                            bitin = shifted->signals.back();
                        } else {
                            bitin = topLevel->gnd;
                        }
                    } else {
                        bitin = shifted->signals[j];
                    }
                    topLevel->devices.push_back(new LUT(Device_AND2, vector<Signal*>{bitin, inputs[inb]->signals[i]}, anded->signals[j]));
                }


                if(tempBus == nullptr) {
                    tempBus = anded;
                } else {
                    Bus *added = new Bus();
                    added->name = name + "_ADDED" + to_string(i);
                    added->is_signed = inputs[ina]->is_signed;
                    added->width = output->width;
                    topLevel->AddBus(added);
                    Operation *add;
                    if((inputs[inb]->is_signed) && (i == (inputs[inb]->width - 1))) {
                        add = new Operation(OPER_B_SUB, vector<Bus*>{tempBus, anded}, added);
                    } else {
                        add = new Operation(OPER_B_ADD, vector<Bus*>{tempBus, anded}, added);
                    }
                    add->name = name + "_ADDED" + to_string(i);
                    add->Synthesise(topLevel);
                    tempBus = added;
               }

            }

            if(tempBus == nullptr) {
                for(int i = 0; i < output->width; i++) {
                    output->signals[i]->ConnectTo(topLevel->gnd);
                }
            } else {
                for(int i = 0; i < output->width; i++) {
                    if(i < tempBus->width) {
                        output->signals[i]->ConnectTo(tempBus->signals[i]);
                    } else {
                        if(output->is_signed) {
                            output->signals[i]->ConnectTo(tempBus->signals.back());
                        } else {
                            output->signals[i]->ConnectTo(topLevel->gnd);
                        }
                    }
                }
            }
        }


        void Operation::GenerateDivisionLUTs(LogicDesign* topLevel) {
            int n = GetMaxInputSize();
            Bus *A, *Q, *M, *Mn;

            A = new Bus();
            A->name = name + "_Ai";
            A->width = n;
            A->is_signed = true;
            topLevel->AddBus(A);
            for(int j = 0; j < n; j++) {
                A->signals[j]->ConnectTo(inputs[0]->signals.back());
            }

            M = new Bus();
            M->name = name + "_M";
            M->width = n;
            M->is_signed = inputs[1]->is_signed;
            topLevel->AddBus(M);
            for(int j = 0; j < n; j++) {
                M->signals[j]->ConnectTo(GetInputSignal(1, j, topLevel));
            }

            /*Mn = new Bus();
            Mn->name = name + "_Mn";
            Mn->width = n;
            Mn->is_signed = M->is_signed;
            topLevel->AddBus(Mn);

            Operation *inv = new Operation(OPER_B_SUB, vector<Bus*>{topLevel->CreateConstantBus(0),M},Mn);
            inv->name = name + "_Minv";
            inv->Synthesise(topLevel);*/

            Q = new Bus();
            Q->name = name + "_Q";
            Q->width = n;
            Q->is_signed = inputs[0]->is_signed;
            topLevel->AddBus(Q);
            for(int j = 0; j < n; j++) {
                Q->signals[j]->ConnectTo(GetInputSignal(0, j, topLevel));
            }

            for(int i = n; i > 0; i--) {
                Signal *sign1 = A->signals.back();

                if(inputs[1]->is_signed) {
                    Signal *xorres = topLevel->CreateSignal(name + "_xor1_" + to_string(i));
                    topLevel->devices.push_back(new LUT(Device_XOR2, vector<Signal*>{sign1, inputs[1]->signals.back()}, xorres));
                    sign1 = xorres;
                }
                Bus *sign1n = new Bus();
                sign1n->name = name + "_Sinv" + to_string(i);
                sign1n->width = 1;
                sign1n->is_signed = false;
                topLevel->AddBus(sign1n);
                topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{sign1}, sign1n->signals[0]));
                Bus *ashift = new Bus();
                ashift->name = name + "_As" + to_string(i);
                ashift->width = A->width;
                ashift->is_signed = A->is_signed;
                topLevel->AddBus(ashift);

                Bus *qshift = new Bus();
                qshift->name = name + "_Qs" + to_string(i);
                qshift->width = Q->width;
                qshift->is_signed = Q->is_signed;
                topLevel->AddBus(qshift);

                for(int j = 1; j < n; j++) {
                    qshift->signals[j]->ConnectTo(Q->signals[j-1]);
                    ashift->signals[j]->ConnectTo(A->signals[j-1]);
                }
                ashift->signals[0]->ConnectTo(Q->signals[n-1]);

                Bus *addamt = new Bus();
                addamt->name = name + "_Addm" + to_string(i);
                addamt->width = M->width;
                addamt->is_signed = M->is_signed;
                topLevel->AddBus(addamt);
                for(int j = 0; j < addamt->width; j++) {
                    topLevel->devices.push_back(new LUT(Device_XOR2, vector<Signal*>{M->signals[j], sign1n->signals[0]}, addamt->signals[j]));
                }

                Bus *anext = new Bus();
                anext->name = name + "_An" + to_string(i);
                anext->width = A->width;
                anext->is_signed = A->is_signed;
                topLevel->AddBus(anext);

                Operation *adder = new Operation(OPER_T_ADD_CIN, vector<Bus*>{ashift, addamt, sign1n}, anext);
                adder->name = name + "_ADD" + to_string(i);
                adder->Synthesise(topLevel);

                Signal *sign2 = anext->signals.back();
                if(inputs[1]->is_signed) {
                    Signal *xorres2 = topLevel->CreateSignal(name + "_xor2_" + to_string(i));
                    topLevel->devices.push_back(new LUT(Device_XOR2, vector<Signal*>{sign2, inputs[1]->signals.back()}, xorres2));
                    sign2 = xorres2;
                }

                topLevel->devices.push_back(new LUT(Device_NOT, vector<Signal*>{sign2}, qshift->signals[0]));

                Q = qshift;
                A = anext;
            }

            for(int i = 0; i < output->width; i++) {
                if(i < Q->width) {
                    output->signals[i]->ConnectTo(Q->signals[i]);
                } else {
                    if(output->is_signed) {
                        output->signals[i]->ConnectTo(Q->signals.back());
                    } else {
                        output->signals[i]->ConnectTo(topLevel->gnd);
                    }
                }
            }
        }


        bool GetOperationByName(string name, OperationType &type) {
            for(auto oper : OperationInfo) {
                if(oper.second.name == name) {
                    type = oper.first;
                    return true;
                }
            }
            return false;
        }

        map<OperationType, OperationTypeInfo> OperationInfo = {
            {OPER_B_ADD, {"ADD", 2}},
            {OPER_T_ADD_CIN, {"ADD_CIN", 3}},
            {OPER_B_SUB, {"SUB", 2}},
            {OPER_B_MUL, {"MUL", 2}},
            {OPER_B_DIV, {"DIV", 2}},
            {OPER_B_MOD, {"MOD", 2}},
            {OPER_B_BWOR, {"BWOR", 2}},
            {OPER_B_BWAND, {"BWAND", 2}},
            {OPER_B_BWXOR, {"BWXOR", 2}},
            {OPER_B_LOR, {"LOR", 2}},
            {OPER_B_LAND, {"LAND", 2}},
            {OPER_B_EQ, {"EQ", 2}},
            {OPER_B_NEQ, {"NEQ", 2}},
            {OPER_B_LT, {"LT", 2}},
            {OPER_B_LTE, {"LTE", 2}},
            {OPER_B_GT, {"GT", 2}},
            {OPER_B_GTE, {"GTE", 2}},
            {OPER_B_LS, {"LS", 2}},
            {OPER_B_RS, {"RS", 2}},
            {OPER_U_BWNOT, {"BWNOT", 1}},
            {OPER_U_LNOT, {"LNOT", 1}},
            {OPER_U_MINUS, {"MINUS", 1}},
            {OPER_U_REG, {"REG", 1}},
            {OPER_U_SREG, {"SREG", 1}},
            {OPER_U_SEREG, {"SEREG", 2}},
            {OPER_T_COND, {"COND", 3}},
            {OPER_CONNECT, {"WIRE", 1}}
        };
    }
}
