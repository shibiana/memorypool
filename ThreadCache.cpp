#include "ThreadCache.h"
#include "CentralCache.h"

//�����Ļ����ȡ����
void* ThreadCache::FetchFromCentralCache(size_t size)
{
	size_t num = SizeClass::NumMoveSize(size);

	//CentralCache��ȡ����
	void* start = nullptr, *end = nullptr;
	size_t actualNum = CentralCache::GetInstance().FetchRangeObj(start, end, num, size);

	if (actualNum == 1)
	{
		return start;
	}
	else
	{
		size_t index = SizeClass::ListIndex(size);                //�ҵ���Ӧ��С������Ӧ�ı��
		FreeList& list = _freeLists[index];                    
		list.PushRange(NextObj(start), end, actualNum - 1);

		return start;
	}

}

//�����ڴ���ͷ��ڴ�
void* ThreadCache::Allocate(size_t size)            //�����ڴ�
{
	size_t index = SizeClass::ListIndex(size);      //�ڼ������ı��
	FreeList& freeList = _freeLists[index];
	if (!freeList.Empty())
		return freeList.Pop();
	else
		return FetchFromCentralCache(SizeClass::RoundUp(size));   //��ö������ڴ��С

}


//������������еĶ��󳬹�һ�����Ⱦ�Ҫ�ͷŸ����Ļ���  ����span
void ThreadCache::ListTooLong(FreeList& freeList, size_t num, size_t size)
{
	void* start = nullptr, *end = nullptr;
	freeList.PopRange(start,end,num);

	NextObj(end) = nullptr;
	CentralCache::GetInstance().ReleaseListToSpans(start,size);
}

void ThreadCache::Deallocate(void* ptr, size_t size)  //�ͷ��ڴ�
{
	size_t index = SizeClass::ListIndex(size);		  //
	FreeList& freeList = _freeLists[index];           //ָ���ڼ��Ž��
	freeList.Push(ptr);

	size_t num = SizeClass::NumMoveSize(size);        //�����Ļ������������ڴ�����
	if (freeList.Num() >= num)					      //�ﵽһ��������
	{
		ListTooLong(freeList,num,size);
	}
}