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

#include "inet/common/newqueue/base/PacketSinkBase.h"
#include "inet/common/StringFormat.h"

namespace inet {
namespace queue {

void PacketSinkBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        displayStringTextFormat = par("displayStringTextFormat");
        WATCH(numPacket);
        WATCH(totalLength);
    }
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

void PacketSinkBase::updateDisplayString()
{
    auto text = StringFormat::formatString(displayStringTextFormat, [&] (char directive) {
        static std::string result;
        switch (directive) {
            case 'p':
                result = std::to_string(numPacket);
                break;
            case 'l':
                result = totalLength.str();
                break;
            default:
                throw cRuntimeError("Unknown directive: %c", directive);
        }
        return result.c_str();
    });
    getDisplayString().setTagArg("t", 0, text);
}

} // namespace queue
} // namespace inet

