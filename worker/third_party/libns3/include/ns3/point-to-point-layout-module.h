#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_POINT_TO_POINT_LAYOUT
    // Module headers: 
    #include <ns3/point-to-point-dumbbell.h>
    #include <ns3/point-to-point-grid.h>
    #include <ns3/point-to-point-star.h>
#endif 