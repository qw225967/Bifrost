#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_MESH
    // Module headers: 
    #include <ns3/dot11s-installer.h>
    #include <ns3/flame-installer.h>
    #include <ns3/mesh-helper.h>
    #include <ns3/mesh-stack-installer.h>
    #include <ns3/dot11s-mac-header.h>
    #include <ns3/hwmp-protocol.h>
    #include <ns3/hwmp-rtable.h>
    #include <ns3/ie-dot11s-beacon-timing.h>
    #include <ns3/ie-dot11s-configuration.h>
    #include <ns3/ie-dot11s-id.h>
    #include <ns3/ie-dot11s-metric-report.h>
    #include <ns3/ie-dot11s-peer-management.h>
    #include <ns3/ie-dot11s-peering-protocol.h>
    #include <ns3/ie-dot11s-perr.h>
    #include <ns3/ie-dot11s-prep.h>
    #include <ns3/ie-dot11s-preq.h>
    #include <ns3/ie-dot11s-rann.h>
    #include <ns3/peer-link-frame.h>
    #include <ns3/peer-link.h>
    #include <ns3/peer-management-protocol.h>
    #include <ns3/flame-header.h>
    #include <ns3/flame-protocol-mac.h>
    #include <ns3/flame-protocol.h>
    #include <ns3/flame-rtable.h>
    #include <ns3/mesh-information-element-vector.h>
    #include <ns3/mesh-l2-routing-protocol.h>
    #include <ns3/mesh-point-device.h>
    #include <ns3/mesh-wifi-beacon.h>
    #include <ns3/mesh-wifi-interface-mac-plugin.h>
    #include <ns3/mesh-wifi-interface-mac.h>
#endif 