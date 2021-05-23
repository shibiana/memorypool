#include "ThreadCache.h"
#include "CentralCache.h"

//从中心缓存获取对象
void* ThreadCache::FetchFromCentralCache(size_t size)
{
	size_t num = SizeClass::NumMoveSize(size);

	//CentralCache获取内容
	void* start = nullptr, *end = nullptr;
	size_t actualNum = CentralCache::GetInstance().FetchRangeObj(start, end, num, size);

	if (actualNum == 1)
	{
		return start;
	}
	else
	{
		size_t index = SizeClass::ListIndex(size);                //找到对应大小的链对应的标号
		FreeList& list = _freeLists[index];                    
		list.PushRange(NextObj(start), end, actualNum - 1);

		return start;
	}

}

//申请内存和释放内存
void* ThreadCache::Allocate(size_t size)            //申请内存
{
	size_t index = SizeClass::ListIndex(size);      //第几条链的标号
	FreeList& freeList = _freeLists[index];
	if (!freeList.Empty())
		return freeList.Pop();
	else
		return FetchFromCentralCache(SizeClass::RoundUp(size));   //获得对齐后的内存大小

}


//如果自由链表中的对象超过一定长度就要释放给中心缓存  还给span
void ThreadCache::ListTooLong(FreeList& freeList, size_t num, size_t size)
{
	void* start = nullptr, *end = nullptr;
	freeList.PopRange(start,end,num);

	NextObj(end) = nullptr;
	CentralCache::GetInstance().ReleaseListToSpans(start,size);
}

void ThreadCache::Deallocate(void* ptr, size_t size)  //释放内存
{
	size_t index = SizeClass::ListIndex(size);		  //
	FreeList& freeList = _freeLists[index];           //指定第几号结点
	freeList.Push(ptr);

	size_t num = SizeClass::NumMoveSize(size);        //从中心缓存分配过来的内存数量
	if (freeList.Num() >= num)					      //达到一定的数量
	{
		ListTooLong(freeList,num,size);
	}
}