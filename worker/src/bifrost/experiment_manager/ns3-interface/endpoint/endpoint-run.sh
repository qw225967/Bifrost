#!/bin/bash

# ns3 默认没有开启udp校验和，需要打开网卡校验和
ethtool -K eth0 tx off

route del default
route add default gw $GATEWAY

# 修改 nginx 配置，设置 in 代理到对端，out 代理到物理机中的IP(在bifrost的ns3代码中已写有)
echo "stream {
          upstream udp_in {
              server 10.100.0.100:8889;
          }
          server {
              listen 10.0.0.100:8889 udp;
              proxy_responses 1;
              proxy_timeout 20s;
              proxy_pass udp_in;
              error_log /var/log/nginx/dns.log;
          }
          upstream udp_out {
              server 10.0.0.2:9999;
          }
          server {
              listen 10.100.0.100:8887 udp;
              proxy_responses 1;
              proxy_timeout 20s;
              proxy_pass udp_out;
              error_log /var/log/nginx/dns.log;
          }
      }" >> /etc/nginx/nginx.conf

sleep 3600