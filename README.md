# mcp
cpp mcp streamable http protocol

# common
* types.h 核心内容是开头的一些宏定义，解决了optional类型的序列化和反序列化的问题
* 结构体定义都参照python sdk的定义

# client
* 已经编译测试过了，需要自己解决libcurl的依赖

# server
* 是一个nginx模块，需要编进nginx启动，还需优化
* 自己下载nginx源码，修改build.sh中相应路径，执行
