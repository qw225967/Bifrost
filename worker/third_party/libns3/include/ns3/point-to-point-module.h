#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_POINT_TO_POINT
    // Module headers: 
    #include <ns3/point-to-point-helper.h>
    #include <ns3/point-to-point-channel.h>
    #include <ns3/point-to-point-net-device.h>
    #include <ns3/ppp-header.h>
#endif 