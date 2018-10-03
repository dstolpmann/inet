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

#include "inet/common/newqueue/base/PacketMultiFilterBase.h"
#include "inet/common/newqueue/PacketMultiplexer.h" // KLUDGE: TODO:
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

void PacketMultiFilterBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        numGates = gateSize("in");
        for (int i = 0; i < numGates; ++i) {
            auto inputGate = gate("in", i);
            auto connectedGate = inputGate->getPathStartGate();
            if (!connectedGate)
                throw cRuntimeError("In gate %d is not connected", i);
            auto input = dynamic_cast<IPacketProducer *>(connectedGate->getOwnerModule());
            auto provider = dynamic_cast<IPacketProvider *>(connectedGate->getOwnerModule());
            providers.push_back(provider);
            inputGates.push_back(inputGate);
            producers.push_back(input);
        }
        for (int i = 0; i < numGates; ++i) {
            auto outputGate = gate("out", i);
            auto connectedGate = outputGate->getPathEndGate();
            if (!connectedGate)
                throw cRuntimeError("Out gate %d is not connected", i);
            auto output = dynamic_cast<IPacketCollector *>(connectedGate->getOwnerModule());
            auto consumer = dynamic_cast<IPacketConsumer *>(connectedGate->getOwnerModule());
            consumers.push_back(consumer);
            outputGates.push_back(outputGate);
            collectors.push_back(output);
            auto collection = dynamic_cast<IPacketCollection *>(connectedGate->getOwnerModule());
            // TODO: KLUDGE: to see through a PacketMultiplexer from a RedDropper
            if (!collection)
                collection = check_and_cast<IPacketCollection *>(check_and_cast<PacketMultiplexer *>(connectedGate->getOwnerModule())->gate("out")->getPathEndGate()->getOwnerModule());
            collections.push_back(collection);
        }
    }
}

// TODO: several functions are copied from PacketFilterBase and extended with multiple gate support via indices
void PacketMultiFilterBase::pushPacket(Packet *packet, cGate *gate)
{
    if (matchesPacket(gate, packet)) {
        EV_INFO << "Passing through packet " << packet->getName() << "." << endl;
        animateSend(packet, outputGates[gate->getIndex()]);
        consumers[gate->getIndex()]->pushPacket(packet, outputGates[gate->getIndex()]->getPathEndGate());
    }
    else {
        EV_INFO << "Filtering out packet " << packet->getName() << "." << endl;
        PacketDropDetails details;
        details.setReason(OTHER_PACKET_DROP);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
}

bool PacketMultiFilterBase::canPopSomePacket(cGate *gate)
{
    auto provider = providers[gate->getIndex()];
    auto providerGate = inputGates[gate->getIndex()]->getPathStartGate();
    while (true) {
        auto packet = provider->canPopPacket(providerGate);
        if (matchesPacket(gate, packet))
            return true;
        else {
            packet = provider->popPacket(providerGate);
            EV_INFO << "Filtering out packet " << packet->getName() << "." << endl;
            PacketDropDetails details;
            details.setReason(OTHER_PACKET_DROP);
            emit(packetDroppedSignal, packet, &details);
            delete packet;
        }
    }
}

Packet *PacketMultiFilterBase::popPacket(cGate *gate)
{
    auto provider = providers[gate->getIndex()];
    auto providerGate = inputGates[gate->getIndex()]->getPathStartGate();
    while (true) {
        auto packet = provider->popPacket(providerGate);
        if (matchesPacket(gate, packet)) {
            EV_INFO << "Passing through packet " << packet->getName() << "." << endl;
            animateSend(packet, gate);
            return packet;
        }
        else {
            EV_INFO << "Filtering out packet " << packet->getName() << "." << endl;
            PacketDropDetails details;
            details.setReason(OTHER_PACKET_DROP);
            emit(packetDroppedSignal, packet, &details);
            delete packet;
        }
    }
}

int PacketMultiFilterBase::getNumPackets()
{
    int len = 0;
    for (const auto & elem : collections)
        len += (elem)->getNumPackets();
    return len;
}

b PacketMultiFilterBase::getTotalLength()
{
    b len = b(0);
    for (const auto & elem : collections)
        len += (elem)->getTotalLength();
    return len;
}

void PacketMultiFilterBase::handleCanPushPacket(cGate *gate)
{
    int index = gate->getPathStartGate()->getIndex();
    if (producers[index] != nullptr)
        producers[index]->handleCanPushPacket(inputGates[index]);
}

void PacketMultiFilterBase::handleCanPopPacket(cGate *gate)
{
    int index = gate->getPathEndGate()->getIndex();
    if (collectors[index] != nullptr)
        collectors[index]->handleCanPopPacket(outputGates[index]);
}

} // namespace queue
} // namespace inet

