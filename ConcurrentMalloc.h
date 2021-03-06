#pragma once
#include "ThreadCache.h"
#include "PageCache.h"

static void* ConcurrentMalloc(size_t size)  //申请
{
	if (size <= MAX_SIZE)
	{
		if (pThreadCache == nullptr)
		{
			pThreadCache = new ThreadCache;
			cout << std::this_thread::get_id() << "->" << pThreadCache << endl;
		}
		return pThreadCache->Allocate(size);
	}

	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))  //小于128页所占的空间 128*64K
	{
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT); //对齐，以页对齐
		size_t pagenum = (align_size >> PAGE_SHIFT);    //计算出页数
		Span* span = PageCache::GetPageCacheInstance().NewSpan(pagenum);
		span->_objSize = align_size;
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else   //如果要申请的内存大于128页  需要向系统申请内存
	{
		size_t align_size = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT);
		size_t pagenum = (align_size >> PAGE_SHIFT);
		return SystemAlloc(pagenum);
	}
}

static void ConcurrentFree(void* ptr)       //释放
{
	size_t pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetPageCacheInstance().GetIdToSpan(pageid);
	if (span == nullptr)
	{
		SystemFree(ptr);
		return;
	}

	size_t size = span->_objSize;
	if (size <= MAX_SIZE)
	{
		pThreadCache->Deallocate(ptr, size);
	}
	else if (size <= ((MAX_PAGES - 1) << PAGE_SHIFT))
	{
		PageCache::GetPageCacheInstance().ReleaseSpanToPageCache(span);
	}
}