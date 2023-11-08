#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_DSR
    // Module headers: 
    #include <ns3/dsr-helper.h>
    #include <ns3/dsr-main-helper.h>
    #include <ns3/dsr-errorbuff.h>
    #include <ns3/dsr-fs-header.h>
    #include <ns3/dsr-gratuitous-reply-table.h>
    #include <ns3/dsr-maintain-buff.h>
    #include <ns3/dsr-network-queue.h>
    #include <ns3/dsr-option-header.h>
    #include <ns3/dsr-options.h>
    #include <ns3/dsr-passive-buff.h>
    #include <ns3/dsr-rcache.h>
    #include <ns3/dsr-routing.h>
    #include <ns3/dsr-rreq-table.h>
    #include <ns3/dsr-rsendbuff.h>
#endif 