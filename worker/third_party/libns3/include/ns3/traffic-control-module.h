#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_TRAFFIC_CONTROL
    // Module headers: 
    #include <ns3/queue-disc-container.h>
    #include <ns3/traffic-control-helper.h>
    #include <ns3/cobalt-queue-disc.h>
    #include <ns3/codel-queue-disc.h>
    #include <ns3/fifo-queue-disc.h>
    #include <ns3/fq-cobalt-queue-disc.h>
    #include <ns3/fq-codel-queue-disc.h>
    #include <ns3/fq-pie-queue-disc.h>
    #include <ns3/mq-queue-disc.h>
    #include <ns3/packet-filter.h>
    #include <ns3/pfifo-fast-queue-disc.h>
    #include <ns3/pie-queue-disc.h>
    #include <ns3/prio-queue-disc.h>
    #include <ns3/queue-disc.h>
    #include <ns3/red-queue-disc.h>
    #include <ns3/tbf-queue-disc.h>
    #include <ns3/traffic-control-layer.h>
#endif 