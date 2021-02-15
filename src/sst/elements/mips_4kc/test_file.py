import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
op.add_option("-c", "--cacheSz", action="store", type="int", dest="cacheSz", default=4)
op.add_option("-l", "--faultLoc", action="store", type="int", dest="faultLoc", default=0)
op.add_option("-f", "--faultFile", action="store", type="string", dest="faultFile", default="faultsFile.txt")
op.add_option("-e", "--execFile", action="store", type="string", dest="execFile", default="test/matmat.out")

op.add_option("-n", "--foo", action="store", type="string", dest="foo", default="test/matmat.out") #unused
(options, args) = op.parse_args()

execSplit = options.execFile.split('/')
baseExec = execSplit[len(execSplit)-1]

#figure out fault periods
if baseExec == "dmr_matmat.out":
    faultPeriod = 255000
    if options.faultLoc == 0x4:
        faultPeriod = 3456
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 57000
elif baseExec == "dmr_matmatO3.out":
    faultPeriod = 64600
    if options.faultLoc == 0x4:
        faultPeriod = 3456
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 17850
elif baseExec == "matmat.out":
    faultPeriod = 102600
    if options.faultLoc == 0x4:
        faultPeriod = 1728
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 21873
elif baseExec == "matmatO3.out":
    faultPeriod = 27540
    if options.faultLoc == 0x4:
        faultPeriod = 1728
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 2711
elif baseExec == "rd_matmat.out":
    faultPeriod = 102800
    if options.faultLoc == 0x4:
        faultPeriod = 1728
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 21870
elif baseExec == "rd_matmatO3.out":
    faultPeriod = 28000
    if options.faultLoc == 0x4:
        faultPeriod = 1728
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 2711
elif baseExec == "tmr_matmat.out":
    faultPeriod = 370200
    if options.faultLoc == 0x4:
        faultPeriod = 5184
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 81779
elif baseExec == "tmr_matmatO3.out":
    faultPeriod = 90550
    if options.faultLoc == 0x4:
        faultPeriod = 5184
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 25018
else:
    print("Unrecognized executable. Using default faultPeriod")
    faultPeriod = 100000
    if options.faultLoc == 0x4:
        faultPeriod = 1500   #ensure we get at least one MDU insert
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10):
        faultPeriod = 20000

print("Using faultPeriod of %s" % faultPeriod)

# Define the simulation components
comp_mips = sst.Component("MIPS4KC", "mips_4kc.MIPS4KC")
comp_mips.addParams({
    "verbose" : 0,
    "execFile" : options.execFile,
    "clock" : "1GHz",
    "fault_locations" : options.faultLoc,
    "fault_period" : faultPeriod,
    "fault_file" : options.faultFile, 
    "timeout" : 1000000
})

comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
    "access_latency_cycles" : "1",
    "cache_frequency" : "4 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : "MSI",
    "associativity" : "4",
    "cache_line_size" : "64",
    #"debug" : "1",
    #"debug_level" : "10",
    "verbose" : 0,
    "L1" : "1",
    "cache_size" : "%dKiB"%options.cacheSz
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
link_mips_cache = sst.Link("link_mips_mem")
link_mips_cache.connect( (comp_mips, "mem_link", "10ps"), (comp_l1cache, "high_network_0", "10ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "10ps"), (comp_memory, "direct_link", "10ps") )

