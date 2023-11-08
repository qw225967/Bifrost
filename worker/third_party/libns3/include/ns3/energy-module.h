#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_ENERGY
    // Module headers: 
    #include <ns3/basic-energy-harvester-helper.h>
    #include <ns3/basic-energy-source-helper.h>
    #include <ns3/energy-harvester-container.h>
    #include <ns3/energy-harvester-helper.h>
    #include <ns3/energy-model-helper.h>
    #include <ns3/energy-source-container.h>
    #include <ns3/li-ion-energy-source-helper.h>
    #include <ns3/rv-battery-model-helper.h>
    #include <ns3/basic-energy-harvester.h>
    #include <ns3/basic-energy-source.h>
    #include <ns3/device-energy-model-container.h>
    #include <ns3/device-energy-model.h>
    #include <ns3/energy-harvester.h>
    #include <ns3/energy-source.h>
    #include <ns3/li-ion-energy-source.h>
    #include <ns3/rv-battery-model.h>
    #include <ns3/simple-device-energy-model.h>
#endif 