## 盘口k线还原

本项目使用深交所和上交所的原始逐笔委托和逐笔成交，撮合任意频率的K线和盘口，并写出到共享内存中。增加了对于不同频率的trade/order因子写出的支持。

## 项目结构

```shell
./
├── CMakeLists.txt
├── config.json                     # 配置参数文件，包括路径、端口等参数
├── docs                            # 项目文档
├── include                         # 头文件，包括外部库
│   ├── MatchEngineAPI.hpp          # API和SPI接口定义
│   ├── MatchEngineAPIData.hpp      # 数据类型定义
│   ├── MatchEngineAPIStruct.hpp    # 结构体定义
├── python                          # python代码，读取外部数据
│   ├── my_utils.py
├── README.md
├── demo                            # 示例代码
│   ├── Basic.hpp                   # demo中的Basic逻辑代码
│   └── MoneyFlow.cpp               # demo中的MoneyFlow逻辑代码
└──
```

## 编译过程

1. 需要用户从源码编译安装ZMQ(libzmq,cppzmq)，pybind11两个外部库。

2. 编译项目
```shell
mkdir build
cd build
cmake ..
make
```

## 调用方法

1. 直接调用可执行文件：先调用demo，然后调用offline_data，不输入日期默认运行调用当天的撮合，可以通过位置输入两个参数：
```shell
./offline_data [date] [incre_port]
./demo [date] [incre_port]
```
- date: 日期，格式为%Y-%m-%d，不输入默认调用当天。
- incre_port: 端口偏移量，用于多进程同时运行脚本。

## 因子开发说明

框架整体基于事件驱动的回调机制，提供用户进行逐笔的trade/order因子开发的功能。用户需基于MatchEngineAPI.hpp提供的接口名称，实现自己的因子回调类。具体开发与调用逻辑如下：

1. 创建一个接收回报的类，该类继承自MatchEngineSPI，用来接收撮合引擎过程中接收到的逐笔委托和成交。
2. 调用createMatchAPI接口，创建一个API对象，该对象用来启动撮合和和查询盘口。
3. 调用registerSpi接口，将回报接收类对象传进请求类对象中。
4. 调用startMatch接口进行撮合。
5. 每个进程会有其独立的spi对象接口，每个spi只会处理一部分股票数据，在处理数据和进行ob的时候需要注意。

在demo/BaseFactorGenerator.cpp中提供了demo，可以参考进行开发。

## 开发细节说明

1. 上交所的逐笔委托是成交后的剩余量，因此如果要对逐笔委托进行统计，必须要将上交所委托进行还原。框架会将每一笔上交所的trade发送一笔主动方向的order，只调整UnifiedRecord中的applseqnum字段，其余字段都不做调整。受限于交易所的发送逻辑，还原操作放在SPI处进行，demo中已给出还原的代码。
2. SPI中的两个onFactorOB函数必须实现，分别对应ob一只票和批量ob的场景，两者的逻辑须保持一致。可以考虑在批量ob的函数中循环调用ob一只票的函数。

## 开发格式约定

1. 按照因子逻辑分类组织代码。demo中给出了两类逻辑的分类示例，具体到代码中并非对应的逻辑，仅作为分类示例。具体因子逻辑分类名称请与负责人讨论后确定。
2. 避免复杂的因子逻辑组织。每一个逻辑分类代码，包括了这个逻辑分类下需要的所有内容，包括自定义的结构体、函数等，不要进行复杂的代码逻辑组织，不要新建额外的cpp或者hpp文件，保证因子代码的独立性，尽量维持代码结构的扁平，因子计算逻辑的清晰。
3. 保证代码中与文档中的因子名称对应。每个因子通过成员类属性在代码中保存，名称要与文档中写的因子变量名称保持一致。
