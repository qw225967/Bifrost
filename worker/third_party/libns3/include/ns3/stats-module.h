#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_STATS
    // Module headers: 
    #include <ns3/file-helper.h>
    #include <ns3/gnuplot-helper.h>
    #include <ns3/average.h>
    #include <ns3/basic-data-calculators.h>
    #include <ns3/boolean-probe.h>
    #include <ns3/data-calculator.h>
    #include <ns3/data-collection-object.h>
    #include <ns3/data-collector.h>
    #include <ns3/data-output-interface.h>
    #include <ns3/double-probe.h>
    #include <ns3/file-aggregator.h>
    #include <ns3/get-wildcard-matches.h>
    #include <ns3/gnuplot-aggregator.h>
    #include <ns3/gnuplot.h>
    #include <ns3/histogram.h>
    #include <ns3/omnet-data-output.h>
    #include <ns3/probe.h>
    #include <ns3/stats.h>
    #include <ns3/time-data-calculators.h>
    #include <ns3/time-probe.h>
    #include <ns3/time-series-adaptor.h>
    #include <ns3/uinteger-16-probe.h>
    #include <ns3/uinteger-32-probe.h>
    #include <ns3/uinteger-8-probe.h>
#endif 