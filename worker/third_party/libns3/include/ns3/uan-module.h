#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_UAN
    // Module headers: 
    #include <ns3/acoustic-modem-energy-model-helper.h>
    #include <ns3/uan-helper.h>
    #include <ns3/acoustic-modem-energy-model.h>
    #include <ns3/uan-channel.h>
    #include <ns3/uan-header-common.h>
    #include <ns3/uan-header-rc.h>
    #include <ns3/uan-mac-aloha.h>
    #include <ns3/uan-mac-cw.h>
    #include <ns3/uan-mac-rc-gw.h>
    #include <ns3/uan-mac-rc.h>
    #include <ns3/uan-mac.h>
    #include <ns3/uan-net-device.h>
    #include <ns3/uan-noise-model-default.h>
    #include <ns3/uan-noise-model.h>
    #include <ns3/uan-phy-dual.h>
    #include <ns3/uan-phy-gen.h>
    #include <ns3/uan-phy.h>
    #include <ns3/uan-prop-model-ideal.h>
    #include <ns3/uan-prop-model-thorp.h>
    #include <ns3/uan-prop-model.h>
    #include <ns3/uan-transducer-hd.h>
    #include <ns3/uan-transducer.h>
    #include <ns3/uan-tx-mode.h>
#endif 