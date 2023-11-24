#!/bin/bash

# ns3 默认没有开启udp校验和，需要打开网卡校验和
ethtool -K eth0 tx off

route del default
route add default gw $GATEWAY

udp_in_server=""
udp_in_listen=""
udp_out_server=""
udp_out_listen=""

if [ $1 = "client" ];then
  # 配置输入的流向
  udp_in_server="10.100.0.100:8889"
  udp_in_listen="0.0.0.0:8889"
  # 配置输出的流向
  udp_out_server="10.0.0.1:9098"
  udp_out_listen="0.0.0.0:8887"
elif [ $1 = "server" ]; then
  # 配置输入的流向
  udp_in_server="10.0.0.100:8887"
  udp_in_listen="0.0.0.0:8887"
  # 配置输出的流向
  udp_out_server="10.100.0.1:7777"
  udp_out_listen="0.0.0.0:8889"
else
  echo "endpoint name err!"
fi


# 修改 nginx 配置，设置 in 代理到对端，out 代理到物理机中的IP(在bifrost的ns3代码中已写有)
echo "stream {
          upstream udp_in {
              server $udp_in_server;
          }
          server {
              listen $udp_in_listen udp;
              proxy_responses 1;
              proxy_timeout 2s;
              proxy_pass udp_in;
              error_log /var/log/nginx/dns.log;
          }
          upstream udp_out {
              server $udp_out_server;
          }
          server {
              listen $udp_out_listen udp;
              proxy_responses 1;
              proxy_timeout 2s;
              proxy_pass udp_out;
              error_log /var/log/nginx/dns.log;
          }
      }" >> /etc/nginx/nginx.conf

sed -i "s/768/10240/g" /etc/nginx/nginx.conf

nginx

sleep 3600
