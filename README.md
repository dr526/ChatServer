# ChatServer

基于moduo库实现的集群聊天服务器

使用了nginx tcp负载均衡模块

使用redis作为服务器中间件，借助redis发布-订阅功能实现跨服务器通信

# 环境配置

1. 操作系统：Ubuntu18.04

2. muduo网络库源码编译安装：https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980

3. mysql环境安装配置

   下载安装mysql-server和mysql开发包

   ```bash
   sudo apt-get install mysql-server
   sudo apt-get install libmysqlclient-dev
   ```

   修改mysql的root用户密码

   ```bash
   #第一步：查看初始用户名和密码
   sudo cat /etc/mysql/debian.cnf
   #第二步：使用查看到的用户名和密码登录mysql
   mysql -u (查看到的username) -p(查看到的password)
   #第三步：修改root用户的密码
   update mysql.user set authentication_string=password('新密码') where user='root' and host='localhost';
   #第四步：设置MySQL字符编码为UTF-8
   set character_set_server=utf8;
   ```

4. redis环境安装和配置

   下载redis

   ```bash
   sudo apt-get install redis-server
   #ubuntu通过上面命令安装完redis，会自动启动redis服务
   ```

   确认redis服务是否正常启动

   

5. nginx配置tcp负载均衡

   下载nginx：[nginx: download](https://nginx.org/en/download.html)【我用的是nginx-1.12.2.tar.gz】

   解压nginx

   ```bash
   tar -xvf nginx-1.12.2.tar.gz 
   ```

   进入解压好的nginx-1.12.2目录

   ![nginx1](https://github.com/dr526/MyPicture/blob/main/nginx1.png)

   编译nginx

   ![编译2](https://github.com/dr526/MyPicture/blob/main/nginx2.png)

   出现错误

   ![nginx3](https://github.com/dr526/MyPicture/blob/main/nginx3.png)

   安装pcre库，出现错误

   ![nginx4](https://github.com/dr526/MyPicture/blob/main/nginx4.png)

   手动编译安装pcre库

   （1）下载并解压pcre库

   ```bash
   wget https://ftp.pcre.org/pub/pcre/pcre-8.43.tar.gz
   tar -xvf pcre-8.43.tar.gz
   ```

   ![nginx5](https://github.com/dr526/MyPicture/blob/main/nginx5.png)

   （2）编译安装pcre库

   ```bash
   cd pcre-8.43
   sudo ./configure
   sudo make
   sudo make install
   ```

   重新编译nginx

   ```bash
   #在nginx-1.12.2目录下
   sudo ./configure --with-stream
   ```

   命令执行成功

   ![nginx6](https://github.com/dr526/MyPicture/blob/main/nginx6.png)

   继续执行编译

   ```bash
   sudo make && make install
   ```

   出现"struct crypt_data"没有名为"current_salt"成员的错误

   ![nginx7](https://github.com/dr526/MyPicture/blob/main/nginx7.png)

   解决方案：进入相应路径，将源码的第36行注释

   ```bash
   sudo vi src/os/unix/ngx_user.c
   ```

   ![nginx8](https://github.com/dr526/MyPicture/blob/main/nginx8.png)

   重新执行sudo make && make install命令

   出现-Werror=cast-function-type错误

   ![nginx9](https://github.com/dr526/MyPicture/blob/main/nginx9.png)

   解决方案

   ```bash
   #进入nginx-1.12.2目录下的objs目录
   cd objs
   #修改Makefile文件
   sudo vi Makefile 
   ```

   ![nginx10](https://github.com/dr526/MyPicture/blob/main/nginx10.png)

   重新回到nginx-1.12.2目录下执行sudo make && make install命令

   出现权限不够错误

   ![nginx11](https://github.com/dr526/MyPicture/blob/main/nginx11.png)

   进入root模式执行命令

   ```bash
   sudo su #进入root模式
   make && make install
   ```

   编译完成后，进入/usr/local/nginx查看

   ```bash
   cd /usr/local/nginx/
   ls
   ```

   ![nginx12](https://github.com/dr526/MyPicture/blob/main/nginx12.png)

   启动nginx

   ```bash
   cd sbin
   ./nginx
   ```

   出现无法连接pcre库的错误

   ![nginx13](https://github.com/dr526/MyPicture/blob/main/nginx13.png)

   查看依赖库

   ![nginx14](https://github.com/dr526/MyPicture/blob/main/nginx14.png)

   到/usr/local/lib目录下查看

   ![nginx15](https://github.com/dr526/MyPicture/blob/main/nginx15.png)

   设置软连接

   ```bash
   #回到nginx下的sbin目录
   cd /usr/local/nginx/sbin
   #设置软连接
   ln -s /usr/local/lib/libpcre.so.1.2.11 libpcre.so.1
   #设置LD_LIBRARY_PATH(注：这种方法，每次开启nginx都需要重新设置LD_LIBRARY_PATH)
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   ```

   重新启动nginx

   ```bash
   ./nginx
   # 查看服务是否正常启动
   netstat -tanp
   ```

   ![nginx16](https://github.com/dr526/MyPicture/blob/main/nginx16.png)

   配置负载均衡

   ```bash
   #进入/usr/local/nginx下的conf目录
   cd conf
   #编辑nginx.conf文件
   sudo vi nginx.conf 
   ```

   ![nginx17](https://github.com/dr526/MyPicture/blob/main/nginx17.png)

   重启nginx

   ```bash
   #进入root模式
   sudo su
   #进入/usr/local/nginx/sbin目录
   cd /usr/local/nginx/sbin
   #设置LD_LIBRARY_PATH
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   #平滑重启nginx
   ./nginx -s reload
   ```

   ![nginx18](https://github.com/dr526/MyPicture/blob/main/nginx18.png)

6. 创建数据库

   ```bash
   #登录mysql
   mysql -u root -p
   #执行sql语句
   source chat.sql
   ```

7. 修改数据库连接池配置

   修改bin/mysql.ini文件

   ```bash
   #数据库连接池的配置文件
   ip=数据库所在服务器ip
   port=3306
   username=root
   password=修改为你的密码
   dbname=数据库名称
   #数据库连接池初始化的连接数
   initSize=10
   maxSize=1024
   #最大空闲时间默认单位是秒
   maxIdleTime=60
   #连接超时时间单位是毫秒
   connectionTimeout=100
   
   ```

# 编译方式

```bash
#进入build目录
cd build
#清空build目录下所有文件
rm -rf *
#执行cmake命令
cmake ..
#执行make命令
make
```
