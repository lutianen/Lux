# LuxRedis

Redis.

## **Install Redis**

``` bash
wget https://github.com/redis/redis/archive/7.0.10.tar.gz
tar -zvxf redis-7.0.10.tar.gz
mv /root/redis-7.0.10.tar.gz /usr/local/redis
make
sudo make install
```

## **启动 Redis 服务**

```bash
# 启动 redis 服务
redis-server ./redis.conf

# 查看 redis 是否正常启动
ps -aux | grep redis

# 启动 客户端
redis-cli
```

### redis.conf 配置文件

文件位置：`/usr/local/redis/redis.conf`

**重要的配置选项**

- `daemonize`: `yes` / `no`

    `yes` 表示启用守护进程，默认是 `no 即不以守护进程方式运行。其中Windows系统下不支持启用守护进程方式运行

- `port`: 6379

    指定 Redis 监听端口，默认端口为 6379

- `bind`:

    绑定的主机地址,如果需要设置远程访问则直接将这个属性备注下或者改为`bind *`即可, 这个属性和下面的 `protected-mode`控制了是否可以远程访问

- `protected-mode`: `yes` / `no`

    保护模式，该模式控制外部网是否可以连接 redis 服务，默认是 `yes`,所以默认我们外网是无法访问的，如需外网连接 redis 服务则需要将此属性改为 `no`

- `timeout`: 300

    当客户端闲置多长时间后关闭连接，如果指定为 0，表示关闭该功能

- `loglevel`: `debug`、`verbose`、`notice`、`warning`

    日志级别，默认为 notice

- `databases`: 16

    设置数据库的数量，默认的数据库是16。整个通过客户端工具可以看得到

- `dir`

    指定本地数据库存放目录

- `maxclients`: 0

    设置同一时间最大客户端连接数，默认无限制，Redis 可以同时打开的客户端连接数为 Redis 进程可以打开的最大文件描述符数，如果设置 `maxclients` 问 `0`，表示不作限制。
    当客户端连接数到达限制时，Redis 会关闭新的连接并向客户端返回 `max number of clients reached` 错误信息。

## **Install hiredis**

```bash
git clone https://github.com/redis/hiredis.git
mkdir build
cmake ..
make -j 6
sudo make install
```

### 注意事项

报错示例：

`/home/lux/Lux/build/LuxRedis/test/LuxRedisTest: error while loading shared libraries: libhiredis.so.1.1.1-dev: cannot open shared object file: No such file or directory`

原因：

**编译器只会使用/lib和/usr/lib这两个目录下的库文件，通常通过源码包进行安装时，如果不指定--prefix，会将库安装在/usr/local/lib目录下；当运行程序需要链接动态库时，提示找不到相关的.so库，会报错。也就是说，/usr/local/lib目录不在系统默认的库搜索目录中，需要将目录加进去**

解决方案：

```bash
# 1. 打看 /etc/ld.so.conf 文件
sudo vim /etc/ld.so.conf

# 2. 加入动态文件库所在的目录
include /etc/ld.so.confd/*.conf
/usr/local/lib

# 3. 运行 ldconfig，使所有的库文件都被缓存到文件 /etc/ld.so.cache 中，否则可能会找不到
sudo ldconfig
```
