#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_AODV
    // Module headers: 
    #include <ns3/aodv-helper.h>
    #include <ns3/aodv-dpd.h>
    #include <ns3/aodv-id-cache.h>
    #include <ns3/aodv-neighbor.h>
    #include <ns3/aodv-packet.h>
    #include <ns3/aodv-routing-protocol.h>
    #include <ns3/aodv-rqueue.h>
    #include <ns3/aodv-rtable.h>
#endif 