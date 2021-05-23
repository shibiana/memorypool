#pragma once
#include "common.h"

class CentralCache
{
public:
	//从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);

	//将一定数量的对象释放到span跨度  还给central cache
	void ReleaseListToSpans(void* start, size_t size);

	//从spanlist 或者 page cache 获取一个span
	Span* GetOneSpan(size_t size);

	static CentralCache& GetInstance()
	{
		static CentralCache inst;
		return inst;
	}

private:
	//单例：保证创建一个对象
	CentralCache()
	{}

	//防止singleton single = singleton :: GetInstance();这种形式会生成默认的拷贝构造函数，违背单例特性 
	CentralCache(const CentralCache&) = delete;    //阻止拷贝构造  也可以在这只声明但是不定义拷贝构造函数和重载=操作符

	SpanList _spanLists[NFREE_LIST];
};