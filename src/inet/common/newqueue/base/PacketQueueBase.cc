//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#include "inet/common/newqueue/base/PacketQueueBase.h"
#include "inet/common/Simsignals.h"
#include "inet/common/StringFormat.h"

namespace inet {
namespace queue {

void PacketQueueBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        displayStringTextFormat = par("displayStringTextFormat");
        numPushed = 0;
        numPopped = 0;
        numRemoved = 0;
        numDropped = 0;
        WATCH(numPushed);
        WATCH(numPopped);
        WATCH(numRemoved);
        WATCH(numDropped);
    }
}

void PacketQueueBase::emit(simsignal_t signal, cObject *object, cObject *details)
{
    if (signal == packetPushedSignal)
        numPushed++;
    else if (signal == packetPoppedSignal)
        numPopped++;
    else if (signal == packetRemovedSignal)
        numRemoved++;
    else if (signal == packetDroppedSignal)
        numDropped++;
    cSimpleModule::emit(signal, object, details);
}

void PacketQueueBase::updateDisplayString()
{
    auto text = StringFormat::formatString(displayStringTextFormat, [&] (char directive) {
        static std::string result;
        switch (directive) {
            case 'p':
                result = std::to_string(getNumPackets());
                break;
            case 'l':
                result = getTotalLength().str();
                break;
            case 'u':
                result = std::to_string(numPushed);
                break;
            case 'o':
                result = std::to_string(numPopped);
                break;
            case 'r':
                result = std::to_string(numPopped);
                break;
            case 'd':
                result = std::to_string(numDropped);
                break;
            case 'n':
                result = !isEmpty() ? getPacket(0)->getFullName() : "";
                break;
            default:
                throw cRuntimeError("Unknown directive: %c", directive);
        }
        return result.c_str();
    });
    getDisplayString().setTagArg("t", 0, text);
}

} // namespace queue
} // namespace inet

