# Bifrost

Bifrost is an experimental system that integrates multiple Qos methods, and use to conduct point-to-point/multi-point weak network confrontation experiments.
For the convenience of operation, we have simplified the difficulty of parameter 
adjustment and provided detailed information for each link. This project will include common bandwidth estimation 
algorithms, compensation methods such as FEC, Nack, and Red in WebRTC, and provide more room for expansion for basic 
multi-stream competition experiments.It also supports the use of NS3 as a proxy for network simulation, and supports interaction with NS3 simulation networks in physical networks to achieve algorithm validation.
If you want to learn more about NS3 related content, please check it out:
***worker/src/bifrost/experiment_manager/ns3-interface/README.md***

## Install

***Just support MacOS/Linux now!***

***Clion idea is recommended.***

### 1.Clone code
```
git clone https://github.com/qw225967/Bifrost.git
```

### 2.Libuv static library build
Need add the libuv.a to CMakeLists.txt — target_link_libraries(worker libuv.a)

```
cd Bifrost/worker/third_party/libuv/

mkdir -p build

# generate project with tests
cd build && cmake .. -DBUILD_TESTING=ON

cd ..

# add `-j <n>` with cmake >= 3.12
cmake --build build
```

### 3.Build
#### 3.1 MacOS
Clion must use lldb. 

#### 3.2 Linux
You can use the ***Bifrost/worker/build.sh*** or build anything by yourself.

```
sh build.sh
```
or
```
cd Bifrost/worker

mkdir -p build

cd build && cmake ..

make
```

## Run
Bifrost requires the use of two terminals for end-to-end testing, and if only one machine is used, weak network restrictions cannot be achieved through the network.

Please refer to **worker/conf/README.md** for usage and Configuration

## Display

This demo uses grafana-based data display, using the CSV file path as the data source.

### 1.Mac uses grafana

Mac install grafana

```
brew update

brew install grafana
```

grafana service operate

```
# start grafana service
brew services start grafana

# stop grafana service
brew services stop grafana

# restart grafana service
brew services restart grafana
```

Add this option to **grafana/grafana.ini**
Use ***ps -ef | grep grafana*** command to view the default startup configuration

```
[plugin.marcusolsson-csv-datasource]
allow_local_mode = true
```

Please refer to **worker/conf/README.md** for usage


### 2.Show
The figure below shows the trend of the slope in the trend line module of the GCC algorithm during the transmission 
process.
![img_1.png](draw/img_1.png)

The figure below shows the target code rate and sending code rate of the GCC algorithm during transmission.
![img.png](draw/img.png)

## Expand

This project also provides the ability to use NS3 simulation for network simulation.
It provides the ability to convert physical and virtual networks, and provides an interface for developing network topology.

For details, please refer to **worker/src/bifrost/experiment_manager/ns3-interface/README.md**.