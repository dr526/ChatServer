#include "CommonConnectionPool.h"
#include<muduo/base/Logging.h>

//连接池的构造
ConnectionPool::ConnectionPool() {
	//加载了配置项
	if (!loadConfigFile()) {
		return;
	}

	//创建初始数量的连接
	for (int i = 0; i < this->initSize; ++i) {
		MySQL* p = new MySQL();
		p->connect(ip, port, username, password, dbname);
		p->refreshAliveTime();//刷新一下开始空闲的起始时间
		this->connectionQueue.push(p);
		++this->connectionCnt;
	}

	//启动一个新的线程，作为连接的生产者
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();

	//启动一个定时线程，扫描超过maxIdleTime时间的空闲连接，进行连接的回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

//扫描超过maxIdleTime时间的空闲连接，进行连接的回收
void ConnectionPool::scannerConnectionTask() {
	for (;;) {
		//通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(maxIdleTime));

		//扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(queueMutex);
		while (this->connectionCnt > this->initSize) {
			MySQL* p = this->connectionQueue.front();
			if (p->getAliveTime() >= maxIdleTime * 1000) {
				this->connectionQueue.pop();
				--connectionCnt;
				delete p;//调用~connection()释放连接
			}
			else {
				break;//队头的连接没有超过maxIdleTime，其它连接肯定没有
			}
		}
	}
}

//运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask() {
	for (;;) {
		unique_lock<mutex> lock(this->queueMutex);
		while (!this->connectionQueue.empty()) {
			cv.wait(lock);//队列不空，此处生产线程进入等待状态，并释放锁
		}
		//连接数量没有到达上限，继续创建新的连接
		if (this->connectionCnt < this->maxSize) {
			MySQL* p = new MySQL();
			p->refreshAliveTime();//刷新以下开始空闲的起始时间
			p->connect(ip, port, username, password, dbname);
			this->connectionQueue.push(p);
			++this->connectionCnt;
		}
		//通知消费者线程，可以进行连接
		cv.notify_all();
	}
}

//线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool() {
	static ConnectionPool pool;//静态对象，由编译器自动进行lock()和unlock()
	return &pool;
}

//从配置文件中加载配置项
bool ConnectionPool::loadConfigFile() {
	FILE* fp=fopen("mysql.ini", "r");
	if (fp == nullptr) {
		LOG_INFO<<"mysql.ini file is not exist!";
		return false;
	}
	while (!feof(fp)) {
		char line[1024] = { 0 };
		fgets(line, 1024, fp);
		std::string str = line;
		int idx = str.find('=', 0);
		if (idx == -1) {//无效的配置项或注释
			continue;
		}

		int endidx = str.find('\n', idx);
		std::string key = str.substr(0, idx);
		std::string value = str.substr(idx + 1, endidx - idx - 1);
		if (key == "ip") {
			this->ip = value;
		}
		else if (key == "port") {
			this->port = atoi(value.c_str());
		}
		else if (key == "username") {
			this->username = value;
		}
		else if (key == "password") {
			this->password = value;
		}
		else if (key == "dbname") {
			this->dbname = value;
		}
		else if (key == "initSize") {
			this->initSize = atoi(value.c_str());
		}
		else if (key == "maxSize") {
			this->maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime") {
			this->maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout") {
			this->connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}

//给外部提供接口，从连接池中获取一个可用的空闲连接
std::shared_ptr<MySQL> ConnectionPool::getConnection() {
	unique_lock<mutex> lock(queueMutex);
	while (this->connectionQueue.empty()) {
		//等待超时时间，有可能在等待结束后被其它抢了
		if (cv_status::timeout == cv.wait_for(lock, chrono::microseconds(this->connectionTimeout))) {
			if (this->connectionQueue.empty()) {
				LOG_INFO<<"获取空闲连接超时了……获取连接失败！";
				return nullptr;
			}
		}
	}
	/**
		智能指针在析构时，会把connecion资源直接delete掉
		相当于调用connection的析构函数，connection就被close掉
		这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
	**/
	std::shared_ptr<MySQL> sp(this->connectionQueue.front(),
		//&是捕获外部变量
		[&](MySQL* pcon) {
			//这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
			unique_lock<mutex> lock(queueMutex);
			pcon->refreshAliveTime();//刷新一下开始空闲的起始时间
			connectionQueue.push(pcon);
		});
	this->connectionQueue.pop();
	if (this->connectionQueue.empty()) {
		cv.notify_all();//当最后一个connection被消费后，就会通知生产者生产连接
	}
	return sp;
}

