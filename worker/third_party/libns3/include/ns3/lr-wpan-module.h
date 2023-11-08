#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_LR_WPAN
    // Module headers: 
    #include <ns3/lr-wpan-helper.h>
    #include <ns3/lr-wpan-constants.h>
    #include <ns3/lr-wpan-csmaca.h>
    #include <ns3/lr-wpan-error-model.h>
    #include <ns3/lr-wpan-fields.h>
    #include <ns3/lr-wpan-interference-helper.h>
    #include <ns3/lr-wpan-lqi-tag.h>
    #include <ns3/lr-wpan-mac-header.h>
    #include <ns3/lr-wpan-mac-pl-headers.h>
    #include <ns3/lr-wpan-mac-trailer.h>
    #include <ns3/lr-wpan-mac.h>
    #include <ns3/lr-wpan-net-device.h>
    #include <ns3/lr-wpan-phy.h>
    #include <ns3/lr-wpan-spectrum-signal-parameters.h>
    #include <ns3/lr-wpan-spectrum-value-helper.h>
#endif 