#include "CentralCache.h"
#include "PageCache.h"

//�����Ļ����ȡһ�������Ķ����thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)
{
	size_t index = SizeClass::ListIndex(size);  //threadcache��centralcache������8 16...�ֽ�Ϊӳ��� pagecache�ǰ���ҳ����ӳ���
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	Span* span = GetOneSpan(size);
	FreeList& freelist = span->_freeList;
	size_t actulNum = freelist.PopRange(start,end,num);
	span->_usecount += actulNum;         

	spanlist.unlock();//����

	return actulNum;


}

//��һ�������Ķ����ͷŵ�span���  ����central cache
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

		//��ʾ��ǰspan�г�ȥ�Ķ���ȫ������ �ɽ�span ���ظ�pagecache�ϲ�
		if (span->_usecount == 0)
		{
			size_t index = SizeClass::ListIndex(span->_objSize);          //���ݶ����С �ҵ��Լ����ڵ�����
			_spanLists[index].Erase(span);
			span->_freeList.Clear();

			PageCache::GetPageCacheInstance().ReleaseSpanToPageCache(span);
		}

		start = next;
	}

	spanlist.unlock();
}

//��spanlist ���� page cache ��ȡһ��span
Span* CentralCache::GetOneSpan(size_t size)
{
	//����spanlist:������spanlist���ң����û�о���page cache�л�ȡ
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();           //���� ����ͷ
	while (it != spanlist.End())
	{
		if (!it->_freeList.Empty())
		{
			return it;
		}
		else
			it = it->_next;

	}

	//�����spanlist��û���ҵ�����Ҫ��page cache�л�ȡһ��span
	size_t numpage = SizeClass::ListIndex(size);
	Span* span = PageCache::GetPageCacheInstance().NewSpan(numpage);

	//��span�����зֳɶ�Ӧ��С�ҵ�span��freelist��
	char* start = (char*)(span->_pageid << 12);    //4K ÿҳ�Ĵ�С
	char* end = start + (span->_pagesize << 12);
	while (start < end)
	{
		char* obj = start;
		start += size;
		span->_freeList.Push(obj);
	}
	//�Ƚ�span�����з�  �ٽ�span���ص�centralcache��spanlist��
	span->_objSize = size;
	spanlist.PushFront(span);

	return span;
}