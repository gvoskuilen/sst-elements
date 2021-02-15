// Copyright 2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2020 NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_MMIO_INTERFACE_H
#define MEMHIERARCHY_MMIO_INTERFACE_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/subcomponent.h>
#include "memLinkBase.h"

namespace SST {
namespace MemHierarchy {

class MemEvent;

class MMIO : public SST::SubComponent {
public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::MMIO, std::string, std::string)

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MMIO, "memHierarchy", "MMIO", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Provides MMIO interface into the memory subsystem", SST::MemHierarchy::MMIO)

    SST_ELI_DOCUMENT_PARAMS()

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( { "link", "Interface to memory system", "SST::MemHierarchy::MemLinkBase"})

    /* Command types */
    enum class Command{Read, ReadResp, Write, WriteResp, CustomReq, CustomResp};

    /* 
     * Constructor
     */
    MMIO(ComponentId_t id, Params& params, std::string timebase, std::string compName);
    
    // Destructor
    ~MMIO() {};

    /* These functions must be called by parent prior to init() phase */
    // Addresses [base, base + size) map to this MMIO component
    void setMemoryAddress(Addr base, Addr size);
    // This is the handler that should be called when an event arrives
    void setHandler(std::function<void(Event::id_type,MMIO::Command,Addr,size_t,bool,vector<uint8_t>&)>);

    /*
     * SST init() function
     * Registers the mmio interface's address range with the rest of the memory system for future addressing
     * Also will pass any initialization data from CPU to MMIO interface
     */
    virtual void init(unsigned int);

    /*
     * SST setup() function
     * Error checking before simulation begins
     */
    virtual void setup();

    /* Called by parent to send various event types */
    void sendRead(Addr addr, size_t size);
    void sendWrite(Addr addr, vector<uint8_t>& payload, bool response);
    void sendResponse(Event::id_type id);
    void sendResponse(Event::id_type id, vector<uint8_t>& payload);
    //void sendCustomRequest();
    //void sendCustomResponse();
    
    /* Debug - called by parent */
    virtual void printStatus(Output &out);
    
protected:

    /* Handle event received on link */
    virtual void handleEvent( SST::Event* );

private:
    // Parent's name
    std::string compName;
    
    // Output & debug
    Output dbg;
    Output out;
    std::set<Addr> DEBUG_ADDR;

    // Link to mem subsystem
    SST::MemHierarchy::MemLinkBase * link_;

    // Parent handler to call when an event is received
    std::function<void(Event::id_type,MMIO::Command,Addr,size_t,bool,vector<uint8_t>&)> recvHandler;
    
    MemRegion region_;

    // Tracking events that need to be responded to
    std::map<SST::Event::id_type, MemEvent*> pendingResponses;

    // Name of parent component
    std::string pName; 
    
}; // end
        
}
}


#endif /* MEMHIERARCHY_MMIO_INTERFACE_H */
