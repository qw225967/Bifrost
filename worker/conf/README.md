使用指南：
    Bifrost需要使用到两台终端进行端对端测试，如果只用一台机器是无法走网络进行弱网限制的。
Instructions for use:

Bifrost requires the use of two terminals for end-to-end testing, and if only one machine is used, weak network restrictions cannot be achieved through the network.

推流的机器请将配置按下方的方式设置。

```

    # 推流配置：
    #push configuration
    
    {   
        # 本机的配置，这是用于绑定当前机器接收的ip和port的。
        # The configuration of this machine is used to bind the IP and port received by the current machine.

        "LocalReceiveConfigs": {
            "userName": "local_client_1",
            # 推流本机的端口
            "rtcPort": 9001,
             # 推流本机监听所有ip的数据
            "rtcIp": "0.0.0.0",
        },

        # 远端的配置，这是用于绑定对端机器接收的ip和port的，ssrc标记每个端发送独立的——也就是所有推流端都唯一标识。
        # 例如：本机有一个推流端，那它的ssrc都会不一样用于区别rtc测试的逻辑
        # The configuration of the remote end is used to bind the IP and port received by the opposite machine, with SSRC marking that each end sends independently - that is, all streaming ends are uniquely identified.
        # For example, if the local machine has a streaming end, its SSRC will be different for distinguishing the logic of RTC testing        

        "RemoteSendConfigs": {
            "userName": "remote_client_1",
            # 你想要推的目标端口
            "rtcPort": 9012,
            # 你想要推的目标ip
            "rtcIp": "101.42.42.53",
            "ssrc": 12341234
        },
        
        # 实验配置，目前未启用
        "ExperimentConfig": {
            "NackExperiment": {
            },
            "FecExperiment": {
            },
            "GccExperiment": {
                "TrendLineWindowSize": 80,
                "TrendLineThreshold": 0
            }
        }
    }


```


```
    # 拉流配置：
    #play configuration
    
    {   
        # 本机的配置，这是用于绑定当前机器接收的ip和port的。（推流端要明确拉流端的ip和port配置到remote中）
        # The configuration of this machine is used to bind the IP and port received by the current machine. (The push end should clearly configure the IP and port of the pull end in the remote)

        "LocalReceiveConfigs": {
            "userName": "local_client_1",
            # 本机的端口，这里要和对端的远端 端口 对上
            "rtcPort": 9012,
            # 本机的ip
            "rtcIp": "0.0.0.0",
        },

        # 实验配置，目前未启用
        #Experimental configuration, currently unabled

        "ExperimentConfig": {
            "NackExperiment": {
            },
            "FecExperiment": {
            },
            "GccExperiment": {
                "TrendLineWindowSize": 80,
                "TrendLineThreshold": 0
            }
        }
    }

```

play端可以切 Bifrost-player 分支，该分支只初始化了一个play能力。
The play end can switch to the Bifrost player branch, which only initializes one play capability.

如果你需要使用NS3模拟的能力，请参考：worker/src/bifrost/experiment_manager/ns3-interface/README.md