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
#include "inet/common/newqueue/PacketComparatorFunction.h"
#include "inet/common/newqueue/PacketDropperFunction.h"
#include "inet/common/newqueue/PacketQueue.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(PacketQueue);

void PacketQueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        inputGate = gate("in");
        producer = dynamic_cast<IPacketProducer *>(findConnectedModule(inputGate));
        outputGate = gate("out");
        collector = dynamic_cast<IPacketCollector *>(findConnectedModule(outputGate));
        frameCapacity = par("frameCapacity");
        dataCapacity = b(par("dataCapacity"));
        buffer = getModuleFromPar<IPacketBuffer>(par("bufferModule"), this, false);
        const char *comparatorClass = par("comparatorClass");
        if (*comparatorClass != '\0')
            packetComparatorFunction = check_and_cast<IPacketComparatorFunction *>(createOne(comparatorClass));
        if (packetComparatorFunction != nullptr)
            queue.setup(packetComparatorFunction);
        const char *dropperClass = par("dropperClass");
        if (*dropperClass != '\0')
            packetDropperFunction = check_and_cast<IPacketDropperFunction *>(createOne(dropperClass));
    }
    else if (stage == INITSTAGE_QUEUEING) {
        checkPushPacketSupport(inputGate);
        checkPopPacketSupport(outputGate);
        if (producer != nullptr)
            producer->handleCanPushPacket(inputGate);
    }
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

void PacketQueue::handleMessage(cMessage *message)
{
    if (producer != nullptr)
        producer->handleCanPushPacket(inputGate);
    delete message;
}

bool PacketQueue::isOverloaded()
{
    return (frameCapacity != -1 && getNumPackets() > frameCapacity) ||
           (dataCapacity != b(-1) && getTotalLength() > dataCapacity);
}

int PacketQueue::getNumPackets()
{
    return queue.getLength();
}

Packet *PacketQueue::getPacket(int index)
{
    return check_and_cast<Packet *>(queue.get(index));
}

void PacketQueue::pushPacket(Packet *packet, cGate *gate)
{
    emit(packetPushedSignal, packet);
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;
    // KLUDGE: begin
    if (frameCapacity != -1 && getNumPackets() + 1 > frameCapacity) {
        delete packet;
        return;
    }
    // KLUDGE: end
    queue.insert(packet);
    if (buffer != nullptr)
        buffer->addPacket(packet, this);
    else if (isOverloaded()) {
        if (packetDropperFunction != nullptr)
            packetDropperFunction->dropPackets(this);
        else
            throw cRuntimeError("Queue is overloaded and packet dropper function is not specified");
    }
    updateDisplayString();
    if (collector != nullptr && getNumPackets() != 0)
        collector->handleCanPopPacket(outputGate);
}

Packet *PacketQueue::popPacket(cGate *gate)
{
    auto packet = check_and_cast<Packet *>(queue.front());
    EV_INFO << "Popping packet " << packet->getName() << " from the queue." << endl;
    if (buffer != nullptr)
        buffer->removePacket(packet, this);
    else {
        queue.pop();
        updateDisplayString();
    }
    animateSend(packet, outputGate);
    emit(packetPoppedSignal, packet);
    return packet;
}

void PacketQueue::removePacket(Packet *packet)
{
    EV_INFO << "Removing packet " << packet->getName() << " from the queue." << endl;
    if (buffer != nullptr)
        buffer->removePacket(packet, this);
    else {
        queue.remove(packet);
        updateDisplayString();
        emit(packetRemovedSignal, packet);
    }
}

void PacketQueue::handlePacketRemoved(Packet *packet)
{
    queue.remove(packet);
    updateDisplayString();
    emit(packetRemovedSignal, packet);
}

} // namespace queue
} // namespace inet

