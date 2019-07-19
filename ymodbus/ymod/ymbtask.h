/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#ifndef __YMODBUS_YMBTASK_H__
#define __YMODBUS_YMBTASK_H__

#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace YModbus {

class Task
{
public:
	Task();
	~Task();

	virtual void Start(void);
	virtual void Wait(void);

	//用于控制单个个体
	virtual void Stop(void) { stop_ = true; }

	//用于全部继承本类型的对象
	static void LetUsGo(void);
	static void LetUsDone(void);

protected:
	bool IsRunning(void) { return running_ && !stop_; }
	virtual void Run(void) = 0;
	virtual std::string Name(void) = 0;

private:
	virtual bool Init(void) { return true; }
	virtual void Fini(void) {}

	void task(void);
	static bool ShallWeGo(void);

	bool init_;
	bool stop_; //用于个体

	std::thread task_;

	//用于全部继承本类型的对象
	static volatile bool running_;
	static volatile bool go_;
	static std::mutex goMutex_;
	static std::condition_variable goCond_;
};

} //namespace YModbus

#endif // __YMODBUS_YMBTASK_H__
