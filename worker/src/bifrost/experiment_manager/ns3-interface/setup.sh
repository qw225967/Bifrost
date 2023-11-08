#!/bin/bash

echo "Cleaning up..."
docker kill bifrost-ns3 client server
docker system prune --force

echo "Building NS3 container..."
cd bifrost-ns3
docker build . -t qns
cd ..

echo "Configuring networks..."
docker network create leftnet --subnet 10.0.0.0/16
docker network create rightnet --subnet 10.100.0.0/16
docker create -it --privileged --net rightnet --ip 10.100.0.2 --name bifrost-ns3 qns sh
docker network connect leftnet bifrost-ns3 --ip 10.0.0.2

cd endpoint
echo "Building endpoint container..."
docker build . -t endpoint
echo "Running server and client container..."

# 配置两个端口容器的网络，并且暴露udp端口对外，传入两个端口的名字判断 nginx 参数输入
docker run -d --cap-add=NET_ADMIN --network leftnet --ip 10.0.0.100 -e GATEWAY="10.0.0.2" -p 8889:8889/udp -e PARAMS="client" --name client endpoint
docker run -d --cap-add=NET_ADMIN --network rightnet --ip 10.100.0.100 -e GATEWAY="10.100.0.2"  -p 8887:8887/udp -e PARAMS="server" --name server endpoint
cd ..

echo "Starting NS3 container"
docker start -i bifrost-ns3