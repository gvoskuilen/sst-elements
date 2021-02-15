import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
op.add_option("-c", "--cacheSz", action="store", type="int", dest="cacheSz", default=4)
op.add_option("-e", "--execFile0", action="store", type="string", dest="execFile0", default="test/readerWriter.out")
op.add_option("-E", "--execFile1", action="store", type="string", dest="execFile1", default="test/readerWriter.out")

op.add_option("-f", "--faultFile", action="store", type="string", dest="faultFile", default="")

op.add_option("-n", "--foo", action="store", type="string", dest="foo", default="test/matmat.out") #unused
(options, args) = op.parse_args()


# Define the simulation components
comp_mips0 = sst.Component("MIPS4KC_0", "mips_4kc.MIPS4KC")
comp_mips0.addParams({
    "verbose" : 0,
    "execFile" : options.execFile0,
    "clock" : "1GHz",
    "fault_file" : options.faultFile, 
    "timeout" : 1000000,
    "stack_top"  : 0x80000000,
    "proc_num" : 0
})

comp_l1cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
comp_l1cache0.addParams({
    "access_latency_cycles" : "1",
    "cache_frequency" : "4 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "verbose" : 0,
    "L1" : "1",
    "cache_size" : "%dKiB"%options.cacheSz
})

#second mips core
comp_mips1 = sst.Component("MIPS4KC_1", "mips_4kc.MIPS4KC")
comp_mips1.addParams({
    "verbose" : 0,
    "execFile" : options.execFile1,
    "clock" : "1GHz",
    "fault_file" : options.faultFile, 
    "timeout" : 1000000,
    "stack_top"  : 0x81000000,
    "proc_num" : 1
})

comp_l1cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
comp_l1cache1.addParams({
    "access_latency_cycles" : "1",
    "cache_frequency" : "4 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    "verbose" : 0,
    "L1" : "1",
    "cache_size" : "%dKiB"%options.cacheSz
})

comp_bus = sst.Component("bus", "memHierarchy.Bus")
comp_bus.addParams({
      "bus_frequency" : "4 Ghz"
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : 1,
      "coherence_protocol" : "MSI",
      "debug_level" : 10,
      "backend.access_time" : "10 ps",
      "backing" : "malloc", 
      "clock" : "4GHz",
      "backend.mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.enableAllStatisticsForComponentType("memHierarchy.Cache")


# Define the simulation links
link_mips_cache0 = sst.Link("link_mips_mem_0")
link_mips_cache0.connect( (comp_mips0, "mem_link", "10ps"), (comp_l1cache0, "high_network_0", "10ps") )
link_l1_bus0 = sst.Link("link_l1_bus_0")
link_l1_bus0.connect( (comp_l1cache0, "low_network_0", "10ps"), (comp_bus, "high_network_0", "10ps"))

link_mips_cache1 = sst.Link("link_mips_mem_1")
link_mips_cache1.connect( (comp_mips1, "mem_link", "10ps"), (comp_l1cache1, "high_network_0", "10ps") )
link_l1_bus1 = sst.Link("link_l1_bus_1")
link_l1_bus1.connect( (comp_l1cache1, "low_network_0", "10ps"), (comp_bus, "high_network_1", "10ps"))


link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_bus, "low_network_0", "10ps"), (comp_memory, "direct_link", "10ps") )

