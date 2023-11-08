#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_ANTENNA
    // Module headers: 
    #include <ns3/angles.h>
    #include <ns3/antenna-model.h>
    #include <ns3/cosine-antenna-model.h>
    #include <ns3/isotropic-antenna-model.h>
    #include <ns3/parabolic-antenna-model.h>
    #include <ns3/phased-array-model.h>
    #include <ns3/three-gpp-antenna-model.h>
    #include <ns3/uniform-planar-array.h>
#endif 