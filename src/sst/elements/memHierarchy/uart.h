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

#ifndef _UART_H
#define _UART_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include "sst/elements/memHierarchy/util.h"
#include "mmio.h"

namespace SST {
namespace MemHierarchy {

class UARTMessage : public SST::Event
{
public:
    UARTMessage() : SST::Event() { }
    uint32_t getData() {return data;}
    void setData(uint32_t d) {data = d;}

public:    
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & data;
    }

    ImplementSerializable(SST::MemHierarchy::UARTMessage);

protected:
    uint32_t data;
};
    
class UART : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(UART, "memHierarchy", "UART", SST_ELI_ELEMENT_VERSION(1,0,0),
            "UART component for sending and recieving", COMPONENT_CATEGORY_MEMORY)
    SST_ELI_DOCUMENT_PARAMS( 
            {"clock",               "(string) Clock frequency of controller", NULL},
            {"verbose",             "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
            {"base_addr",           "(uint) Lowest address handled by this UART.", "0"},
            {"mem_size",            "(uint) Size of address region in bytes.", "64"})
    SST_ELI_DOCUMENT_PORTS( {"other_UART",     "Connection to a second UART for communication. If no second UART, sends to stdout", {"memHierarchy.UARTMessage"} } )    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
            {"mmio", "MMIO interface into memory subsystem", "SST::MemHierarchy::MMIO"}
    )

    UART(ComponentId_t id, Params &params);

    virtual void init(unsigned int);
    virtual void setup();

protected:
    ~UART() {}

    /* Handle event from MMIO interface */
    void handleEvent(Event::id_type id, MMIO::Command cmd, Addr addr, size_t size, bool response, vector<uint8_t>& payload);

    /* Handle event from UART interface */
    virtual void handleUARTEvent( SST::Event* );

    /* Internal: handle a read */
    void handleRead(Event::id_type id, Addr addr, size_t size);

    /* Internal: handle a write */
    void handleWrite(Event::id_type id, Addr addr, size_t size, bool response, vector<uint8_t>& payload);

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);
    //virtual void emergencyShutdown();
    
    /* Output */
    Output out;

    MemRegion region_; // Which address region we are, for translating to local addresses

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void emergencyShutdown() {};

private:

    // where incoming data is stored
    deque<uint8_t> incomingBuffer;
    uint32_t tx_buffer;
    
    MMIO* mmio;

    SST::Link *other_UART;

}; // end UART
        
}
}


#endif /*  _UART_H */
