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
#include "inet/common/newqueue/PacketCollector.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(PacketCollector);

void PacketCollector::initialize(int stage)
{
    PacketSinkBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        inputGate = gate("in");
        provider = check_and_cast<IPacketProvider *>(getConnectedModule(inputGate));
        collectionIntervalParameter = &par("collectionInterval");
        collectionTimer = new cMessage("CollectionTimer");
    }
    else if (stage == INITSTAGE_QUEUEING)
        checkPopPacketSupport(inputGate);
}

void PacketCollector::handleMessage(cMessage *message)
{
    if (message == collectionTimer) {
        if (provider->canPopSomePacket(gate("in")->getPathStartGate())) {
            collectPacket();
            scheduleCollectionTimer();
        }
    }
    else
        throw cRuntimeError("Unknown message");
}

void PacketCollector::scheduleCollectionTimer()
{
    scheduleAt(simTime() + collectionIntervalParameter->doubleValue(), collectionTimer);
}

void PacketCollector::collectPacket()
{
    auto packet = provider->popPacket(gate("in")->getPathStartGate());
    EV_INFO << "Collecting packet " << packet->getName() << "." << endl;
    numPacket++;
    totalLength += packet->getDataLength();
    PacketDropDetails details;
    details.setReason(OTHER_PACKET_DROP);
    emit(packetDroppedSignal, packet, &details);
    updateDisplayString();
    delete packet;
}

void PacketCollector::handleCanPopPacket(cGate *gate)
{
    if (gate->getPathEndGate() == inputGate && !collectionTimer->isScheduled()) {
        collectPacket();
        scheduleCollectionTimer();
    }
}

} // namespace queue
} // namespace inet

