#pragma once
#include "common.h"

class PageCache
{
public:
	//申请新的span
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);

	//释放空间span回pagecache，并合并相邻的span
	void ReleaseSpanToPageCache(Span* span);

	Span* GetIdToSpan(PAGE_ID id);

	static PageCache& GetPageCacheInstance()
	{
		static PageCache inst;
		return inst;
	}

private:
	//同CentralCache 单例模式
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	SpanList _spanLists[MAX_PAGES];

	std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	std::mutex _mtx;
};