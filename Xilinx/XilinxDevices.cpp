#include "XilinxDevices.hpp"
#include "../LogicDesign.hpp"
using namespace std;
namespace SynthFramework {
  namespace Polymer {
      Xilinx_Carry4::Xilinx_Carry4() {
          name = "carry4_" + to_string(carry4count);
          carry4count++;
      }

      Xilinx_Carry4::Xilinx_Carry4(Signal *ci, Signal *cinit, vector<Signal*> DI, vector<Signal*> S, vector<Signal*> O, vector<Signal*> CO) {
          name = "carry4_" + to_string(carry4count);
          carry4count++;

          DeviceInputPort *cip = new DeviceInputPort();
          cip->device = this;
          cip->pin = 0;
          cip->connectedNet = ci;
          ci->connectedPorts.push_back(cip);
          inputPorts.push_back(cip);

          DeviceInputPort *cinitp = new DeviceInputPort();
          cinitp->device = this;
          cinitp->pin = 1;
          cinitp->connectedNet = cinit;
          cinit->connectedPorts.push_back(cinitp);
          inputPorts.push_back(cinitp);

          for(int i = 0 ; i < 4; i++) {
              DeviceInputPort *dip = new DeviceInputPort();
              dip->device = this;
              dip->pin = 2 + i;
              dip->connectedNet = DI[i];
              DI[i]->connectedPorts.push_back(dip);
              inputPorts.push_back(dip);
          }

          for(int i = 0 ; i < 4; i++) {
              DeviceInputPort *sip = new DeviceInputPort();
              sip->device = this;
              sip->pin = 6 + i;
              sip->connectedNet = S[i];
              S[i]->connectedPorts.push_back(sip);
              inputPorts.push_back(sip);
          }

          for(int i = 0 ; i < 4; i++) {
              DeviceOutputPort *op = new DeviceOutputPort();
              op->device = this;
              op->pin = i;
              op->connectedNet = O[i];
              O[i]->connectedPorts.push_back(op);
              outputPorts.push_back(op);
          }

          for(int i = 0 ; i < 4; i++) {
              DeviceOutputPort *cop = new DeviceOutputPort();
              cop->device = this;
              cop->pin = 4 + i;
              cop->connectedNet = CO[i];
              CO[i]->connectedPorts.push_back(cop);
              outputPorts.push_back(cop);
          }
    }

    bool Xilinx_Carry4::OptimiseDevice(LogicDesign *topLevel) {
        return false;
        /*In the future this should fold constants, deal with outputs with no fanout, etc*/
    }

    int Xilinx_Carry4::carry4count;

    Xilinx_DSP48Mul::Xilinx_DSP48Mul() {
        name = "dsp48_" + to_string(dsp48count);
        dsp48count++;
    }

    Xilinx_DSP48Mul::Xilinx_DSP48Mul(Signal *clock, vector<Signal*> A, vector<Signal*> B, vector<Signal*> M) {
        name = "dsp48_" + to_string(dsp48count);
        dsp48count++;

        DeviceInputPort *clkp = new DeviceInputPort();
        clkp->device = this;
        clkp->pin = 1;
        clkp->connectedNet = clock;
        clock->connectedPorts.push_back(clkp);
        inputPorts.push_back(clkp);

        for(int i = 0 ; i < 25; i++) {
            DeviceInputPort *aip = new DeviceInputPort();
            aip->device = this;
            aip->pin = 1 + i;
            aip->connectedNet = A[i];
            A[i]->connectedPorts.push_back(aip);
            inputPorts.push_back(aip);
        }

        for(int i = 0 ; i < 18; i++) {
            DeviceInputPort *bip = new DeviceInputPort();
            bip->device = this;
            bip->pin = 26 + i;
            bip->connectedNet = B[i];
            B[i]->connectedPorts.push_back(bip);
            inputPorts.push_back(bip);
        }

        for(int i = 0 ; i < 43; i++) {
            DeviceOutputPort *op = new DeviceOutputPort();
            op->device = this;
            op->pin = i;
            op->connectedNet = M[i];
            M[i]->connectedPorts.push_back(op);
            outputPorts.push_back(op);
        }
    }

    int Xilinx_DSP48Mul::dsp48count;

  }
}
