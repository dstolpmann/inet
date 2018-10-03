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

#ifndef __INET_PACKETQUEUEBASE_H
#define __INET_PACKETQUEUEBASE_H

#include "inet/common/newqueue/base/PacketQueueingElementBase.h"
#include "inet/common/newqueue/contract/IPacketQueue.h"
#include "inet/common/newqueue/contract/IPacketQueueingElement.h"

namespace inet {
namespace queue {

class INET_API PacketQueueBase : public PacketQueueingElementBase, public IPacketQueue
{
  protected:
    const char *displayStringTextFormat = nullptr;
    int numPushed = -1;
    int numPopped = -1;
    int numRemoved = -1;
    int numDropped = -1;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void emit(simsignal_t signal, cObject *object, cObject *details = nullptr) override;
    virtual void updateDisplayString();

  public:
    virtual bool canPopSomePacket(cGate *gate) override { return getNumPackets() > 0; }
    virtual bool canPushSomePacket(cGate *gate) override { return true; }
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETQUEUEBASE_H

