#pragma once
#include "common.h"

class PageCache
{
public:
	//�����µ�span
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);

	//�ͷſռ�span��pagecache�����ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);

	Span* GetIdToSpan(PAGE_ID id);

	static PageCache& GetPageCacheInstance()
	{
		static PageCache inst;
		return inst;
	}

private:
	//ͬCentralCache ����ģʽ
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	SpanList _spanLists[MAX_PAGES];

	std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	std::mutex _mtx;
};