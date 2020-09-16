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

#ifndef _BASICCLOCKS_H
#define _BASICCLOCKS_H

/*
 * This example demonstrates different ways of managing links
 *  1. Event sizes are randomly selected between 0 and eventSize for each event
 *  2. The component uses a Statistic to count the number of payload bytes it received
 *
 * Concepts covered:
 *  - Clocks and clock handlers
 *  - TimeConverters
 *  - UnitAlgebra for error checking
 */

#include <sst/core/component.h>

namespace SST {
namespace simpleElementExample {


// Components inherit from SST::Component
class basicClocks : public SST::Component
{
public:

/*
 *  SST Registration macros register Components with the SST Core and 
 *  document their parameters, ports, etc.
 *  SST_ELI_REGISTER_COMPONENT is required, the documentation macros
 *  are only required if relevant
 */
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        basicClocks,                        // Component class
        "simpleElementExample",             // Component library (for Python/library lookup)
        "basicClocks",                      // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
        "Basic: managing clocks example",   // Description
        COMPONENT_CATEGORY_UNCATEGORIZED    // Category
    )

    // Document the parameters that this component accepts
    // { "parameter_name", "description", "default value or NULL if required" }
    SST_ELI_DOCUMENT_PARAMS(
        { "clock0",     "Frequency or period (with units) of clock0",     "1GHz" },
        { "clock1",     "Frequency or period (with units) of clock1",     "5ns" },
        { "clock2",     "Frequency or period (with units) of clock2",     "15ns" },
        { "clockTicks", "Number of clock0 ticks to execute",    "500" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS()
    
    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS()

    // Optional since there is nothing to document - see SubComponent examples for more info
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( )

// Class members

    // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
    basicClocks(SST::ComponentId_t id, SST::Params& params);
    
    // Destructor
    ~basicClocks();

private:
   
    // Clock handler for clock0
    bool mainTick(SST::Cycle_t cycle);

    // Clock handler for clock1 and clock2
    bool otherTick(SST::Cycle_t cycle, uint32_t id);
    
    // TimeConverters - see timeConverter.h/.cc in sst-core
    // These store a clock interval and can be used to convert between time
    TimeConverter* clock1converter;     // TimeConverter for clock1
    TimeConverter* clock2converter;     // TimeConverter for clock2
    Clock::HandlerBase* clock2Handler; // Clock2 handler (clock2Tick)


    // Params
    Cycle_t cycleCount;
    std::string clock0Freq;
    std::string clock1Freq;
    std::string clock2Freq;

    // SST Output object, for printing, error messages, etc.
    SST::Output* out;
    
    // Number of cycles between print statements in otherTick
    Cycle_t printInterval;
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASICCLOCKS_H */
