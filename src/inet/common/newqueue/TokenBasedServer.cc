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

#include "inet/common/newqueue/TokenBasedServer.h"
#include "inet/common/Simsignals.h"
#include "inet/common/StringFormat.h"

namespace inet {
namespace queue {

Define_Module(TokenBasedServer);

void TokenBasedServer::initialize(int stage)
{
    PacketServerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        displayStringTextFormat = par("displayStringTextFormat");
        numTokens = par("intialNumTokens");
        maxNumTokens = par("maxNumTokens");
        tokenProductionTimer = new cMessage("TokenProductionTimer");
        WATCH(numTokens);
    }
    else if (stage == INITSTAGE_QUEUEING)
        scheduleTokenProductionTimer();
    else if (stage == INITSTAGE_LAST)
        updateDisplayString();
}

void TokenBasedServer::handleMessage(cMessage *message)
{
    if (message == tokenProductionTimer) {
        if (numTokens < maxNumTokens) {
            numTokens++;
            processPackets();
            updateDisplayString();
        }
        scheduleTokenProductionTimer();
    }
    else
        PacketServerBase::handleMessage(message);
}

void TokenBasedServer::scheduleTokenProductionTimer()
{
    scheduleAt(simTime() + par("tokenProductionInterval"), tokenProductionTimer);
}

void TokenBasedServer::processPackets()
{
    while (true) {
        auto packet = provider->canPopPacket(inputGate->getPathStartGate());
        if (packet == nullptr)
            break;
        else {
            double tokenConsumptionPerPacket = par("tokenConsumptionPerPacket");
            double tokenConsumptionPerBit = par("tokenConsumptionPerBit");
            int numRequiredTokens = tokenConsumptionPerPacket + tokenConsumptionPerBit * packet->getTotalLength().get();
            if (numTokens >= numRequiredTokens) {
                packet = provider->popPacket(inputGate->getPathStartGate());
                EV_INFO << "Processing packet " << packet->getName() << ".\n";
                animateSend(packet, outputGate);
                consumer->pushPacket(packet, outputGate->getPathEndGate());
                numTokens -= numRequiredTokens;
            }
            else
                break;
        }
    }
}

void TokenBasedServer::handleCanPushPacket(cGate *gate)
{
}

void TokenBasedServer::handleCanPopPacket(cGate *gate)
{
    processPackets();
    updateDisplayString();
}

void TokenBasedServer::updateDisplayString()
{
    auto text = StringFormat::formatString(displayStringTextFormat, [&] (char directive) {
        static std::string result;
        switch (directive) {
            case 'n':
                result = std::to_string(numTokens);
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

