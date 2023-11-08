#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_SIXLOWPAN
    // Module headers: 
    #include <ns3/sixlowpan-helper.h>
    #include <ns3/sixlowpan-header.h>
    #include <ns3/sixlowpan-net-device.h>
#endif 