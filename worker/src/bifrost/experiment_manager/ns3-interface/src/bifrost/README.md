# Bifrost ns3 interface

This is an interface tool that supports the interaction of physical network and NS3(ns3.29) simulated network data.

## Important

**NS3 Interface is not support MacOS**

## Framework

***1.The sender use `TapBridge` to receive physical-link.***

***2.The receiver use `` to send to virtual-link.***

```
      
                                    +-----------------------------------------+
      +-----------------------+     |           |                 |           |     +-----------------------+                     
      |      client eth0      |     |           |     +-----+     |           |     |      server eth1      |
      |                       |     |   local   |     |     |     |   local   |     |                       |
      +                       + ——— +           + ——— | ns3 + ——— +           + ——— +                       +
      |       Sender Ip       |     |   range   |     |     |     |   range   |     |      Reciever Ip      |
      |       Sender Port     |     |           |     +-----+     |           |     |      Reciever port    |
      +----------+------------+     |           |                 |           |     |                       |
                                    +-----------------------------------------+     +-----------------------+
```

