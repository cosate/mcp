# mcp
* cpp mcp streamable http protocol
* 60%的代码是调教AI写出来的，自己检查再改改。AI太强大了orz
* c++17标准

## 模块说明
- common  
  - types.h, 核心是开头的一些宏定义, 解决std::optional类型的序列化和反序列化  
  - 结构体定义参考python sdk
- client  
  - 已编译测试, 需要自行解决libcurl依赖
- server  
  - nginx http模块，需要编入nginx后启动  
  - 自行下载nginx 源码，修改build.sh路径执行  
  - 功能包括:
    - 解析mcp request
    - 异步响应
    - 按mcp method限流
- third_party
  - 依赖三方库: nlohmann/json, spdlog

## TODO
- 实现mcp tools等能力
