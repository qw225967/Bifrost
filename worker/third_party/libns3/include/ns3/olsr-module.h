#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_OLSR
    // Module headers: 
    #include <ns3/olsr-helper.h>
    #include <ns3/olsr-header.h>
    #include <ns3/olsr-repositories.h>
    #include <ns3/olsr-routing-protocol.h>
    #include <ns3/olsr-state.h>
#endif 