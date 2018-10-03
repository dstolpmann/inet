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

package inet.common.newqueue;

import inet.common.newqueue.contract.IPacketBuffer;

//
// This module provides packet storage primarily for sharing resources between
// multiple packet queues.
//
simple PacketBuffer like IPacketBuffer
{
    parameters:
        int frameCapacity = default(-1); // maximum number of packets in the queue, no limit by default
        int dataCapacity @unit(b) = default(-1b); // maximum total length of packets in the queue, no limit by default
        string displayStringTextFormat = default("%p packets\n%l"); // %p number of packet, %l total length
        @display("i=block/buffer");
        @signal[packetAdded](type=inet::Packet);
        @signal[packetRemoved](type=inet::Packet);
        @signal[packetDropped](type=inet::Packet);
        @statistic[packetDropQueueOverflow](title="packet drops: queue overflow"; source=packetDropReasonIsQueueOverflow(packetDropped); record=count,sum(packetBytes),vector(packetBytes); interpolationmode=none);
}

