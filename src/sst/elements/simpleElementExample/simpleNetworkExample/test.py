import sst

#####################
# Topology
#####################
#
#   endpoint0             endpoint1
#      |                     |
#     nic0                  nic1
#      |                     |
#    iface0 <-> router <-> iface1
#                  |
#               topology
#
#   <-> is a link
#   | is a subcomponent relationship
#



# Create the endpoints
endpoint0 = sst.Component("Endpoint0", "simpleElementExample.endpointComponent")
endpoint1 = sst.Component("Endpoint1", "simpleElementExample.endpointComponent")
#endpoint2 = sst.Component("Endpoint2", "simpleElementExample.endpointComponent")

# Give each endpoint a NIC
nic0 = endpoint0.setSubComponent("nic", "simpleElementExample.nicSubComponent")
nic1 = endpoint1.setSubComponent("nic", "simpleElementExample.nicSubComponent")
#nic2 = endpoint2.setSubComponent("nic", "simpleElementExample.nicSubComponent")

# Tell each NIC to use Merlin's SimpleNetwork interface
iface0 = nic0.setSubComponent("iface", "merlin.linkcontrol")
iface1 = nic1.setSubComponent("iface", "merlin.linkcontrol")
#iface2 = nic2.setSubComponent("iface", "merlin.linkcontrol")

# Use a merlin hr_router for this example
router = sst.Component("router", "merlin.hr_router")

# Tell hr_router what topology it is using
router.setSubComponent("topology", "merlin.singlerouter")

################
# Parameters
################

verb_params = { "verbose" : 1 }
net_params = {
    "input_buf_size" : "512B",
    "output_buf_size" : "512B",
    "link_bw" : "1GB/s",
}

# Make endpoints and NICs verbose
endpoint0.addParams(verb_params)
endpoint1.addParams(verb_params)
#endpoint2.addParams(verb_params)
nic0.addParams(verb_params)
nic1.addParams(verb_params)
#nic2.addParams(verb_params)

# Add network parameters to network components/subcomponents
iface0.addParams(net_params)
iface1.addParams(net_params)
#iface2.addParams(net_params)
router.addParams(net_params)

# Add other router parameters
router.addParams({
    "xbar_bw" : "1GB/s",
    "flit_size" : "32B",
    "num_ports" : "2",
    "id" : 0
})



################
# Links
################
# Connect the merlin interfaces to the merlin router
link0 = sst.Link("link0")
link0.connect( (iface0, "rtr_port", "1ms"), (router, "port0", "1ms") )

link1 = sst.Link("link1")
link1.connect( (iface1, "rtr_port", "1ms"), (router, "port1", "1ms") )

#link2 = sst.Link("link2")
#link2.connect( (iface2, "rtr_port", "1ms"), (router, "port2", "1ms") )
