#include "AlteraDevices.hpp"
#include "../LogicDesign.hpp"
#include "../Util.hpp"
using namespace std;
namespace SynthFramework {
  namespace Polymer {
      Altera_CarrySum::Altera_CarrySum() {
          name = "carrysum_" + to_string(cscount);
          cscount++;
      }

      Altera_CarrySum::Altera_CarrySum(Signal *sin, Signal *cin, Signal *sout, Signal *cout) {
          name = "carrysum_" + to_string(cscount);
          cscount++;

          DeviceInputPort *sip = new DeviceInputPort();
          sip->device = this;
          sip->pin = 0;
          sip->connectedNet = sin;
          sin->connectedPorts.push_back(sip);
          inputPorts.push_back(sip);

          DeviceInputPort *cip = new DeviceInputPort();
          cip->device = this;
          cip->pin = 1;
          cip->connectedNet = cin;
          cin->connectedPorts.push_back(cip);
          inputPorts.push_back(cip);

          DeviceOutputPort *sop = new DeviceOutputPort();
          sop->device = this;
          sop->pin = 0;
          sop->connectedNet = sout;
          sout->connectedPorts.push_back(sop);
          outputPorts.push_back(sop);

          DeviceOutputPort *cop = new DeviceOutputPort();
          cop->device = this;
          cop->pin = 1;
          cop->connectedNet = cout;
          cout->connectedPorts.push_back(cop);
          outputPorts.push_back(cop);

    }

    bool Altera_CarrySum::OptimiseDevice(LogicDesign *topLevel) {
        bool val0, val1;
        bool optimised = false;
        if(inputPorts[0]->connectedNet->GetConstantValue(val0) && inputPorts[1]->connectedNet->GetConstantValue(val1)) {
            outputPorts[0]->Disconnect();
            PrintMessage(MSG_DEBUG, "net ===" + outputPorts[0]->connectedNet->name + "=== is stuck at " + to_string(val0));
            if(val0) {
                outputPorts[0]->connectedNet->ConnectTo(topLevel->vcc);
            } else {
                outputPorts[0]->connectedNet->ConnectTo(topLevel->gnd);
            }

            outputPorts[1]->Disconnect();
            PrintMessage(MSG_DEBUG, "net ===" + outputPorts[1]->connectedNet->name + "=== is stuck at " + to_string(val1));
            if(val1) {
                outputPorts[1]->connectedNet->ConnectTo(topLevel->vcc);
            } else {
                outputPorts[1]->connectedNet->ConnectTo(topLevel->gnd);
            }
            optimised = true;
        }
        return optimised;
    }

    int Altera_CarrySum::cscount;

    Altera_ROM::Altera_ROM() {
        name = "ROM1_" + to_string(romcount);
        romcount++;
    }
    Altera_ROM::Altera_ROM(int _width, int _length, string _filename, Signal *clock, vector<Signal*> address, vector<Signal*> data) {
        name = "ROM1_" + to_string(romcount);
        romcount++;
        width = _width;
        length = _length;
        filename = _filename;
        DeviceInputPort *cip = new DeviceInputPort();
        cip->device = this;
        cip->pin = 0;
        cip->connectedNet = clock;
        clock->connectedPorts.push_back(cip);
        inputPorts.push_back(cip);

        for(int i = 0; i < address.size(); i++) {
            DeviceInputPort *aip = new DeviceInputPort();
            aip->device = this;
            aip->pin = i+1;
            aip->connectedNet = address[i];
            address[i]->connectedPorts.push_back(aip);
            inputPorts.push_back(aip);
        }

        for(int i = 0; i < data.size(); i++) {
            DeviceOutputPort *dop = new DeviceOutputPort();
            dop->device = this;
            dop->pin = i;
            dop->connectedNet = data[i];
            data[i]->connectedPorts.push_back(dop);
            outputPorts.push_back(dop);
        }
    }

    int Altera_ROM::romcount;

    Altera_Multiplier18::Altera_Multiplier18() {
        name = "MUL18_" + to_string(mulcount);
        mulcount++;
    }
    Altera_Multiplier18::Altera_Multiplier18(vector<Signal*> a, vector<Signal*> b, vector<Signal*> out, bool _sign_a, bool _sign_b) {
        name = "MUL18_" + to_string(mulcount);
        mulcount++;
        for(int i = 0; i < 18; i++) {
            DeviceInputPort *aip = new DeviceInputPort();
            aip->device = this;
            aip->pin = i;
            aip->connectedNet = a[i];
            a[i]->connectedPorts.push_back(aip);
            inputPorts.push_back(aip);
        }
        for(int i = 0; i < 18; i++) {
            DeviceInputPort *bip = new DeviceInputPort();
            bip->device = this;
            bip->pin = i + 18;
            bip->connectedNet = b[i];
            b[i]->connectedPorts.push_back(bip);
            inputPorts.push_back(bip);
        }
        for(int i = 0; i < 36; i++) {
            DeviceOutputPort *op = new DeviceOutputPort();
            op->device = this;
            op->pin = i;
            op->connectedNet = out[i];
            out[i]->connectedPorts.push_back(op);
            outputPorts.push_back(op);
        }
        sign_a = _sign_a;
        sign_b = _sign_b;
    }

    bool Altera_Multiplier18::OptimiseDevice(LogicDesign *topLevel) {
        //Detect how many bits are duplicates of the sign bit
        bool ls;
        int actualSizeA = 18, actualSizeB = 18;
        if(sign_a) {
            for(int i = 16; i >= 0; i--) {
                if(inputPorts[i]->connectedNet != inputPorts[17]->connectedNet) {
                    actualSizeA = i + 2;
                    break;
                }
            }
        } else {
            for(int i = 17; i >= 0; i--) {
                if((!inputPorts[i]->connectedNet->GetConstantValue(ls)) || (!ls)) {
                    actualSizeA = i + 1;
                    break;
                }
            }
        }
        if(sign_b) {
            for(int i = 16; i >= 0; i--) {
                if(inputPorts[i+18]->connectedNet != inputPorts[35]->connectedNet) {
                    actualSizeB = i + 2;
                    break;
                }
            }
        } else {
            for(int i = 17; i >= 0; i--) {
                if((!inputPorts[i+18]->connectedNet->GetConstantValue(ls)) || (!ls)) {
                    actualSizeB = i + 1;
                    break;
                }
            }
        }
        int newActualSize = actualSizeA + actualSizeB;
        if(newActualSize < actualsize) {
            for(int i = newActualSize; i < actualsize; i++) {
                Signal *dummy = topLevel->CreateSignal(name + "_do_" + to_string(i));
                outputPorts[i]->Disconnect();
                outputPorts[i]->connectedNet = dummy;
                dummy->connectedPorts.push_back(outputPorts[i]);
            }
            actualsize = newActualSize;
            PrintMessage(MSG_DEBUG, "multiplier " + name + " has an actual output size of " + to_string(newActualSize));
            return true;
        } else {
            return false;
        }
    }
    int Altera_Multiplier18::mulcount;
  }
}
