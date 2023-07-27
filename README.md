# Bifrost

Bifrost is an experimental system that integrates multiple Qos methods, and use to conduct point-to-point/multi-point weak network confrontation experiments.
For the convenience of operation, we have simplified the difficulty of parameter 
adjustment and provided detailed information for each link. This project will include common bandwidth estimation 
algorithms, compensation methods such as FEC, Nack, and Red in WebRTC, and provide more room for expansion for basic 
multi-stream competition experiments.

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
cd Bifrost/worker

mkdir -p build

cd build && cmake ..

make
```

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

### 2.Show
The figure below shows the trend of the slope in the trend line module of the GCC algorithm during the transmission 
process.
![img_1.png](draw/img_1.png)

The figure below shows the target code rate and sending code rate of the GCC algorithm during transmission.
![img.png](draw/img.png)
