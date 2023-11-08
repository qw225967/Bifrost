#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_DSDV
    // Module headers: 
    #include <ns3/dsdv-helper.h>
    #include <ns3/dsdv-packet-queue.h>
    #include <ns3/dsdv-packet.h>
    #include <ns3/dsdv-routing-protocol.h>
    #include <ns3/dsdv-rtable.h>
#endif 