#include "PageCache.h"

//申请新的span
Span* PageCache::_NewSpan(size_t numpage)
{
	//如果这个位置有 则在这个位置取
	if (!_spanLists[numpage].Empty())
	{
		Span* span = _spanLists[numpage].Begin(); 
		_spanLists[numpage].PopFront();                //每次都从前面取
		return span;
	}

	//若这个位置没有，则不需要走这个位置
	for (size_t i = numpage + 1; i < MAX_PAGES; i++)
	{
		if (!_spanLists[i].Empty())    //如果在后续找到更大的链表  就取下进行分裂
		{
			Span* span = _spanLists[i].Begin();     //找到的那个更大的链表上的第一个span单元
			_spanLists[i].PopFront();

			Span* splitspan = new Span;
			//尾切  将大的span从后面切出两部分  一部分分配  一部分接到较小的spanlist上
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;      //splitspan的起始id
			splitspan->_pagesize = numpage;       //splitspan的大小  分下来两部分中小的那部分
			for (PAGE_ID i = 0; i < numpage; i++)
			{
				_idSpanMap[splitspan->_pageid + i] = splitspan;
			}

			span->_pagesize -= numpage;           //剩下那部分的大小

			_spanLists[span->_pagesize].PushFront(span);  //将剩下那部分的span挂在到对应的spanlists上

			return splitspan;
		}
	}

	//如果pagecache上到128页都没有空闲span了，就需要找系统申请
	void* ptr = SystemAlloc(MAX_PAGES - 1);

	Span* bigspan = new Span;
	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigspan->_pagesize = MAX_PAGES - 1;

	for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;          //将申请的128页都分配到对应span上
	}

	_spanLists[bigspan->_pagesize].PushFront(bigspan);

	return _NewSpan(numpage);

}

Span* PageCache::NewSpan(size_t numpage)
{
	_mtx.lock();
	Span* span = _NewSpan(numpage);
	_mtx.unlock();

	return span;
}

//释放空间span回pagecache，并合并相邻的span   还给pagecache
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//页合并：先前再后
	while (1)
	{
		PAGE_ID prevPageId = span->_pageid - 1;
		auto pit = _idSpanMap.find(prevPageId);
		//如果pit不存在
		if (pit == _idSpanMap.end())
		{
			break;
		}

		//
		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0)             //说明前一个span还有人在用不能合并
		{
			break;
		}

		//超过128页的span，不要合并
		if (span->_pagesize + prevSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}

		//合并
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; i++)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		_spanLists[prevSpan->_pagesize].Erase(prevSpan);            //pagecache是根据页的大小建立索引的
		delete prevSpan;
	}

	//向后合并
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		//后面的页不存在
		if (nextIt == _idSpanMap.end())
		{
			break;
		}

		//后面的页在使用中
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)
		{
			break;
		}

		if (span->_pagesize + nextSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}

		//合并
		span->_pagesize += nextSpan->_pagesize;
		for (PAGE_ID i = 0; i < nextSpan->_pagesize; i++)
		{
			_idSpanMap[nextSpan->_pageid + i] = span;
		}

		_spanLists[nextSpan->_pagesize].Erase(nextSpan);
		delete nextSpan;
	}

	//将page cache 中的span挂入
	_spanLists[span->_pagesize].PushFront(span);
}

Span* PageCache::GetIdToSpan(PAGE_ID id)
{
	auto it = _idSpanMap.find(id);
	if (it != _idSpanMap.end())
	{
		return it->second;
	}
	else
		return nullptr;
}