#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_MOBILITY
    // Module headers: 
    #include <ns3/group-mobility-helper.h>
    #include <ns3/mobility-helper.h>
    #include <ns3/ns2-mobility-helper.h>
    #include <ns3/box.h>
    #include <ns3/constant-acceleration-mobility-model.h>
    #include <ns3/constant-position-mobility-model.h>
    #include <ns3/constant-velocity-helper.h>
    #include <ns3/constant-velocity-mobility-model.h>
    #include <ns3/gauss-markov-mobility-model.h>
    #include <ns3/geographic-positions.h>
    #include <ns3/hierarchical-mobility-model.h>
    #include <ns3/mobility-model.h>
    #include <ns3/position-allocator.h>
    #include <ns3/random-direction-2d-mobility-model.h>
    #include <ns3/random-walk-2d-mobility-model.h>
    #include <ns3/random-waypoint-mobility-model.h>
    #include <ns3/rectangle.h>
    #include <ns3/steady-state-random-waypoint-mobility-model.h>
    #include <ns3/waypoint-mobility-model.h>
    #include <ns3/waypoint.h>
#endif 