#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_FLOW_MONITOR
    // Module headers: 
    #include <ns3/flow-monitor-helper.h>
    #include <ns3/flow-classifier.h>
    #include <ns3/flow-monitor.h>
    #include <ns3/flow-probe.h>
    #include <ns3/ipv4-flow-classifier.h>
    #include <ns3/ipv4-flow-probe.h>
    #include <ns3/ipv6-flow-classifier.h>
    #include <ns3/ipv6-flow-probe.h>
#endif 