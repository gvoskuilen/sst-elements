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

#include "sst_config.h"

#include "endpoint.h"
#include "nicSubComponent.h"

#include "sst/core/event.h"

using namespace SST;
using namespace SST::SimpleNetworkExample;

/* Constructor */
endpointComponent::endpointComponent(ComponentId_t id, Params& params) :
  Component(id)
{
    // Create an output object, parameterize output verboseness
    int verbosity = params.find<int>("verbose", 0);
    output = new SST::Output("", verbosity, 0, SST::Output::STDOUT);

    // get parameters
    msgCount = params.find<int64_t>("msgCount", 10);
    msgFreq = params.find<int64_t>("msgFreq", 2);
    msgPerCycle = params.find<int64_t>("msgPerCycle", 1);

    // tell the simulator not to end until we're done
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // set our clock
    registerClock("1GHz", new Clock::Handler<endpointComponent>(this,
                &endpointComponent::clockTick));

    // load the NIC subcomponent
    // First, check if the user put something in our 'nic' subcomponent slot
    // The user should have connected the NIC's port, not ours
    nic = loadUserSubComponent<nicAPI>("nic");

    // If the user did not put something in the 'nic' slot, load the default
    // In this case, the NIC must use our port, check that it is conneced
    if (!nic) {
        if (!isPortConnected("component_port")) {
            output->fatal(CALL_INFO, -1, "Error in %s: 'component_port' is not connected\n");
        }
        Params nicParams;
        nicParams.insert("port", "component_port");
        nic = loadAnonymousSubComponent<nicAPI>("simpleElementExample.nicSubComponent", "nic", 
                0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
    }
    
    // register our message handler with the NIC
    nic->setMsgHandler(new Event::Handler<endpointComponent>(this, &endpointComponent::handleMessage));
  
    // Create an RNG to randomize traffic send pattern
    rng = new SST::RNG::MarsagliaRNG(11, 272727);
}

endpointComponent::~endpointComponent() { }

void endpointComponent::init(unsigned int phase)
{
    // REQUIRED: call init on the NIC subcomponent, which will call it on the SimpleNetwork interface
    nic->init(phase);
}


void endpointComponent::setup() 
{
    // Not needed for SimpleNetwork
    nic->setup();

    destinations = nic->getNumDestinations();
    address = (int) nic->getAddress();

    lastAddress = destinations - 1;

    // Stop when we receive this many messages
    expectedMsgSend = destinations * msgCount;
    expectedMsgRecv = destinations * msgCount;
    output->verbose(CALL_INFO, 1, 0, "%s address is %d and found %d destinations\n", getName().c_str(), 
            address, destinations);
}

// incoming events are scanned and deleted
void endpointComponent::handleMessage(Event *ev)
{

    /* Example: handle a message & cast to correct type (dynamic if needed). */
    nicEvent * event = static_cast<nicEvent*>(ev);
    expectedMsgRecv--;

    output->verbose(CALL_INFO, 1, 0, "%s received message from %s, waiting for %d\n", getName().c_str(), event->getSource().c_str(), expectedMsgRecv);

    /* Component must delete event */
    delete event;
}

// We can send up to msgPerCycle events per clock cycle and there's a
// 1/msgFreq chance of sending any given event
bool endpointComponent::clockTick( Cycle_t )
{
    for (int i = 0; i < msgPerCycle; i++) {
        
        // communicate?
        if (expectedMsgSend > 0 && (rng->generateNextInt32() % msgFreq) == 0) {
            // yes, create a new event
            nicEvent *event = new nicEvent(getName());
        
            // Round-robin through destinations, skipping ourselves
            lastAddress = (lastAddress + 1) % destinations;
            int dest = lastAddress;
            if (dest >= address)
                dest++;
       
            // Send event to nic, nic may not send immediately if network is busy
            nic->send(event, dest);
            expectedMsgSend--;
            output->verbose(CALL_INFO, 1, 0, "%s sent message to %d, %d left\n", getName().c_str(), dest, expectedMsgSend);
        }

    }
    if (expectedMsgSend == 0 && expectedMsgRecv == 0) {
        primaryComponentOKToEndSim(); // Tell simulator it's OK to end now
        return true; // We're done, no need to keep calling the clock
    } 

    // return false so we keep going
    return false;
}

