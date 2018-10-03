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

#include "inet/common/newqueue/PacketDropperFunction.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

static bool isOverloaded(IPacketCollection *collection)
{
    auto maxNumPackets = collection->getMaxNumPackets();
    auto maxTotalLength = collection->getMaxTotalLength();
    return (maxNumPackets != -1 && collection->getNumPackets() > maxNumPackets) ||
           (maxTotalLength != b(-1) && collection->getTotalLength() > maxTotalLength);
}

static void dropPacket(IPacketCollection *collection, int i)
{
    auto packet = collection->getPacket(i);
    collection->removePacket(packet);
    PacketDropDetails details;
    details.setReason(QUEUE_OVERFLOW);
    details.setLimit(collection->getMaxNumPackets());
    check_and_cast<cModule *>(collection)->emit(packetDroppedSignal, packet, &details);
    delete packet;
}

static void dropTail(IPacketCollection *collection)
{
    while (!collection->isEmpty() && isOverloaded(collection))
        dropPacket(collection, collection->getNumPackets() - 1);
}

Register_Packet_Dropper_Function(DropTail, dropTail);

static void dropHead(IPacketCollection *collection)
{
    while (!collection->isEmpty() && isOverloaded(collection))
        dropPacket(collection, 0);
}

Register_Packet_Dropper_Function(DropHead, dropHead);

} // namespace queue
} // namespace inet

