# 数据库的使用

数据库默认使用SQLite完成操作,以下是数据库中各表功能
## 数据库各个表含义
### sessions
表名：sessions  
存储所有的会话ID，格式为  

会话ID 会话所有者 会话管理员 会话成员（不含管理员）
其中管理员和成员为字符串类型，多个成员UID之间由空格分割

### friends_sessions
存储用户的好友对应的会话列表  
表名: uuid_friends_sessions  
格式: 好友的UUID 对应的会话
