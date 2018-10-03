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

#include "inet/common/ModuleAccess.h"
#include "inet/common/newqueue/base/PacketClassifierBase.h"

namespace inet {
namespace queue {

void PacketClassifierBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        inputGate = gate("in");
        producer = dynamic_cast<IPacketProducer *>(findConnectedModule(inputGate));
        for (int i = 0; i < gateSize("out"); i++) {
            auto outputGate = gate("out", i);
            auto consumer = dynamic_cast<IPacketConsumer *>(getConnectedModule(outputGate));
            outputGates.push_back(outputGate);
            consumers.push_back(consumer);
        }
    }
    else if (stage == INITSTAGE_QUEUEING) {
        for (auto outputGate : outputGates)
            checkPushPacketSupport(outputGate);
        checkPushPacketSupport(inputGate);
    }
}

void PacketClassifierBase::pushPacket(Packet *packet, cGate *gate)
{
    EV_INFO << "Classifying packet " << packet->getName() << ".\n";
    int index = classifyPacket(packet);
    animateSend(packet, outputGates[index]);
    consumers[index]->pushPacket(packet, outputGates[index]->getPathEndGate());
}

void PacketClassifierBase::handleCanPushPacket(cGate *gate)
{
    if (producer != nullptr)
        producer->handleCanPushPacket(inputGate);
}

} // namespace queue
} // namespace inet

