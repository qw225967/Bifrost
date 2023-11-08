#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_BRIDGE
    // Module headers: 
    #include <ns3/bridge-helper.h>
    #include <ns3/bridge-channel.h>
    #include <ns3/bridge-net-device.h>
#endif 