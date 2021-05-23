#pragma once

#include "common.h"

class ThreadCache
{
public:
	//�����ڴ���ͷ��ڴ�
	void* Allocate(size_t size);            //�����ڴ�
	void Deallocate(void* ptr, size_t size);  //�ͷ��ڴ�

	//�����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index);

	//������������еĶ��󳬹�һ�����Ⱦ�Ҫ�ͷŸ����Ļ���
	void ListTooLong(FreeList& freeList, size_t num, size_t size);


private:
	FreeList _freeLists[NFREE_LIST];
};

// �߳�TLS Thread Local Storage       �൱����ÿ���̶߳��������Լ����Ǹ�����
_declspec (thread) static ThreadCache* pThreadCache = nullptr; //thread ��������һ���̱߳��ر���,���԰�������Ϊstatic��ʹÿ���߳̿�����ͬʱÿ���̵߳õ����������Լ���ֵ