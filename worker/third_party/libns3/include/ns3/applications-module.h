#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_APPLICATIONS
    // Module headers: 
    #include <ns3/bulk-send-helper.h>
    #include <ns3/on-off-helper.h>
    #include <ns3/packet-sink-helper.h>
    #include <ns3/three-gpp-http-helper.h>
    #include <ns3/udp-client-server-helper.h>
    #include <ns3/udp-echo-helper.h>
    #include <ns3/application-packet-probe.h>
    #include <ns3/bulk-send-application.h>
    #include <ns3/onoff-application.h>
    #include <ns3/packet-loss-counter.h>
    #include <ns3/packet-sink.h>
    #include <ns3/seq-ts-echo-header.h>
    #include <ns3/seq-ts-header.h>
    #include <ns3/seq-ts-size-header.h>
    #include <ns3/three-gpp-http-client.h>
    #include <ns3/three-gpp-http-header.h>
    #include <ns3/three-gpp-http-server.h>
    #include <ns3/three-gpp-http-variables.h>
    #include <ns3/udp-client.h>
    #include <ns3/udp-echo-client.h>
    #include <ns3/udp-echo-server.h>
    #include <ns3/udp-server.h>
    #include <ns3/udp-trace-client.h>
#endif 