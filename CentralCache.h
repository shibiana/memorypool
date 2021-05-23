#pragma once
#include "common.h"

class CentralCache
{
public:
	//�����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);

	//��һ�������Ķ����ͷŵ�span���  ����central cache
	void ReleaseListToSpans(void* start, size_t size);

	//��spanlist ���� page cache ��ȡһ��span
	Span* GetOneSpan(size_t size);

	static CentralCache& GetInstance()
	{
		static CentralCache inst;
		return inst;
	}

private:
	//��������֤����һ������
	CentralCache()
	{}

	//��ֹsingleton single = singleton :: GetInstance();������ʽ������Ĭ�ϵĿ������캯����Υ���������� 
	CentralCache(const CentralCache&) = delete;    //��ֹ��������  Ҳ��������ֻ�������ǲ����忽�����캯��������=������

	SpanList _spanLists[NFREE_LIST];
};