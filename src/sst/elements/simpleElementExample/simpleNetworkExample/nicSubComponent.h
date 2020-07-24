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

#ifndef _SIMPLE_NIC_SUBCOMPONENT_H
#define _SIMPLE_NIC_SUBCOMPONENT_H

#include<queue>

#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
namespace SimpleNetworkExample {

// Basic event that the endpoints will exchange, contains just the name of the sender
class nicEvent : public SST::Event
{
    public:
        nicEvent(std::string name) : Event(), source(name) { }
        std::string getSource() { return source; }

        virtual Event* clone(void) override {
            nicEvent* ev = new nicEvent(*this);
            return ev;
        }

    private:
        std::string source; // Name of sender

    public: /* Required for serialization */
        nicEvent() : Event() {}

        void serialize_order(SST::Core::Serialization::serializer &ser) override {
            Event::serialize_order(ser);
            ser & source;
        }
        
        ImplementSerializable(SST::SimpleNetworkExample::nicEvent);
};

// The API of the NIC. This doesn't need to be split from the nicSubComponent unless we
// want to support swapping the NIC model (as well as the SimpleNetwork interface).
class nicAPI : public SST::SubComponent 
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::SimpleNetworkExample::nicAPI)
    
    // Constructor & destructor
    nicAPI(ComponentId_t id, Params& params) : SubComponent(id) { }
    virtual ~nicAPI() { }

    // The endpoint uses this function to register an event handler which will be called 
    // when an event is received
    virtual void setMsgHandler(Event::HandlerBase* handler) =0;
    
    // SimpleNetwork requires calling init() on it during SST's init() phase
    // Since the simulator only calls init() on components (not subcomponents),
    // all NICs need to call init() on their SimpleNetwork interfaces and all
    // endpoints need to call init() on their NIC(s)
    virtual void init(unsigned int phase) =0;

    // This is optional - not required to call it on SimpleNetwork subcomponents,
    // so the NIC can implement if it needs and the endpoint should call it during
    // its own setup() function
    virtual void setup() { }

    // The endpoint uses this to send an event to destination 'dest'
    // on the network
    virtual void send(nicEvent* ev, int dest) =0;
    
    // Returns the number of destinations on the network
    // The NIC must manually count these (not provided by SimpleNetwork interface)
    virtual int getNumDestinations() =0;

    // Returns this NIC's network address, this calls through to the SimpleNetwork interface
    virtual SST::Interfaces::SimpleNetwork::nid_t getAddress() =0;
};

/*
 * This is the NIC SubComponent.
 *
 * The NIC loads a SimpleNetwork interface into its 'iface' slot, for example 'merlin.linkcontrol' or 'kingsley.linkcontrol'
 *
 * The NIC buffers messages from its parent, sending as many as possible on each clock tick
 * When it receives a message, it calls an event handler that its parent has registered 
 * 
 * The 'network' port only needs to be connected if 1) this NIC was declared in the Python and 2) the SimpleNetwork subcomponent was not declared in the Python
 * If (1) the NIC was not declared in the Python, connect the parent's port
 * If (2) the SimpleNetwork subcomponent was declared in the Python, connect the SimpleNetwork subcomponent's port
 */
class nicSubComponent : public nicAPI
{
public:

    // Register the SubComponent
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        nicSubComponent,
        "simpleElementExample",
        "nicSubComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Example NIC SubComponent that interfaces with SimpleNetwork",
        SST::SimpleNetworkExample::nicAPI
    )

    // Params
    SST_ELI_DOCUMENT_PARAMS(
        {"port", "Port to use, if loaded as an anonymous subcomponent", "network"},
        {"verbose", "Verbosity for output (0 = nothing)", "0"}
    )

    // Ports
    SST_ELI_DOCUMENT_PORTS(
        {"network", "Port to network", {"simpleNetworkExample.nicEvent"} }
    )

    // SubComponent Slots
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"iface", "SimpleNetwork interface to a network", "SST::Interfaces::SimpleNetwork"}
    )
  
    // Constructor
    nicSubComponent(ComponentId_t id, Params& params);

    // Destructor
    virtual ~nicSubComponent();

    // Callback to parent on received messages
    virtual void setMsgHandler(Event::HandlerBase* handler);

    // Functions that are called automatically by the simulator on components
    // Not called automatically on subcomponents to enforce call ordering (e.g., parent first)
    // So, the parent component should call these on subcomponents
    virtual void init(unsigned int phase);
    virtual void setup();

    // Send event to the destination address
    virtual void send(nicEvent* ev, int dest);

    // Get number of destinations
    virtual int getNumDestinations();

    // Get this endpoint's network address
    virtual SST::Interfaces::SimpleNetwork::nid_t getAddress();
    
    // This is a callback function that the SimpleNetwork interface calls when a message is received
    bool msgNotify(int virtualNetwork);
    
    // Clock
    // We queue events for our parent and drain on each clock if we can
    // This clock is independent of our parent - may happen before or after our parent's cycle
    virtual bool clockTick(Cycle_t cycle);

protected:
    // Output object
    Output *output;

    // SimpleNetwork interface
    SST::Interfaces::SimpleNetwork* iface;

    // Callback function for received messages
    SST::Event::HandlerBase * msgHandler;

    // Track whether initial broadcast has been sent
    bool initBroadcastSent;

    // Count how many destinations there are
    int numDestinations;

    // Buffer 'sent' requests here until the network can actually accept them
    // Will attempt to send on the clock cycle
    std::queue<SST::Interfaces::SimpleNetwork::Request*> sendQ;

};

} // namespace SimpleNetworkExample
} // namespace SST




#endif /* _SIMPLE_NIC_SUBCOMPONENT_H */
