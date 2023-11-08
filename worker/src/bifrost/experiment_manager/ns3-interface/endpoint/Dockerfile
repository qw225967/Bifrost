FROM ubuntu:16.04

RUN apt-get update && \
  apt-get install -y net-tools iputils-ping tcpdump traceroute nginx vim gcc netcat nmap iperf ethtool iperf3

COPY endpoint-run.sh .
RUN chmod +x endpoint-run.sh
ENTRYPOINT ./endpoint-run.sh