#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_CSMA
    // Module headers: 
    #include <ns3/csma-helper.h>
    #include <ns3/backoff.h>
    #include <ns3/csma-channel.h>
    #include <ns3/csma-net-device.h>
#endif 