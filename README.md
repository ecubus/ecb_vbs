# VBS WebSocket适配器

## 项目简介

VBS WebSocket适配器是[haloos/vbs](https://gitee.com/haloos/vbs)的一个app项目，提供了通过WebSocket协议与MVBS (Motor Vehicle Bus System) 通信的能力。本项目允许客户端通过WebSocket连接读取和写入MVBS总线上的数据，获取统计信息，以及查询在线节点。

通过VBS WebSocket适配器，开发者可以采用半动态的方式快速调试vbs协议，无需为每次调试都编写专门的测试代码。传统vbs应用开发需要针对每种数据类型和测试场景编写C代码并重新编译整个应用，而本适配器提供了一种更灵活的方式，允许通过WebSocket接口动态构造和发送任意类型的数据。

这种设计显著减少了开发过程中的编译次数和时间消耗。尽管在IDL（接口定义语言）文件或XML配置文件变更后仍需要重新编译，但对于大多数调试和测试场景，开发者可以直接通过修改JSON数据完成，避免了频繁的编译-调试循环，极大提升了开发效率。

适配器特别适合以下场景：
- 快速验证新定义的vbs消息格式
- 调试现有vbs系统中的通信问题
- 开发基于Web技术的vbs监控和调试工具
- 进行跨平台的vbs协议测试

## 功能特性

- 基于WebSocket的JSON-RPC风格API
- 支持读写MVBS总线数据
- 支持获取Reader/Writer统计信息
- 支持获取Reader/Writer在线节点信息
- 实时数据通知机制

## 编译说明

> [!NOTE]
> 在编译本代码前，确保vbs的相关代码(examples)可以成功编译，参考[VBSLite快速开始](https://gitee.com/haloos/vbs/blob/master/developer-guide/quick_start.md)。

将本项目克隆或复制到vbs根目录的app目录下，然后需要应用以下补丁文件：

1. `app/patchs/build.patch` - 增加对C++文件的编译支持
2. `app/patchs/mvbs.patch` - 修复C++编译时的类型转换问题
3. `app/patchs/tools.patch` - 增加对C++模板生成的支持

### 应用补丁命令：

```bash
cd vbs根目录
cd build
git apply app/patchs/build.patch
cd ..
cd mvbs
git apply app/patchs/mvbs.patch
cd ..
cd tools
git apply app/patchs/tools.patch
cd ..
```
### 编译

在项目根目录下执行以下命令：

```bash
make ecb_vbs -j16
```


## 运行

启动服务器：

```bash
./app 20101
```

然后可以使用WebSocket客户端连接到 `ws://localhost:20101` 发送JSON请求。


## 使用的第三方库

本项目使用了以下开源库：

1. **WebSocket++**
   - 描述: C++实现的WebSocket服务器和客户端库
   - 项目地址: https://github.com/zaphoyd/websocketpp
   - 许可证: BSD许可证

2. **nlohmann/json**
   - 描述: 现代C++ JSON处理库
   - 项目地址: https://github.com/nlohmann/json
   - 许可证: MIT许可证

3. **struct_json**
   - 描述: C++结构体与JSON转换库
   - 项目地址: https://github.com/alibaba/yalantinglibs
   - 许可证: Apache 2.0许可证

## WebSocket通信格式

### 请求格式

所有请求都使用JSON格式，具有以下通用结构：

```json
{
  "id": "请求ID",
  "method": "方法名称",
  "topic_name": "主题名称",
  "type_name": "数据类型名称",
  "data": {} // 可选，用于写入操作
}
```

### 响应格式

所有响应都使用JSON格式，具有以下通用结构：

```json
{
  "id": "请求ID",
  "method": "方法名称",
  "ok": true/false,
  "message": "可选的消息",
  "data": {} // 可选，根据请求返回相应数据
}
```

### 支持的方法

#### 1. 写入数据 (write)

向指定主题写入数据。

**请求示例：**
```json
{
  "id": "1",
  "method": "write",
  "topic_name": "example_topic",
  "type_name": "ExampleType",
  "data": {
    "field1": "value1",
    "field2": 123
  }
}
```

**响应示例：**
```json
{
  "id": "1",
  "method": "write",
  "ok": true,
  "message": "Data written successfully",
  "writesn": {
    "high": 0,
    "low": 1
  }
}
```

#### 2. 读取数据 (read)

从指定主题读取数据。

**请求示例：**
```json
{
  "id": "2",
  "method": "read",
  "topic_name": "example_topic",
  "type_name": "ExampleType"
}
```

**响应示例：**
```json
{
  "id": "2",
  "method": "read",
  "ok": true,
  "data": {
    "field1": "value1",
    "field2": 123
  },
  "sample_info": {
    "valid_data": true,
    "sample_state": 1,
    "reception_timestamp": 12345678
  }
}
```

#### 3. 获取Reader统计信息 (get_reader_stats)

获取指定主题Reader的统计信息。

**请求示例：**
```json
{
  "id": "3",
  "method": "get_reader_stats",
  "topic_name": "example_topic"
}
```

**响应示例：**
```json
{
  "id": "3",
  "method": "get_reader_stats",
  "ok": true,
  "data": {
    "samples_received": 100,
    "samples_lost": 0,
    "bytes_received": 1024
  }
}
```

#### 4. 获取Writer统计信息 (get_writer_stats)

获取指定主题Writer的统计信息。

**请求示例：**
```json
{
  "id": "4",
  "method": "get_writer_stats",
  "topic_name": "example_topic"
}
```

**响应示例：**
```json
{
  "id": "4",
  "method": "get_writer_stats",
  "ok": true,
  "data": {
    "samples_sent": 100,
    "bytes_sent": 1024
  }
}
```

#### 5. 获取Writer在线节点 (get_writer_peers)

获取指定主题Writer的在线节点列表。

**请求示例：**
```json
{
  "id": "5",
  "method": "get_writer_peers",
  "topic_name": "example_topic"
}
```

**响应示例：**
```json
{
  "id": "5",
  "method": "get_writer_peers",
  "data": [
    {
      "guid": "00000001.00000002.00000003.00000004",
      "ip": "192.168.1.100",
      "port": 7400
    }
  ]
}
```

#### 6. 获取Reader在线节点 (get_reader_peers)

获取指定主题Reader的在线节点列表。

**请求示例：**
```json
{
  "id": "6",
  "method": "get_reader_peers",
  "topic_name": "example_topic"
}
```

**响应示例：**
```json
{
  "id": "6",
  "method": "get_reader_peers",
  "data": [
    {
      "guid": "00000001.00000002.00000003.00000004",
      "ip": "192.168.1.100",
      "port": 7400
    }
  ]
}
```

### 通知消息

当订阅的主题有新数据到达时，服务器会主动发送通知消息：

```json
{
  "notify": "rx_indication",
  "topic_name": "example_topic",
  "type_name": "ExampleType",
  "ok": true,
  "data": {
    "field1": "value1",
    "field2": 123
  },
  "sample_info": {
    "valid_data": true,
    "sample_state": 1,
    "reception_timestamp": 12345678
  }
}
```

