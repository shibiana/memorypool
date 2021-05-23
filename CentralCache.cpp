#include "CentralCache.h"
#include "PageCache.h"

//从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)
{
	size_t index = SizeClass::ListIndex(size);  //threadcache和centralcache都是以8 16...字节为映射的 pagecache是按照页进行映射的
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	Span* span = GetOneSpan(size);
	FreeList& freelist = span->_freeList;
	size_t actulNum = freelist.PopRange(start,end,num);
	span->_usecount += actulNum;         

	spanlist.unlock();//解锁

	return actulNum;


}

//将一定数量的对象释放到span跨度  还给central cache
void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	while (start)
	{
		void* next = NextObj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = PageCache::GetPageCacheInstance().GetIdToSpan(id);
		span->_freeList.Push(start);
		span->_usecount--;

		//表示当前span切出去的对象全部返回 可将span 返回给pagecache合并
		if (span->_usecount == 0)
		{
			size_t index = SizeClass::ListIndex(span->_objSize);          //根据对象大小 找到自己所在的链表
			_spanLists[index].Erase(span);
			span->_freeList.Clear();

			PageCache::GetPageCacheInstance().ReleaseSpanToPageCache(span);
		}

		start = next;
	}

	spanlist.unlock();
}

//从spanlist 或者 page cache 获取一个span
Span* CentralCache::GetOneSpan(size_t size)
{
	//遍历spanlist:优先在spanlist中找，如果没有就在page cache中获取
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();           //返回 链表头
	while (it != spanlist.End())
	{
		if (!it->_freeList.Empty())
		{
			return it;
		}
		else
			it = it->_next;

	}

	//如果在spanlist中没有找到就需要在page cache中获取一个span
	size_t numpage = SizeClass::ListIndex(size);
	Span* span = PageCache::GetPageCacheInstance().NewSpan(numpage);

	//把span对象切分成对应大小挂到span的freelist中
	char* start = (char*)(span->_pageid << 12);    //4K 每页的大小
	char* end = start + (span->_pagesize << 12);
	while (start < end)
	{
		char* obj = start;
		start += size;
		span->_freeList.Push(obj);
	}
	//先将span进行切分  再将span挂载到centralcache的spanlist上
	span->_objSize = size;
	spanlist.PushFront(span);

	return span;
}