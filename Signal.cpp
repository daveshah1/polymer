#include "Signal.hpp"
#include "Util.hpp"
#include "LogicPort.hpp"
#include "LogicDevice.hpp"
#include <typeinfo>
using namespace std;

namespace SynthFramework {
    namespace Polymer {

        int Signal::GetFanout() {
            int fanout = 0;
            for(auto port : connectedPorts) {
                if(!port->IsDriver()) {
                    fanout++;
                }
            }
            return fanout;
        }

        void Signal::ConnectTo(Signal* other) {
            for(auto port : connectedPorts) {
                port->connectedNet = other;
                other->connectedPorts.push_back(port);
            }
            if(isSlow)
              other->isSlow = true;
            for(auto bus : parentBuses) {
                for(auto &sig : bus->signals) {
                    if(sig == this) {
                        sig = other;
                    }
                }
                other->parentBuses.push_back(bus);
            }
            parentBuses.clear();
            connectedPorts.clear();
        }

        bool Signal::GetConstantValue(bool &val) {
            for(auto port : connectedPorts) {
                if(port->IsDriver() && port->IsConstantDriver()) {
                    switch(port->GetConstantState()) {
                    case LogicState_Unknown:
                        break;
                    case LogicState_Low:
                        val = false;
                        return true;
                    case LogicState_High:
                        val = true;
                        return true;
                    case LogicState_DontCare:
                        val = false;
                        return true;
                    }

                }
            }
            return false;
        }

        Bus::Bus() {

        }

        Bus::Bus(string _name, bool _is_signed, int _width) {
            name = _name;
            is_signed = _is_signed;
            width = _width;
        }

        bool Bus::GetConstantValue(long long &val) {
            long long val_tmp = 0;
            for(int i = 0; i < signals.size(); i++) {
                bool cval;
                if(!signals[i]->GetConstantValue(cval)) {
                    return false;
                } else {
                    if(cval) {
                        val_tmp |= 1LL << i;
                    }
                }
            }
            //sign extend if necessary
            if(is_signed) {
                if((val_tmp & (1LL << (signals.size() - 1))) != 0) {
                    for(int i = signals.size(); i < (8*sizeof(long long)); i++) {
                        val_tmp |= (1LL << i);
                    }
                }
            }
            val = val_tmp;
            return true;
        }

        LogicDevice *Signal::GetDriver() {
            for(auto port : connectedPorts) {
                if(port->IsDriver()) {
                    DeviceOutputPort* devPort = dynamic_cast<DeviceOutputPort*>(port);
                    if(devPort != nullptr) {
                        return devPort->device;
                    }
                }
            }
            return nullptr;
        }
    }
}
