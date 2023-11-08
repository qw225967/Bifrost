#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_BUILDINGS
    // Module headers: 
    #include <ns3/building-allocator.h>
    #include <ns3/building-container.h>
    #include <ns3/building-position-allocator.h>
    #include <ns3/buildings-helper.h>
    #include <ns3/building-list.h>
    #include <ns3/building.h>
    #include <ns3/buildings-channel-condition-model.h>
    #include <ns3/buildings-propagation-loss-model.h>
    #include <ns3/hybrid-buildings-propagation-loss-model.h>
    #include <ns3/itu-r-1238-propagation-loss-model.h>
    #include <ns3/mobility-building-info.h>
    #include <ns3/oh-buildings-propagation-loss-model.h>
    #include <ns3/random-walk-2d-outdoor-mobility-model.h>
    #include <ns3/three-gpp-v2v-channel-condition-model.h>
#endif 