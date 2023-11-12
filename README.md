# 狸猫通信程序服务端
一个可以进行即时通信的程序  
狸猫通信程序支持在不同平台不同设备之间即时交流，支持文字、图片、语音通话，
屏幕共享等功能。即使在同一平台，也有为不同需求设计的客户端，以达到最佳的使用体验。  

## 服务端
采用C++编写，具有效率高，内存占用低等优点。支持Linux平台，且可以较为容易的移植到其他平台。  
采用模块化开发，可以非常方便的为其添加额外的功能支持  
### 依赖
项目依赖于以下库：
- [RBS Lib](https://github.com/Reliable-Binary-Solutions-Studio/RBSlibs)
- [libuuid](https://github.com/Reliable-Binary-Solutions-Studio/RBSlibs)
- [libfmt](https://github.com/fmtlib/fmt)
- [sqlite3](https://www.sqlite.org/index.html)


### 编译
#### Linux
```shell
sudo apt install uuid-dev libsqlite3-dev libfmt-dev sqlite3 g++
g++ -std=c++17 -o server *.cpp -luuid -lfmt -lsqlite3 -O3
```

## 服务端开发进度
项目正在开发中，目前已经完成的功能如下：
1. 基本功能  
- [x] 登录与注册
- [x] UID获取用户名
- [x] 好友申请
- [x] 好友申请列表获取
- [x] 好友申请处理
- [x] 好友列表获取
- [ ] 好友删除
- [ ] 用户备注
- [x] 文本发送
- [x] 文本接收
- [x] 最近消息获取


*更多功能开发中*

## 客户端开发进度
*客户端开发准备中*
