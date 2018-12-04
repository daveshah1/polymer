#include "LogicPort.hpp"
#include "Signal.hpp"
using namespace std;

namespace SynthFramework {
    namespace Polymer {
        void LogicPort::Disconnect() {
            int index = -1;
            for(int i = 0; i < connectedNet->connectedPorts.size(); i++) {
                if(connectedNet->connectedPorts[i] == this) {
                    index = i;
                }
            }
            if(index != -1) {
                connectedNet->connectedPorts.erase(connectedNet->connectedPorts.begin() + index);
            }
        }
    }
}
