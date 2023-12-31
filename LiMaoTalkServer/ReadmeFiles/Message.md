﻿# 通信协议
## 一、主消息通信协议

1. 主消息通信使用TCP协议作为基础协议，
协议头部为4字节的消息长度，消息长度不包含头部长度。  
2. 消息体为json格式，消息体中包含消息类型和消息内容。

## 二、消息类型

基本消息类型均为JSON格式，消息体中应至少包含`ID`和`State`字段  
以下是具体消息的定义  

|ID|功能|  
|--|----|  
|[0](#连通性检测)|连通性检测|
|[100](#登录请求)|登录请求|
|[101](#注册请求)|注册请求|
|[102](#好友申请)|好友申请|
|[103](#请求好友申请列表)|请求好友申请列表|
|[104](#同意或忽略好友申请)|同意或忽略好友申请|
|[105](#UID获取用户名)|UID获取用户名|
|[106](#获取好友列表)|获取好友列表|
|[107](#设置在线状态)|设置在线状态|
|[201](#向好友发送消息)|向好友发送消息|
|[202](#收到消息)|收到好友或群聊的消息|
|[203](#获取消息)|获取某个好友或群组的指定范围消息|


## 三、消息内容

### 连通性检测
#### ID 0  
#### 请求样例
```json
{
	"ID": 0,
	"State": 0
}
```
#### 返回样例
与请求样例相同
#### 功能说明
用于测试客户端与服务器连通性，客户端发送此消息后，服务器应返回相同的消息。

### 登录请求
#### ID 100
#### 请求样例
```JSON
{
	"ID": 100,
	"State": 0,
	"Data": {
		"UID": "10000",
		"PassWord": "test"
	}
}
```
#### 返回样例
```JSON
{
	"ID": 100,
	"State": 0,
	"Message":"Success",
	"Data": {
		"Token": "test"
	}
}
```
#### 字段说明
1. 请求字段  
`UID` 用户的UID  
`PassWord` 密码的HASH256值(开发阶段使用明文)
2. 返回字段  
`State`  

|State|说明|
|-|-|
|0|正常|
|-1|一般异常，异常原因在Message字段|

`Token`  
用于后续会话的Token，客户端应保存Token，用于后续的请求。

#### 功能说明
用于用户登录，登录成功后，服务器返回Token，客户端应保存Token，用于后续的请求。


### 注册请求
#### ID 101
#### 请求样例
```JSON
{
	"ID": 101,
	"State": 0,
	"Data": {
		"UserName":"name",
		"PassWord": "test"
	}
}
```
#### 返回样例
```JSON
{
	"ID": 101,
	"State": 0,
	"Message":"Success",
	"Data": {
		"UID": "10000"
	}
}
```
#### 字段说明
1. 请求字段  
`UserName` 用户名,1-40个字符  
`PassWord` 密码的明文  
2. 返回字段  
`State`

|State|说明|
|-|-|
|0|正常|
|-1|一般异常，异常原因在Message字段|

`UID`
用户的UID

#### 功能说明
用于用户注册，注册成功后，服务器返回UID。用户应记住UID用于以后登录

### 好友申请
#### ID 102
#### 请求样例
```JSON
{
	"ID": 102,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"FriendUID": "10001",
		"Message": "Hello"
	}
}
```
#### 返回样例
```JSON
{
	"ID": 102,
	"State": 0,
	"Token": "test",
	"UID": "10000"
}
```
#### 接收样例
```JSON
{
	"ID": 102,
	"State": 100,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"RequsetUID": "10001",
		"Message": "Hello"
	}
}
```
#### 字段说明
1. 请求字段  
`FriendUID` 好友的UID  
`Message` 申请信息
2. 返回字段  
`State`  

|State|说明(返回信息)|
|-|-|
|0|正常|
|-1|一般异常，异常原因在Message字段|
|1|Token错误|
|2|Data字段未包含FriendUID|
|3|已经是好友|
|4|不能添加自己为好友|

3. 接收字段  
`RequestUID` 申请人的UID  
`Message` 申请信息  
`State`


|State|说明(接收信息)|
|-|-|
|100|收到好友请求|

#### 功能说明
客户端主动发送时用于用户申请添加好友  
服务端主动发送时用于通知用户收到好友申请

### 请求好友申请列表

#### ID 103

#### 请求样例
```JSON
{
	"ID": 103,
	"State": 0,
	"Token": "test",
	"UID": "10000",
}
```

#### 返回样例
```JSON
{
	"ID": 103,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"RequestList": [
			{
				"RequestUID": "10001",
				"Message": "Hello"
			},
			{
				"RequestUID": "10002",
				"Message": "Hello"
			}
		]
	}
}
```
#### 字段说明
1. 请求字段  
*略*
2. 返回字段  
`State`  

|State|说明|
|-|-|
|0|正常|
|-1|一般异常，异常原因在Message字段|

#### 功能说明
用于请求好友申请列表，返回好友申请列表，如果没有好友申请，返回空列表。

### 同意或忽略好友申请

#### ID 104
#### 请求样例
```JSON
{
	"ID": 104,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"AgreeUID": "10001",
		"Agree": true
	}
}
```

#### 返回样例
```JSON
{
	"ID": 104,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Message": "Success"
}
```

#### 参数说明
1. 请求字段  
`AgreeUID` 申请人的UID  
`Agree` 是否同意  
2. 返回字段  
`State`

|State|说明|
|-|-|
|0|正常|
|1|Token无效|
|2|AgreeUID无效|
|3|AgreeUID未请求你为好友|
|4|Agree字段不存在或无效|

### UID获取用户名

#### ID 105

#### 请求样例
```JSON
{
	"ID": 105,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"UID": "10001"
	}
}
```

#### 返回样例
```JSON
{
	"ID": 105,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"UID": "10001",
		"UserName": "name"
	}
}
````

#### 参数说明
1. 请求字段  
`UID` 用户的UID  
2. 返回字段  
`Data/UID` 要获取用户名的UID  
`Data/UserName` 用户名  
`State`  

|State|说明|
|-|-|
|0|正常|
|1|Token无效|						
|-1|一般错误,错误原因在Message字段|

#### 功能说明
用于根据UID获取用户的用户名  
### 获取好友列表
#### ID 106
#### 请求样例
```JSON
{
	"ID": 106,
	"State": 0,
	"Token": "test",
	"UID": "10000"
}
```
#### 返回样例
```JSON
{
	"ID": 106,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"data": {
		"FriendsUID": [10001,10002,10004]
	}
}
```
### 设置在线状态
#### ID 107
#### 请求样例
```JSON
{
	"ID": 107,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"OnlineState":"Online"
	}
}
```
#### 返回样例
```JSON
{
	"ID": 107,
	"State": 0,
	"Token": "test",
	"UID": "10000"
}
```
#### 参数说明
1.请求参数  
`OnlineState`:在线状态，Online表示设置为在线，Offline表示设置为离线  
2.返回参数
`State`表示状态  
#### 功能说明
用于设置客户端状态，若设置为离线状态，服务器将不会主动推送消息。登陆后默认为在线状态，未经过登录默认为离线状态。

### 向好友发送消息
#### ID 201
#### 请求样例
```JSON
{
	"ID": 201,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"FriendUID": "10001",
		"MessageType": "text",
		"Message":"发送的消息内容"
	}
}
```

#### 返回样例
```JSON
{
	"ID": 201,
	"State": 0,
	"Token": "test",
	"UID": "10000"
}
```

#### 参数说明
1.请求字段  
`FriendUID`要发送的好友UID  
`MessageType` 要发送的信息类型，目前支持text  
`Message`要发送的消息，测试阶段为明文,因此不支持非二进制数据  
2.返回字段  
`State`  

|State|说明|
|-|-|
|0|正常|
|1|TOKEN错误|
|2|无效的好友UID|
|3|目标不存在|
|4|消息为空|

### 收到消息
#### ID 202
#### 请求样例
该消息不存在请求样例  
#### 返回样例
```JSON
{
	"ID": 202,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"SenderUID":10001,
		"FromType":"friend"
		"MessageType":"text",
		"Message":"hello"
	}
}
```
#### 参数详解
`SenderUID`发送者的UID
`FromType`来自哪种类型的会话，可能是friend、group(群聊)
`MessageType`消息类型
`Message`消息内容，测试阶段为明文

#### 203
#### 请求样例
```JSON
{
	"ID": 203,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
		"TargetUID": "10001",
		"TargetType": "Friend",
		"Range": "0,-1"
	}
}
```
#### 返回样例
```JSON
{
	"ID": 203,
	"State": 0,
	"Token": "test",
	"UID": "10000",
	"Data": {
	"Messages": [
	{
		"Time":123456,
		"SenderUID":10001,
		"MessageType":"text",
		"Message":"hello"
		}
		]
	}
}
```

#### 参数解释
1.请求字段  
TargetUID: 要请求关于谁的消息  
TargetType: 目标类型，目前支持好友(Friend)  
Range: 请求的消息范围，为闭区间，用负数表示倒数，正向从0开始计数  
2.返回字段
`State`  

|State|说明|
|-|-|
|1|Token错误|
|2|缺少参数|
|3|参数类型错误或格式错误|
|4|Target type未知|
|5|权限不足，可能是对方不是好友|
|6|无效的范围|

`Messages`  
返回的参数见返回示例  
#### 功能说明
获取与某个好友有关的指定范围的消息
