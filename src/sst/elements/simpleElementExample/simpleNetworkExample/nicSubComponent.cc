// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// REQUIRED include
#include "sst_config.h"

#include "nicSubComponent.h"

using namespace SST;
using namespace SimpleNetworkExample;

nicSubComponent::nicSubComponent(ComponentId_t id, Params& params) : nicAPI(id, params) {
    // Create output object
    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("", verbosity, 0, SST::Output::STDOUT);

    // Register a clock
    registerClock("1GHz", new Clock::Handler<nicSubComponent>(this, &nicSubComponent::clockTick));

    // Load the SimpleNetwork interface for the network we are using (e.g., merlin.linkcontrol)
    iface = loadUserSubComponent<SST::Interfaces::SimpleNetwork>("iface", ComponentInfo::SHARE_NONE, 1); // '1' is the number of virtual networks
    if (!iface) {
        // If the user didn't specify the interface in the python, use merlin by default
        Params netparams;
        netparams.insert("port_name", params.find<std::string>("port", "network"));
        netparams.insert("in_buf_size", "256B");
        netparams.insert("out_buf_size", "256B");
        netparams.insert("link_bw", "40GiB/s");
        // Load interface manually. Share our ports (the interface will use our 'network' port or the port our parent provided). 1 is the number of virtual networks
        iface = loadAnonymousSubComponent<SST::Interfaces::SimpleNetwork>("merlin.linkcontrol", "iface", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, netparams, 1);
    }

    // Set a callback function for the network interface
    // The interface will call our handler, and we'll call our parent's handler
    iface->setNotifyOnReceive(new SST::Interfaces::SimpleNetwork::Handler<nicSubComponent>(this, &nicSubComponent::msgNotify));

    // During initialization, we'll broadcast our presence to the other endpoints and count how many broadcasts we receive
    // Record whether we've sent the broadcast
    initBroadcastSent = false;

    // The number of destinations is detected during init()
    numDestinations = 0;

    // Set by parent
    msgHandler = nullptr;
}
    

nicSubComponent::~nicSubComponent() { }


void nicSubComponent::setMsgHandler(Event::HandlerBase* handler) {
    msgHandler = handler;
}

    
/* Init function, parent needs to call this during its own init function
 *
 * Behavior:
 * 1. Initialize network: REQUIRED
 * 2. The NICs broadcast their presence on the network to get the count of endpoints: OPTIONAL
 */
void nicSubComponent::init(unsigned int phase) {
    // This is required and no message can be
    // sent on the network until it is initialized
    iface->init(phase);

    // After network is initialized, we will broadcast our 
    // presence and count how many endpoints we find
    //
    // This is optional - we need to know how many endpoints there are, but we
    // can get this by dynamic detection (as here) or by passing a parameter from the Python
    // We are making an assumption that the endpoint IDs on this network are numbered 0...N,
    // if that were not the case we'd need to know which IDs were valid on this network
    if (iface->isNetworkInitialized()) {
        if (!initBroadcastSent) {
            initBroadcastSent = true;
            nicEvent* ev = new nicEvent(getName());

            SST::Interfaces::SimpleNetwork::Request * req = new SST::Interfaces::SimpleNetwork::Request();
            req->dest = SST::Interfaces::SimpleNetwork::INIT_BROADCAST_ADDR;
            req->src = iface->getEndpointID(); // The interface assigns each endpoint a unique address
            req->givePayload(ev);
            iface->sendInitData(req);
        }
    }

    // This is optional, needed if endpoints are exchanging messages during init()
    // Checks whether an init message has been received and returns the message if so
    while (SST::Interfaces::SimpleNetwork::Request * req = iface->recvInitData()) {
        // Extract the payload from the request
        nicEvent * ev = static_cast<nicEvent*>(req->takePayload());
        numDestinations++; // Increment destination count

        output->verbose(CALL_INFO, 1, 0, "%s received init message from %s\n", getName().c_str(), ev->getSource().c_str());
    }
}


// Called by parent during setup() phase
void nicSubComponent::setup() {
    // Require parent to register a msgHandler by the time setup() happens
    if (msgHandler == nullptr) {
        output->fatal(CALL_INFO, -1, "%s, Error: nicSubComponent implements a callback-based notification and parent has not registerd a callback function\n", getName().c_str());
    }
}


// Our message handler
// SimpleNetwork tells us when a message has arrived
// but we then need to actually pull the request out of the interface
bool nicSubComponent::msgNotify(int virtualNetwork) { 
    SST::Interfaces::SimpleNetwork::Request* req = iface->recv(0); // Receive an event from virtual network 0
    if (req != nullptr) {
        nicEvent* ev = static_cast<nicEvent*>(req->takePayload());
        delete req; // Delete the wrapper, we grabbed the payload already

        // Call parent's handler with the event
        (*msgHandler)(ev);
    }
    return true;
}


// Send an event
// Because the SimpleNetwork interface can reject events (e.g., no bandwidth to send)
// we queue the events and attempt to drain the events on each clock cycle
void nicSubComponent::send(nicEvent* event, int destination) {
    SST::Interfaces::SimpleNetwork::Request * req = new SST::Interfaces::SimpleNetwork::Request();
    req->dest = destination;
    req->src = iface->getEndpointID();
    req->givePayload(event);
    sendQ.push(req);
}


// Return the number of destinations on the network
int nicSubComponent::getNumDestinations() { 
    return numDestinations; 
}


SST::Interfaces::SimpleNetwork::nid_t nicSubComponent::getAddress() { 
    return iface->getEndpointID();
}


bool nicSubComponent::clockTick(Cycle_t cycle) {
    while (!sendQ.empty()) {
        // Check if there is space to send a 512-bit payload on virtual network 0 and attempt to send if so
        if (iface->spaceToSend(0, 512) && iface->send(sendQ.front(), 0)) {
            sendQ.pop();
        } else {
            break;
        }
    }

    return false; // Keep clock running 
}
