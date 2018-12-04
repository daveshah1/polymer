#include "LogicDevice.hpp"
#include "BasicDevices.hpp"
#include <typeinfo>
using namespace std;

namespace SynthFramework {
    namespace Polymer {
        bool DeviceInputPort::IsDriver() {
            return false;
        }

        bool DeviceInputPort::IsConstantDriver() {
            return false;
        }

        LogicState DeviceInputPort::GetConstantState() {
            return LogicState_Unknown;
        }

        bool DeviceOutputPort::IsDriver() {
            return true;
        }

        bool DeviceOutputPort::IsConstantDriver() {
            ConstantDevice *cdev = dynamic_cast<ConstantDevice*>(device);
            if(cdev != nullptr) {
                return true;
            } else {
                return false;
            }
        }

        LogicState DeviceOutputPort::GetConstantState() {
            ConstantDevice *cdev = dynamic_cast<ConstantDevice*>(device);
            if(cdev != nullptr) {
                if(cdev->value) {
                    return LogicState_High;
                } else {
                    return LogicState_Low;
                }
            } else {
                return LogicState_Unknown;
            }
        }

        void LogicDevice::RemoveDevice() {
            for(auto inp : inputPorts) {
                inp->Disconnect();
            }
            for(auto outp : outputPorts) {
                outp->Disconnect();
            }
        }

        bool LogicDevice::OptimiseDevice(LogicDesign *topLevel) {
            return false;
        }

        bool LogicDevice::IsEquivalentTo(LogicDevice* other) {
            return false;
        }

    }
}
