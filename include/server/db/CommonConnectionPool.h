#pragma once
#include<string>
#include<queue>
#include"db.h"
#include<mutex>
#include<condition_variable>
#include<thread>
#include<memory>
#include<functional>
#include<atomic>//原子类型，它的访问保证不会导致数据的竞争，并且可以用于在不同的线程之间同步内存访问。
/**
	实现连接池功能模块
**/
class ConnectionPool
{
public:
	//给外部提供接口，从连接池中获取一个可用的空闲连接
	//返回一个智能指针，当智能指针出作用域时会自动析构
	std::shared_ptr<MySQL> getConnection();

	//获取连接池对象实例
	static ConnectionPool* getConnectionPool();

private:
	//单例#1 构造函数私有化
	ConnectionPool();

	//从配置文件中加载配置项
	bool loadConfigFile();

	//运行在独立的线程中，专门负责生产新连接
	void produceConnectionTask();

	//启动一个定时线程，扫描超过maxIdleTime时间的空闲连接，进行连接的回收
	void scannerConnectionTask();

	std::string ip;//数据库的ip地址
	unsigned short port;//数据库的端口号
	std::string username;//数据库的用户名
	std::string password;//数据库的密码
	std::string dbname;//连接的数据库名称
	int initSize;//连接池初始量
	int maxSize;//连接池最大连接量
	int maxIdleTime;//最大空闲时间
	int connectionTimeout;//连接超时时间
	std::queue<MySQL*> connectionQueue;//存储MySQL连接队列
	std::mutex queueMutex;//维护连接队列的线程安全互斥锁
	std::atomic_int connectionCnt;//记录所创建的connection连接的总数量
	std::condition_variable cv;//设置条件变量，用于连接生产线程和消费线程的通信
};

