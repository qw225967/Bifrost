#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_FD_NET_DEVICE
    // Module headers: 
    #include <ns3/tap-fd-net-device-helper.h>
    #include <ns3/emu-fd-net-device-helper.h>
    #include <ns3/fd-net-device.h>
    #include <ns3/fd-net-device-helper.h>
#endif 