/**
* ymodbus
* Copyright © 2019-2019 liuyongqing<lyqdy1@163.com>
* v1.0.1 2019.05.04
*/
#include "ymod/ymbtask.h"
#include "ymblog.h"

namespace YModbus {

Task::Task()
	: init_(false)
	, stop_(false)
{

}


Task::~Task()
{

}

void Task::Start()
{
	YMB_DEBUG("Start Task . %s\n", this->Name().c_str());

	init_ = Init();

	if (init_) {
		task_ = std::thread(&Task::task, this);
	}
	else {
		YMB_ERROR("Task Init failed!!! %s", this->Name().c_str());
#ifndef WIN32
		abort();
#endif
	}
}

void Task::Wait()
{
	if (task_.joinable())
		task_.join();

	if (init_) {
		Fini();
		init_ = false;
	}
}

void Task::task()
{
	ShallWeGo(); //wait for go signal

	YMB_DEBUG("Task start run. %s\n", this->Name().c_str());

	Run();

	YMB_DEBUG("Task exit run. %s\n", this->Name().c_str());
}

void Task::LetUsGo(void)
{
	go_ = true;
	goCond_.notify_all();
}

void Task::LetUsDone(void)
{
	running_ = false;
}

bool Task::ShallWeGo(void)
{
	std::unique_lock<std::mutex> lock(goMutex_);
	goCond_.wait(lock, []() { return Task::go_; });
	return go_;
}

volatile bool Task::running_ = true;
volatile bool Task::go_ = false;
std::mutex Task::goMutex_;
std::condition_variable Task::goCond_;

} //namespace YModbus
