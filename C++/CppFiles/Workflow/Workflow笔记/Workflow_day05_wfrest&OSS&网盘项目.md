## MySQL任务
### MySQL任务1
![[Pasted image 20240725222719.png]]
### MySQL任务2
![[Pasted image 20240725222750.png]]
### 一个个地取出结果集
![[Pasted image 20240725222816.png]]

### 获取域（列）的信息

![[Pasted image 20240725222920.png]]

### 获取表格中的内容
![[Pasted image 20240725222931.png]]
## wfrest 
![[Pasted image 20240725223023.png]]

### wfrest的好处
![[Pasted image 20240725223045.png]]

### 框架写的process干了什么事情
![[Pasted image 20240725223112.png]]
![[Pasted image 20240725223120.png]]

### wfrest简化了路由的写法
![[Pasted image 20240813204344.png]]

### wfrest帮助我们解析了数据
![[Pasted image 20240813204427.png]]

### wfrest帮我们解析urlencoded类型的报文体
![[Pasted image 20240813204454.png]]

### form-data格式的报文体
![[Pasted image 20240813204522.png]]

### 实现一个上传的功能
![[Pasted image 20240813204550.png]]

### 在wfrest当中使用workflow的任务和序列机制
![[Pasted image 20240813204710.png]]

### 使用序列的代码
![[Pasted image 20240813204758.png]]

## 网页版本的网盘
![[Pasted image 20240813204826.png]]

### 表的设计
![[Pasted image 20240813204849.png]]

### 全局文件表
![[Pasted image 20240813204911.png]]

### 用户文件表
![[Pasted image 20240813204935.png]]

### token的缓存 加速的token的验证
![[Pasted image 20240813205010.png]]

### CloudiskServer
![[Pasted image 20240813205050.png]]

#### 加载静态资源
![[Pasted image 20240813205109.png]]

#### 注册
![[Pasted image 20240813205130.png]]

### 如何排查网络问题
![[Pasted image 20240813205152.png]]

### 提取用户名&密码 并加密
![[Pasted image 20240813205213.png]]

![[Pasted image 20240813205226.png]]

### 登录 将insert into换成 select from
![[Pasted image 20240813205252.png]]

### 登录成功
![[Pasted image 20240813205313.png]]

### token生成的规则
![[Pasted image 20240813205335.png]]

### 登录成功之后要回复一个json对象
![[Pasted image 20240813205402.png]]

### 将token信息写入数据库，加速验证的速度
![[Pasted image 20240813205440.png]]
### 获取用户信息

![[Pasted image 20240813205521.png]]

![[Pasted image 20240813205533.png]]

### 上传功能
![[Pasted image 20240813205555.png]]

![[Pasted image 20240813205605.png]]

### 将文件的信息写入用户文件表
![[Pasted image 20240813205627.png]]

### 展示文件的信息
![[Pasted image 20240813205646.png]]

### 文件信息读取成功以后
![[Pasted image 20240813205705.png]]

![[Pasted image 20240813205716.png]]

### 前端的代码需要修改一下
![[Pasted image 20240813205737.png]]

### 当用户点击下载按钮
![[Pasted image 20240813205755.png]]

### 部署一个nginx 静态资源服务器
![[Pasted image 20240813205826.png]]

### 面临的问题，单个文件不能太大
![[Pasted image 20240813205909.png]]

### 备份功能
![[Pasted image 20240813205929.png]]

#### 使用一个分布式文件系统实现备份功能
![[Pasted image 20240813205950.png]]

#### Bucket 文件的仓库
![[Pasted image 20240813210013.png]]

#### 设置一下登录凭证
![[Pasted image 20240813210033.png]]

#### OSS的API和SDK
![[Pasted image 20240813210054.png]]

#### 安装之后
![[Pasted image 20240813210111.png]]

#### 上传功能
![[Pasted image 20240813210138.png]]
![[Pasted image 20240813210147.png]]

#### 生成一个下载的链接
![[Pasted image 20240813210208.png]]
