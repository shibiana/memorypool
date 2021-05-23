#include "PageCache.h"

//�����µ�span
Span* PageCache::_NewSpan(size_t numpage)
{
	//������λ���� �������λ��ȡ
	if (!_spanLists[numpage].Empty())
	{
		Span* span = _spanLists[numpage].Begin(); 
		_spanLists[numpage].PopFront();                //ÿ�ζ���ǰ��ȡ
		return span;
	}

	//�����λ��û�У�����Ҫ�����λ��
	for (size_t i = numpage + 1; i < MAX_PAGES; i++)
	{
		if (!_spanLists[i].Empty())    //����ں����ҵ����������  ��ȡ�½��з���
		{
			Span* span = _spanLists[i].Begin();     //�ҵ����Ǹ�����������ϵĵ�һ��span��Ԫ
			_spanLists[i].PopFront();

			Span* splitspan = new Span;
			//β��  �����span�Ӻ����г�������  һ���ַ���  һ���ֽӵ���С��spanlist��
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;      //splitspan����ʼid
			splitspan->_pagesize = numpage;       //splitspan�Ĵ�С  ��������������С���ǲ���
			for (PAGE_ID i = 0; i < numpage; i++)
			{
				_idSpanMap[splitspan->_pageid + i] = splitspan;
			}

			span->_pagesize -= numpage;           //ʣ���ǲ��ֵĴ�С

			_spanLists[span->_pagesize].PushFront(span);  //��ʣ���ǲ��ֵ�span���ڵ���Ӧ��spanlists��

			return splitspan;
		}
	}

	//���pagecache�ϵ�128ҳ��û�п���span�ˣ�����Ҫ��ϵͳ����
	void* ptr = SystemAlloc(MAX_PAGES - 1);

	Span* bigspan = new Span;
	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigspan->_pagesize = MAX_PAGES - 1;

	for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;          //�������128ҳ�����䵽��Ӧspan��
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

//�ͷſռ�span��pagecache�����ϲ����ڵ�span   ����pagecache
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//ҳ�ϲ�����ǰ�ٺ�
	while (1)
	{
		PAGE_ID prevPageId = span->_pageid - 1;
		auto pit = _idSpanMap.find(prevPageId);
		//���pit������
		if (pit == _idSpanMap.end())
		{
			break;
		}

		//
		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0)             //˵��ǰһ��span���������ò��ܺϲ�
		{
			break;
		}

		//����128ҳ��span����Ҫ�ϲ�
		if (span->_pagesize + prevSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}

		//�ϲ�
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; i++)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		_spanLists[prevSpan->_pagesize].Erase(prevSpan);            //pagecache�Ǹ���ҳ�Ĵ�С����������
		delete prevSpan;
	}

	//���ϲ�
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		//�����ҳ������
		if (nextIt == _idSpanMap.end())
		{
			break;
		}

		//�����ҳ��ʹ����
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)
		{
			break;
		}

		if (span->_pagesize + nextSpan->_pagesize >= MAX_PAGES)
		{
			break;
		}

		//�ϲ�
		span->_pagesize += nextSpan->_pagesize;
		for (PAGE_ID i = 0; i < nextSpan->_pagesize; i++)
		{
			_idSpanMap[nextSpan->_pageid + i] = span;
		}

		_spanLists[nextSpan->_pagesize].Erase(nextSpan);
		delete nextSpan;
	}

	//��page cache �е�span����
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