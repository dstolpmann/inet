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

#include "inet/common/newqueue/ThresholdDropper.h"

namespace inet {
namespace queue {

Define_Module(ThresholdDropper);

void ThresholdDropper::initialize(int stage)
{
    PacketMultiFilterBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        frameCapacity = par("frameCapacity");
        dataCapacity = b(par("dataCapacity"));
    }
}

bool ThresholdDropper::matchesPacket(cGate *gate, Packet *packet)
{
    if (frameCapacity != -1 && (getNumPackets() + 1) > frameCapacity)
        return false;
    if (dataCapacity != b(-1) && (getTotalLength() + packet->getTotalLength()) > dataCapacity)
        return false;
    return true;
}

} // namespace queue
} // namespace inet

