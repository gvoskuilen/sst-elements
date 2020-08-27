# Import the SST module
import sst

# The basicStatisticsX.py scripts demonstrate user-side configuration of statistics
# This example demonstrates enabling statistics and reporting statistics at the end of simulation
#
# This component has no links and SST will produce a warning because that is an unusual configuration
# that often points to a mis-configuration. For this simulation, the warning can be ignored.
#
# Relevant code:
#   simpleElementExample/basicStatistics.h
#   simpleElementExample/basicStatistics.cc
#   simpleElementExample/basicEvent.h
#

### Create the component
component = sst.Component("StatisticComponent", "simpleElementExample.basicStatistics")

### Parameterize the component.
# Run 'sst-info simpleElementExample.basicStatistics' at the command line 
# to see parameter documentation
params = {
        "marsagliaZ" : 438,     # Seed for Marsaglia RNG
        "marsagliaW" : 9375794, # Seed for Marsaglia RNG
        "mersenne" : 102485,    # Seed for Mersenne RNG
        "run_cycles" : 1000,    # Number of cycles to run for
        "subids" : 3            # Number of SUBID_statistic instances
}
component.addParams(params)

### Enable statistics
# Limit the verbosity of statistics to any with a load level from 0-7
# This component's statistics range from 1-4 (see sst-info)
sst.setStatisticLoadLevel(7)

# Determine where statistics should be sent
#sst.setStatisticOutput("sst.statOutputConsole") 
sst.setStatisticOutput("sst.statOutputCSV", { "filepath" : "./stats.csv", "seperator" : "," } ) 
#sst.setStatisticOutput("sst.statOutputTXT", { "filepath" : "./example1.txt" } )

# Enable statistics on both components
sst.enableAllStatisticsForComponentType("simpleElementExample.basicStatistics")


