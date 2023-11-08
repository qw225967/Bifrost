#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_INTERNET_APPS
    // Module headers: 
    #include <ns3/dhcp-helper.h>
    #include <ns3/ping-helper.h>
    #include <ns3/ping6-helper.h>
    #include <ns3/radvd-helper.h>
    #include <ns3/v4ping-helper.h>
    #include <ns3/v4traceroute-helper.h>
    #include <ns3/dhcp-client.h>
    #include <ns3/dhcp-header.h>
    #include <ns3/dhcp-server.h>
    #include <ns3/ping.h>
    #include <ns3/ping6.h>
    #include <ns3/radvd-interface.h>
    #include <ns3/radvd-prefix.h>
    #include <ns3/radvd.h>
    #include <ns3/v4ping.h>
    #include <ns3/v4traceroute.h>
#endif 