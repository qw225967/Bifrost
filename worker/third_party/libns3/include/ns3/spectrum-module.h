#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_SPECTRUM
    // Module headers: 
    #include <ns3/adhoc-aloha-noack-ideal-phy-helper.h>
    #include <ns3/spectrum-analyzer-helper.h>
    #include <ns3/spectrum-helper.h>
    #include <ns3/tv-spectrum-transmitter-helper.h>
    #include <ns3/waveform-generator-helper.h>
    #include <ns3/aloha-noack-mac-header.h>
    #include <ns3/aloha-noack-net-device.h>
    #include <ns3/constant-spectrum-propagation-loss.h>
    #include <ns3/friis-spectrum-propagation-loss.h>
    #include <ns3/half-duplex-ideal-phy-signal-parameters.h>
    #include <ns3/half-duplex-ideal-phy.h>
    #include <ns3/ism-spectrum-value-helper.h>
    #include <ns3/matrix-based-channel-model.h>
    #include <ns3/microwave-oven-spectrum-value-helper.h>
    #include <ns3/two-ray-spectrum-propagation-loss-model.h>
    #include <ns3/multi-model-spectrum-channel.h>
    #include <ns3/non-communicating-net-device.h>
    #include <ns3/single-model-spectrum-channel.h>
    #include <ns3/spectrum-analyzer.h>
    #include <ns3/spectrum-channel.h>
    #include <ns3/spectrum-converter.h>
    #include <ns3/spectrum-error-model.h>
    #include <ns3/spectrum-interference.h>
    #include <ns3/spectrum-model-300kHz-300GHz-log.h>
    #include <ns3/spectrum-model-ism2400MHz-res1MHz.h>
    #include <ns3/spectrum-model.h>
    #include <ns3/spectrum-phy.h>
    #include <ns3/spectrum-propagation-loss-model.h>
    #include <ns3/spectrum-transmit-filter.h>
    #include <ns3/phased-array-spectrum-propagation-loss-model.h>
    #include <ns3/spectrum-signal-parameters.h>
    #include <ns3/spectrum-value.h>
    #include <ns3/three-gpp-channel-model.h>
    #include <ns3/three-gpp-spectrum-propagation-loss-model.h>
    #include <ns3/trace-fading-loss-model.h>
    #include <ns3/tv-spectrum-transmitter.h>
    #include <ns3/waveform-generator.h>
    #include <ns3/wifi-spectrum-value-helper.h>
    #include <ns3/spectrum-test.h>
#endif 