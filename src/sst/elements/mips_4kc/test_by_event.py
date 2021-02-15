import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
op.add_option("-c", "--cacheSz", action="store", type="int", dest="cacheSz", default=4)
op.add_option("-f", "--faultLoc", action="store", type="int", dest="faultLoc", default=0)
op.add_option("-e", "--execFile", action="store", type="string", dest="execFile", default="test/matmat.out")
op.add_option("-b", "--faultBits", action="store", type="string", dest="faultBits", default="0")

op.add_option("-n", "--foo", action="store", type="string", dest="foo", default="test/matmat.out") #unused
(options, args) = op.parse_args()

execSplit = options.execFile.split('/')
baseExec = execSplit[len(execSplit)-1]

#figure out fault periods
if baseExec == "dmr_matmat.out":
    faultPeriod = 312769
    if options.faultLoc == 0x4:
        faultPeriod = 3456 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 57010 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 217732 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 204698 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 274744 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 193410 #WB_ADDR
elif baseExec == "dmr_matmatO3.out":
    faultPeriod = 81094
    if options.faultLoc == 0x4:
        faultPeriod = 3456 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 17855 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 37596 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 33686 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 55453 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 28438 #WB_ADDR
elif baseExec == "matmat.out":
    faultPeriod = 122831
    if options.faultLoc == 0x4:
        faultPeriod = 1728 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 21873 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 85395 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 81145 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 107270 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 76914 #WB_ADDR
elif baseExec == "matmatO3.out":
    faultPeriod = 30000
    if options.faultLoc == 0x4:
        faultPeriod = 1728 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 2711 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 10166 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 8128 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 12779 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 7932 #WB_ADDR
elif baseExec == "rd_matmat.out":
    faultPeriod = 122975
    if options.faultLoc == 0x4:
        faultPeriod = 1728 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 21783 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 85000 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 81000 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 107000 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 77000 #WB_ADDR
elif baseExec == "rd_matmatO3.out":
    faultPeriod = 30000
    if options.faultLoc == 0x4:
        faultPeriod = 1728 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 2711 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 10000 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 8560 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 13000 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 8364 #WB_ADDR
elif baseExec == "tmr_matmat.out":
    faultPeriod = 450000
    if options.faultLoc == 0x4:
        faultPeriod = 5184 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 81000 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 313000 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 296000 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 394000 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 280000 #WB_ADDR
elif baseExec == "tmr_matmatO3.out":
    faultPeriod = 111000
    if options.faultLoc == 0x4:
        faultPeriod = 5184 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 25000 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 49000 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 43573 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 74000 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 36000 #WB_ADDR
elif (baseExec == "qsort.out" or baseExec == "rd_qsort.out"):
    faultPeriod = 191000
    if options.faultLoc == 0x4:
        faultPeriod = 0 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 50000 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 141000 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 129000 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 191000 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 118000 #WB_ADDR
elif (baseExec == "qsortO3.out" or baseExec == "rd_qsortO3.out"):
    faultPeriod = 61000
    if options.faultLoc == 0x4:
        faultPeriod = 0 #MDU
    elif (options.faultLoc == 0x8 or options.faultLoc == 0x10 or options.faultLoc == 0x1000):
        faultPeriod = 7900 #MEM
    elif (options.faultLoc == 0x20 or options.faultLoc == 0x200 or options.faultLoc == 0x400):
        faultPeriod = 54000 #WB, INST_
    elif (options.faultLoc == 0x40):
        faultPeriod = 43000 #ALU
    elif (options.faultLoc == 0x80):
        faultPeriod = 61000 #MEM_BP
    elif (options.faultLoc == 0x800):
        faultPeriod = 42000 #WB_ADDR
else:
    print("Unrecognized executable. Using default faultPeriod")
    faultPeriod = 100000


print("Using faultPeriod of %s" % faultPeriod)

# Define the simulation components
comp_mips = sst.Component("MIPS4KC", "mips_4kc.MIPS4KC")
comp_mips.addParams({
    "verbose" : 0,
    "execFile" : options.execFile,
    "clock" : "1GHz",
    "fault_locations" : options.faultLoc,
    "fault_period" : faultPeriod,
    "fault_by_time" : 0,
    "fault_bits" : options.faultBits,    
    "timeout" : 2000000
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

