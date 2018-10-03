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

package inet.common.newqueue.base;

simple PacketSourceBase
{
    parameters:
        string displayStringTextFormat = default("%p packets\n%l"); // %p number of packet, %l total length
        string packetNameFormat = default("%n-%c"); // %n module name, %c packet number
        string packetRepresentation @units("bitCount","bits","byteCount","bytes","applicationPacket") = default("byteCount"); // determines the chunk of the packet data
        volatile int packetLength @unit(b);
        @display("i=block/source");
}
