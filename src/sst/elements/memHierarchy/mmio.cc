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

#include <sst_config.h>
#include "mmio.h"
#include "util.h"
#include "memEvent.h"
#include "memLinkBase.h"


using namespace SST;
using namespace SST::MemHierarchy;
    

// Debug macros
#ifdef __SST_DEBUG_OUTPUT__
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#define Debug(level, fmt, ... )
#endif

MMIO::MMIO(ComponentId_t id, Params &params, std::string timebase, std::string compName) : SubComponent(id) {

    // Output for debug
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    /* mmio interface does not have its own clock, but it does have a clock function for interfacing with link */
    // Give our links a default time base
    registerTimeBase(timebase);
    link_ = loadUserSubComponent<MemLinkBase>("link", ComponentInfo::SHARE_NONE, timebase);
    if (!link_) {
        dbg.fatal(CALL_INFO, -1, "(%s) Missing subcomponent in 'link' slot. If not sure, use 'memHierarchy.memNIC' for NoC interface or 'memHierachy.memLink' otherwise.\n",
            getName().c_str());
    }

    link_->setRecvHandler( new Event::Handler<MMIO>(this, &MMIO::handleEvent));
    std::size_t pos = getName().find(":");
    pName = getName().substr(0,pos);
}

/* This must be called by parent prior to init() */
void MMIO::setMemoryAddress(Addr base, Addr size) {
    region_.start = base;
    region_.end = base + size - 1;
    region_.interleaveSize = 0;
    region_.interleaveStep = 0;
    link_->setRegion(region_);
}

/* This must be called by parent prior to init() */
void MMIO::setHandler(std::function<void(Event::id_type,MMIO::Command,Addr,size_t,bool,vector<uint8_t>&)> handler) {
    recvHandler = handler;
}


/* This must be called by parent component */
void MMIO::init(unsigned int phase) {
    link_->init(phase);

    /* Inherit region from our source(s) */
    if (!phase) {
        /* Announce our presence on link */
        /* Notes: Many of these fields are for coherence protocol handshake and don't matter
         */
        link_->sendInitData(new MemEventInitCoherence(compName, Endpoint::MMIO, true, false, 1, false));
    }

    while (MemEventInit *ev = link_->recvInitData()) {
        delete ev; 
    }
}

/* This must be called by parent component */
void MMIO::setup() {
    link_->setup();
}

/* Event receive on link, pass to parent */
//void MMIO::handleEvent(Event::id_type id, MMIO::Command cmd, Addr addr, size_t size, vector<uint8_t>* payload, CustomData * data) {
void MMIO::handleEvent(Event* event) {
    
    MemEventBase *meb = static_cast<MemEventBase*>(event);

    // Debug, ifdef'd on configuration
    if (is_debug_event(meb)) {
        Debug(_L3_, "%" PRIu64 "    %s    Received: %s\n",
            getCurrentSimTimeNano(), getName().c_str(), meb->getVerboseString().c_str());
    }

    // Sanity check address - TODO ifdef on debug setting?
    if (!region_.contains(meb->getRoutingAddress())) {
        out.fatal(CALL_INFO, -1, "%s, Error: Received an event with a routing address that does not map to this MMIO Interface. Event: %s\n",
            getName().c_str(), meb->getVerboseString().c_str());
    }

    // Pass event to parent
    if (meb->getCmd() == MemHierarchy::Command::GetS || meb->getCmd() == MemHierarchy::Command::GetX) {
        MemEvent * me = static_cast<MemEvent*>(meb);
        MMIO::Command cmd = MMIO::Command::Read;
        if (meb->getCmd() == MemHierarchy::Command::GetX) {
            cmd = MMIO::Command::Write;
        }
        bool response = !(me->queryFlag(MemEventBase::F_NORESPONSE));
        if (response) {
            // In case we have an untimed parent that attempts to send a response in the same cycle we deliver the request
            pendingResponses.insert(std::make_pair(me->getID(), me));
        }
        recvHandler(me->getID(), cmd, me->getAddr(), me->getSize(), response, me->getPayload());
        if (!response) {
            delete me;
        }
    } else if (meb->getCmd() == MemHierarchy::Command::GetSResp || meb->getCmd() == MemHierarchy::Command::GetXResp) {
        MemEvent * me = static_cast<MemEvent*>(meb);
        MMIO::Command cmd = MMIO::Command::ReadResp;
        if (meb->getCmd() == MemHierarchy::Command::GetXResp) {
            cmd = MMIO::Command::WriteResp;
        }
        recvHandler(me->getID(), cmd, me->getAddr(), me->getSize(), false, me->getPayload());
        delete me;
  //  } else if (meb->getCmd() == Command::CustomReq) {
  //      recvHandler(meb->getID(), meb->getCmd(), meb->getEventSize(), meb->queryFlag(MemEventBase::F_NORESPONSE), nullptr);
  //  } else if (meb->getCmd() == Command::CustomResp || meb->getCmd() == Command::CustomAck) {
  //      recvHandler(meb->getID(), meb->getCmd(), meb->getEventSize(), false, nullptr, meb->getCustomInfo());
    } else {
        // FATAL
        dbg.fatal(CALL_INFO, -1, "(%s) Received request with unhandled command type: %s\n",
            getName().c_str(), CommandString[(int)meb->getCmd()]);
    }
}

