#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_TOPOLOGY_READ
    // Module headers: 
    #include <ns3/topology-reader-helper.h>
    #include <ns3/inet-topology-reader.h>
    #include <ns3/orbis-topology-reader.h>
    #include <ns3/rocketfuel-topology-reader.h>
    #include <ns3/topology-reader.h>
#endif 