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

#include "inet/common/newqueue/PriorityBuffer.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(PriorityBuffer);

void PriorityBuffer::makeRoomForPacket(ICallback *packetOwner, Packet *packet)
{
    auto id = check_and_cast<cModule *>(packetOwner)->getId();
    for (auto it : packets) {
        // TODO: provide something better than the module id
        if (check_and_cast<cModule *>(it.first)->getId() > id) {
            auto packet = it.second;
            removePacket(packet, it.first);
            PacketDropDetails details;
            details.setReason(QUEUE_OVERFLOW);
            details.setLimit(frameCapacity);
            emit(packetDroppedSignal, packet, &details);
            delete packet;
            if (!isOverloaded())
                return;
        }
    }
}

} // namespace queue
} // namespace inet

