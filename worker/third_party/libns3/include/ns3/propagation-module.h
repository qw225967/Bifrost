#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_PROPAGATION
    // Module headers: 
    #include <ns3/channel-condition-model.h>
    #include <ns3/cost231-propagation-loss-model.h>
    #include <ns3/itu-r-1411-los-propagation-loss-model.h>
    #include <ns3/itu-r-1411-nlos-over-rooftop-propagation-loss-model.h>
    #include <ns3/jakes-process.h>
    #include <ns3/jakes-propagation-loss-model.h>
    #include <ns3/kun-2600-mhz-propagation-loss-model.h>
    #include <ns3/okumura-hata-propagation-loss-model.h>
    #include <ns3/probabilistic-v2v-channel-condition-model.h>
    #include <ns3/propagation-cache.h>
    #include <ns3/propagation-delay-model.h>
    #include <ns3/propagation-environment.h>
    #include <ns3/propagation-loss-model.h>
    #include <ns3/three-gpp-propagation-loss-model.h>
    #include <ns3/three-gpp-v2v-propagation-loss-model.h>
#endif 