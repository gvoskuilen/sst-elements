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

#ifndef _SIMPLEEXAMPLE_ENDPOINT_COMPONENT_H
#define _SIMPLEEXAMPLE_ENDPOINT_COMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

// Include the API for the NIC SubComponent that we load
#include "nicSubComponent.h"

namespace SST {
namespace SimpleNetworkExample {

/*
 * This example demonstrates creating a NIC subcomponent that enables
 * interfacing with a network via the SimpleNetwork interface
 *
 * This component loads the NIC subcomponent and uses it to send messages
 * to other endpoints
 *
 */

class endpointComponent : public SST::Component
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        endpointComponent,
        "simpleElementExample",
        "endpointComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "SimpleNetwork SubComponent example: Endpoint that loads a SimpleNetwork/NIC interface",
        COMPONENT_CATEGORY_PROCESSOR
    )

    // Document the parameters that this component accepts
    SST_ELI_DOCUMENT_PARAMS(
        { "msgCount",   "Messages to send per endpoint, all endpoints should have same number", "10"},
        { "msgFreq",    "Approximate frequency of sending an event (1 event per msgFreq clock ticks).", "2"},
        { "msgPerCycle", "Maximum number of messages that can be sent in a cycle", "1"},
        { "verbose",    "Verbosity of output (0 = none)", "0"}
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
    )

    // Preferably the user sets the NIC subcomponent in the Python. If not, we do provide a default NIC so we need a default port too 
    // (since the NIC's port is only available if the NIC is part of the Python graph)
    SST_ELI_DOCUMENT_PORTS(
        {"component_port", "Port to use if the user did not declare the 'nic' subcomponent in the Python", {"simpleNetworkExample.nicEvent"} }
    )

    // This is the NIC subcomponent that this endpoint loads. 
    // Any NIC of type 'nicAPI' is compatible.
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"nic", "NIC SubComponent to load, defaults to simpleElementExample.nicSubComponent", "SST::SimpleElementExample::nicAPI"}
    )

    /* Constructor & Destructor */
    endpointComponent(SST::ComponentId_t id, SST::Params& params);
    ~endpointComponent();

    /*These functions are called by the simulation engine */
    
    // After all components are built, this function is called repeatedly on all components until all messages sent in this phase are delivered
    void init(unsigned int phase);
    
    // After init() completes, setup() is called once on each component
    void setup();
    
    // After simulation completes, finish() is called once on each component
    // Ours is empty (nothing to do)
    void finish() { }

private:
    // Message handler, registered with the NIC and called when a message arrives
    void handleMessage(SST::Event *ev);

    // Clock function
    virtual bool clockTick(SST::Cycle_t);

    // SST output object for output, debugging, and assert/fail conditions
    Output* output;

    // Parameters
    int msgFreq;        // Frequency of sending messages
    int msgCount;       // Number of messages to send per destination
    int msgPerCycle;    // Maximum messages per cycle

    // Variables for traffic generation
    int expectedMsgSend;    // Messages left to send
    int expectedMsgRecv;    // Messages left to receive
    int address;            // Our network address
    int destinations;       // Number of other destinations on the network (e.g., endpoints - 1)
    int lastAddress;        // Last address that we sent a message to

    // Pointer to the subcomponent in the 'nic' slot
    nicAPI* nic;

    // Random number generator for creating traffic
    SST::RNG::MarsagliaRNG* rng;
};

} // namespace SimpleNetworkExample
} // namespace SST

#endif /* _SIMPLEEXAMPLE_ENDPOINT_COMPONENT_H */
