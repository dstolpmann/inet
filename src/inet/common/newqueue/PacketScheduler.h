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

#ifndef __INET_PACKETSCHEDULER_H
#define __INET_PACKETSCHEDULER_H

#include "inet/common/newqueue/base/PacketSchedulerBase.h"
#include "inet/common/newqueue/contract/IPacketSchedulerFunction.h"

namespace inet {
namespace queue {

class INET_API PacketScheduler : public PacketSchedulerBase
{
  protected:
    IPacketSchedulerFunction *packetSchedulerFunction;

  protected:
    virtual void initialize(int stage) override;
    virtual int schedulePacket() override;

  public:
    virtual ~PacketScheduler() { delete packetSchedulerFunction; }
};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_PACKETSCHEDULERBASE_H

