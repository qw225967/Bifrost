#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_NIX_VECTOR_ROUTING
    // Module headers: 
    #include <ns3/nix-vector-helper.h>
    #include <ns3/nix-vector-routing.h>
#endif 