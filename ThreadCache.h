#pragma once

#include "common.h"

class ThreadCache
{
public:
	//申请内存和释放内存
	void* Allocate(size_t size);            //申请内存
	void Deallocate(void* ptr, size_t size);  //释放内存

	//从中心缓存获取对象
	void* FetchFromCentralCache(size_t index);

	//如果自由链表中的对象超过一定长度就要释放给中心缓存
	void ListTooLong(FreeList& freeList, size_t num, size_t size);


private:
	FreeList _freeLists[NFREE_LIST];
};

// 线程TLS Thread Local Storage       相当于是每个线程都有属于自己的那个副本
_declspec (thread) static ThreadCache* pThreadCache = nullptr; //thread 用于声明一个线程本地变量,可以把它定义为static，使每个线程看到，同时每个线程得到的是属于自己的值