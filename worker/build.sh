#!/bin/bash

# 移除旧编译内容
rm -rf ./third_party/libuv/build/*
rm -rf ./build/*

# 1.编译libuv
cd ./third_party/libuv/build/ && cmake .. -DBUILD_TESTING=ON
cd .. && cmake --build build

# 2.编译worker
cd  cd ../../build/ && cmake ..
make