// Send a Read request to address 'addr' for 'size' bytes.
void MMIO::sendRead(Addr addr, size_t size) {
    MemEvent * ev = new MemEvent(pName, addr, addr, MemHierarchy::Command::GetS);
    ev->setFlag(MemEventBase::F_NONCACHEABLE);
    ev->setDst(link_->findTargetDestination(addr));
    if (is_debug_event(ev)) {
        Debug(_L3_, "%" PRIu64 "    %s    Send: %s\n",
            getCurrentSimTimeNano(), getName().c_str(), ev->getVerboseString().c_str());
    }

    link_->send(ev);
}

// Send a Write request to address 'addr' with payload. Specify whether ack is needed ('response').
void MMIO::sendWrite(Addr addr, vector<uint8_t>& payload, bool response) {
    MemEvent * ev = new MemEvent(pName, addr, addr, MemHierarchy::Command::GetX, payload);
    ev->setFlag(MemEventBase::F_NONCACHEABLE);
    ev->setDst(link_->findTargetDestination(addr));
    if (!response) {
        ev->setFlag(MemEventBase::F_NORESPONSE);
    }
    if (is_debug_event(ev)) {
        Debug(_L3_, "%" PRIu64 "    %s    Send: %s\n",
            getCurrentSimTimeNano(), getName().c_str(), ev->getVerboseString().c_str());
    }

    link_->send(ev);
}

// Send a Read or Write response for an event that arrived with ID 'id'.
void MMIO::sendResponse(Event::id_type id, vector<uint8_t>& payload) {
    // Create response
    std::map<SST::Event::id_type,MemEvent*>::iterator it = pendingResponses.find(id);
    if (it == pendingResponses.end()) {
        out.fatal(CALL_INFO, -1, "%s, Request to send a response but ID not found: %" PRIu64 ", %" PRIu32 "\n",
            getName().c_str(), id.first, id.second);
    }
    MemEvent * req = it->second;
    MemEvent * resp = req->makeResponse();
    resp->setPayload(payload);
    if (is_debug_event(resp)) {
        Debug(_L3_, "%" PRIu64 "    %s    Send: %s\n",
            getCurrentSimTimeNano(), getName().c_str(), resp->getVerboseString().c_str());
    }
    
    // Send
    link_->send(resp);
    
    // Clean up
    pendingResponses.erase(it);
    delete req;
}

// Send a Read or Write response for an event that arrived with ID 'id'.
void MMIO::sendResponse(Event::id_type id) {
    // Create response
    std::map<SST::Event::id_type,MemEvent*>::iterator it = pendingResponses.find(id);
    if (it == pendingResponses.end()) {
        out.fatal(CALL_INFO, -1, "%s, Request to send a response (ack) but ID not found: %" PRIu64 ", %" PRIu32 "\n",
            getName().c_str(), id.first, id.second);
    }

    MemEvent * req = it->second;
    MemEvent * resp = req->makeResponse();
    if (is_debug_event(resp)) {
        Debug(_L3_, "%" PRIu64 "    %s    Send: %s\n",
            getCurrentSimTimeNano(), getName().c_str(), resp->getVerboseString().c_str());
    }

    // Send
    link_->send(resp);
    
    // Clean up
    pendingResponses.erase(it);
    delete req;
}

void MMIO::printStatus(Output &statusOut) {
    // TODO add pendingResponses
    statusOut.output("  Link Status (%s): ", getName().c_str());
    link_->printStatus(statusOut);
}