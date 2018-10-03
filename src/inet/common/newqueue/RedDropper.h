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

#ifndef __INET_REDDROPPER_H
#define __INET_REDDROPPER_H

#include "inet/common/newqueue/base/PacketMultiFilterBase.h"

namespace inet {
namespace queue {

/**
 * Implementation of Random Early Detection (RED).
 */
class INET_API RedDropper : public PacketMultiFilterBase
{
  protected:
    double wq = 0.0;
    double *minths = nullptr;
    double *maxths = nullptr;
    double *maxps = nullptr;
    double *pkrates = nullptr;
    double *count = nullptr;

    double avg = 0.0;
    simtime_t q_time;

  protected:
    virtual void initialize(int stage) override;
    virtual bool matchesPacket(cGate *gate, Packet *packet) override;
//    virtual void sendOut(Packet *packet) override;

  public:
    virtual ~RedDropper();

};

} // namespace queue
} // namespace inet

#endif // ifndef __INET_REDDROPPER_H

