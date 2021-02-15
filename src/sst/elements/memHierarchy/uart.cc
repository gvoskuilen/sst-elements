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
#include "uart.h"


using namespace SST;
using namespace SST::MemHierarchy;
    

UART::UART(ComponentId_t id, Params &params) : SST::Component(id) {

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    bool found = false;

    // Memory address
    uint64_t base = params.find<uint64_t>("base_addr", 0);
    uint64_t size = params.find<uint64_t>("mem_size", 64);

    std::string clockfreq = params.find<std::string>("clock");
    UnitAlgebra clock_ua(clockfreq);
    if (!(clock_ua.hasUnits("Hz") || clock_ua.hasUnits("s")) || clock_ua.getRoundedValue() <= 0) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: clock. Must have units of Hz or s and be > 0. (SI prefixes ok). You specified '%s'\n", getName().c_str(), clockfreq.c_str());
    }

    mmio = loadUserSubComponent<MMIO>("mmio", ComponentInfo::SHARE_NONE, clockfreq, getName());
    mmio->setMemoryAddress(base, size);
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;
    mmio->setHandler(std::bind(&UART::handleEvent, this, _1, _2, _3, _4, _5, _6));

    // initilize other_uart link
    other_UART = configureLink( "other_UART", "1ns",
                                new Event::Handler<UART>(this, &UART::handleUARTEvent));
}

void UART::init(unsigned int phase) {
    mmio->init(phase);
}

void UART::setup() {
    mmio->setup();
}

void UART::handleEvent(Event::id_type id, MMIO::Command cmd, Addr addr, size_t size, bool response, vector<uint8_t>& payload) {
    if (cmd == MMIO::Command::Read) {
        handleRead(id, addr, size);
    } else if (cmd == MMIO::Command::Write) {
        handleWrite(id, addr, size, response, payload);
    } else {
        out.fatal(CALL_INFO,-1,"UART received unrecognized command\n");        
    }
}

void UART::handleWrite(Event::id_type id, Addr addr, size_t size, bool response, vector<uint8_t>& payload) {
    out.output("Handle Write\n");

    // ignore higher bits in address
    if ((addr & 0xfff) == 0) { // tx_send
        if (other_UART) {
            // send to other UART from buffer
            UARTMessage *msg = new UARTMessage();
            msg->setData(tx_buffer);
            out.output("UART sending: 0x%08" PRIx32 "\n", msg->getData());
            other_UART->send(msg);
        } else {
            // print buffer to stdout
            out.output("UART output: 0x%08" PRIx32 "\n", tx_buffer);
        }
    } else {
        // add to tx buffer
        tx_buffer = 0;
        int dataOffset = size-1;
        vector<uint8_t> &data = payload;
        stringstream buff;
        buff << "uart adding to tx-buffer (sz " << size << " addr 0x" << std::hex << addr << "):";
        while(dataOffset >= 0) {
            // add to buffer
            buff << data[dataOffset];
            tx_buffer <<= 8;
            tx_buffer += data[dataOffset];
            dataOffset--;
        }
        out.output("%s\n", buff.str().c_str());
    }

    /* Send response (ack) if needed */
    if (response) {
        mmio->sendResponse(id);
    }
}

void UART::handleRead(Event::id_type id, Addr addr, size_t size) {
    vector<uint8_t> payload;
    payload.resize(size, 0);

    out.output("UART Read from %llx %d bytes\n", addr, size);

    // we're a simple, humble 32-bit UART, not one of those fancy
    // 64-bit'ers
    const int maxRespSize = 4;
    sst_assert(size == maxRespSize, CALL_INFO, -1, 
            "Error (%s): This is a 32-bit UART..got an event that is too big (%" PRIu64 " bytes)\n",
            getName().c_str(), size);

    // ignore higher order bits in address
    if ((addr & 0xfff) == 0x4) {
        // return if we have data
        if (!incomingBuffer.empty()) {
            payload[0] = 1;
        }
        out.output("UART ready status: %x\n", payload[0]);
    } else { // read from buffer
        if (!incomingBuffer.empty()) { // if nothing to read, return 0s
            uint8_t inData = 0;
            int dataOffset = 0;
            while((dataOffset < maxRespSize) && !incomingBuffer.empty()) {
                inData = incomingBuffer.front();
                incomingBuffer.pop_front();
                payload[dataOffset] = inData;
                dataOffset++;
            }
        } 
    }
    mmio->sendResponse(id, payload);
}

void UART::printStatus(Output &statusOut) 
{
    statusOut.output("MemHierarchy::UART %s\n", getName().c_str());
    mmio->printStatus(statusOut);
    statusOut.output("End MemHierarchy::UART\n\n");
}

// handle UART-to-UART events
void UART::handleUARTEvent(SST::Event * event) {
    UARTMessage *msg = dynamic_cast<UARTMessage*>(event);
    assert(msg);

    uint32_t inData = msg->getData();
    out.output("UART got %08x from other UART\n", inData);
    int dataOffset = 0;
    while(dataOffset < sizeof(inData)) {
        uint8_t d = inData & 0xff;
        inData >>= 8;
        incomingBuffer.push_back(d);
        dataOffset++;
    }
    
    delete event;
}
