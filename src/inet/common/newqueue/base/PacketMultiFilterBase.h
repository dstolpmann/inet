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

#ifndef __INET_PACKETMULTIFILTERBASE_H
#define __INET_PACKETMULTIFILTERBASE_H

#include "inet/common/newqueue/base/PacketQueueingElementBase.h"
#include "inet/common/newqueue/contract/IPacketCollection.h"
#include "inet/common/newqueue/contract/IPacketFilter.h"
#include "inet/common/newqueue/contract/IPacketQueueingElement.h"

namespace inet {
namespace queue {

class INET_API PacketMultiFilterBase : public PacketQueueingElementBase, public IPacketFilter
{
  protected:
    int numGates = 0;

    std::vector<cGate *> inputGates;
    std::vector<IPacketProducer *> producers;
    std::vector<IPacketProvider *> providers;

    std::vector<cGate *> outputGates;
    std::vector<IPacketCollector *> collectors;
    std::vector<IPacketConsumer *> consumers;
    std::vector<IPacketCollection *> collections;

  protected:
    virtual void initialize(int stage) override;
    virtual bool matchesPacket(cGate *gate, Packet *packet) = 0;

  public:
    virtual IPacketConsumer *getConsumer(cGate *gate) override { return this; }
    virtual IPacketProvider *getProvider(cGate *gate) override { return this; }

    virtual bool supportsPushPacket(cGate *gate) override { return true; }
    virtual bool canPushSomePacket(cGate *gate) override { return true; }
    virtual bool canPushPacket(Packet *packet, cGate *gate) override { return true; }
    virtual void pushPacket(Packet *packet, cGate *gate) override;

    virtual bool supportsPopPacket(cGate *gate) override { return true; }
    virtual bool canPopSomePacket(cGate *gate) override;
    virtual Packet *canPopPacket(cGate *gate) override { throw cRuntimeError("Invalid operation"); }
    virtual Packet *popPacket(cGate *gate) override;

    virtual int getNumPackets();
    virtual b getTotalLength();

    virtual void handleCanPushPacket(cGate *gate) override;
    virtual void handleCanPopPacket(cGate *gate) override;
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETMULTIFILTERBASE_H

