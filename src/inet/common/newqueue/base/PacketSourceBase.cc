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

#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/common/newqueue/base/PacketSourceBase.h"
#include "inet/common/packet/chunk/BitCountChunk.h"
#include "inet/common/packet/chunk/BitsChunk.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include "inet/common/packet/chunk/BytesChunk.h"
#include "inet/common/Simsignals.h"
#include "inet/common/StringFormat.h"
#include "inet/common/TimeTag_m.h"

namespace inet {
namespace queue {

void PacketSourceBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        displayStringTextFormat = par("displayStringTextFormat");
        packetNameFormat = par("packetNameFormat");
        packetRepresentation = par("packetRepresentation");
        packetLengthParameter = &par("packetLength");
        WATCH(numPacket);
        WATCH(totalLength);
    }
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

void PacketSourceBase::updateDisplayString()
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

const char *PacketSourceBase::createPacketName()
{
    return StringFormat::formatString(packetNameFormat, [&] (char directive) {
        static std::string result;
        switch (directive) {
            case 'n':
                result = getFullName();
                break;
            case 'c':
                result = std::to_string(numPacket);
                break;
            default:
                throw cRuntimeError("Unknown directive: %c", directive);
        }
        return result.c_str();
    });
}

Ptr<Chunk> PacketSourceBase::createPacketContent()
{
    auto packetLength = b(packetLengthParameter->intValue());
    if (!strcmp(packetRepresentation, "bitCount"))
        return makeShared<BitCountChunk>(packetLength);
    else if (!strcmp(packetRepresentation, "bits")) {
        static int total = 0;
        const auto& packetContent = makeShared<BitsChunk>();
        std::vector<bool> bits;
        bits.resize(b(packetLength).get());
        for (int i = 0; i < (int)bits.size(); i++)
            bits[i] = (total + i) % 2 == 0;
        total += bits.size();
        packetContent->setBits(bits);
        return packetContent;
    }
    else if (!strcmp(packetRepresentation, "byteCount"))
        return makeShared<ByteCountChunk>(packetLength);
    else if (!strcmp(packetRepresentation, "bytes")) {
        static int total = 0;
        const auto& packetContent = makeShared<BytesChunk>();
        std::vector<uint8_t> bytes;
        bytes.resize(B(packetLength).get());
        for (int i = 0; i < (int)bytes.size(); i++)
            bytes[i] = (total + i) % 256;
        total += bytes.size();
        packetContent->setBytes(bytes);
        return packetContent;
    }
    else if (!strcmp(packetRepresentation, "applicationPacket")) {
        const auto& packetContent = makeShared<ApplicationPacket>();
        packetContent->setChunkLength(B(packetLength));
        packetContent->setSequenceNumber(numPacket);
        return packetContent;
    }
    else
        throw cRuntimeError("Unknown representation");
}

Packet *PacketSourceBase::createPacket()
{
    auto packetName = createPacketName();
    auto packetContent = createPacketContent();
    packetContent->addTag<CreationTimeTag>()->setCreationTime(simTime());
    auto packet = new Packet(packetName, packetContent);
    numPacket++;
    totalLength += packet->getDataLength();
    emit(packetCreatedSignal, packet);
    return packet;
}

} // namespace queue
} // namespace inet

