#pragma once

#include <iostream>
#include <assert.h>
#include <unordered_map>
#include <thread>
#include <mutex> //互斥量

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const size_t MAX_SIZE = 64 * 1024;  //64K  单位字节
const size_t NFREE_LIST = MAX_SIZE / 8; //FREE_LIST以8字节为间隔  字节
const size_t MAX_PAGES = 129; //超过128页要向系统申请
const size_t PAGE_SHIFT = 12; // 4K

inline void*& NextObj(void* obj)   //返回值为指向头结点地址处的指针
{
	return *((void**)obj);
}

class FreeList   //自由链表类
{
public:
	void Push(void* obj)  //头插  传入结点
	{
		NextObj(obj)= _freelist;
		_freelist = obj;
		++_num;
	}

	void* Pop()		      //头删
	{
		void* obj = _freelist;
		_freelist = NextObj(obj);
		--_num;
		return obj;
	}

	void PushRange(void* head, void* tail, size_t num)      //将一个范围内的串插入,num表示插入的数量
	{
		NextObj(tail) = _freelist;                          //将这部分串插入到头部
		_freelist = head;
		_num += num;
	}

	size_t PopRange(void*& start, void*& end, size_t num)     //取一段spanlist链表长度,传入指针的引用,从头开始截取
	{
		size_t actnum = 0;
		void* prev = nullptr;
		void* cur = _freelist;
		for (; actnum < num && cur != nullptr; actnum++)
		{
			prev = cur;
			cur = NextObj(cur);
		}

		start = _freelist;
		end = prev;
		_freelist = cur;
		_num -= actnum;

		return actnum;
	}

	size_t Num()                                             //获取现在链表的长度
	{
		return _num;
	}

	bool Empty()           //判断链表是否为空
	{
		return _freelist == nullptr;
	}

	void Clear()          //清空链表
	{
		_freelist = nullptr;
		_num = 0;
	}

private:
	void* _freelist = nullptr;   //链表的头结点
	size_t _num = 0;             //链表的结点数量
};

class SizeClass                  //计算项目各种大小的类
{
public:
	static size_t _RoundUp(size_t size, size_t alignment)      //字节对齐
	{
		return (size + alignment - 1)&(~(alignment - 1));
	}

	static size_t RoundUp(size_t size)      //字节对齐后的字节数量
	{
		assert(size <= MAX_SIZE);

		if (size <= 128)
		{
			return _RoundUp(size,8);       //1到128字节按照8字节对齐
		}

		else if (size <= 1024)
		{
			return _RoundUp(size,16);
		}
		else if (size <= 8192)
		{
			return _RoundUp(size,128);
		}
		else if (size <= 65536)
		{
			return _RoundUp(size,1024);
		}
		return -1;
	}

	static size_t _ListIndex(size_t size, size_t align_shift)   //判断链表数
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;   //先向上进位，再除以每个单元的字节数
	}

	static size_t ListIndex(size_t size)
	{
		assert(size <= MAX_SIZE);

		//每个区间有多少个链
		static int group_array[3] = {16,56,56};  //每个区间的链的个数
		if (size <= 128)
		{
			return _ListIndex(size,3);           //2^3 8 字节对齐
		}
		else if (size <= 1024)
		{
			return _ListIndex(size-128,4)+group_array[0]; //2^4 16字节对齐
		}
		else if (size <= 8192)
		{
			return _ListIndex(size-1024,7)+group_array[1]+group_array[0]; //2^7 128字节对齐
		}
		else if (size <= 65536)
		{
			return _ListIndex(size - 8192, 10) + group_array[2] + group_array[1] + group_array[0]; //2^10 1024字节对齐
		}
		return -1;
	}

	//[2,512]个///////////////////////////不清晰
	//动态计算从中心缓存分配多少个内存对象到ThreadCache中
	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
		{
			return 0;
		}

		int num = MAX_SIZE / size;
		if (num < 2) num = 2;
		if (num > 512) num = 512;

		return num;
	}

	//一次向系统申请多少页 根据size计算中心缓存要从页缓存获取多大的span对象
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;

		npage >>= 12;           //每页的大小为4K  2^12
		if (npage == 0)
			npage = 1;
		return npage;
	}
};

#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID;
#endif

struct Span {
	PAGE_ID _pageid = 0;     //页号
	PAGE_ID _pagesize = 0;   //页的数量

	FreeList _freeList;      //对象自由链表
	size_t _objSize = 0;     //自由链表对象大小
	int _usecount = 0;       //内存块对象计数

	Span* _next = nullptr;   //链接span前后
	Span* _prev = nullptr;
};

class SpanList               //双向链表：双向、带头、循环   用于centralcache
{
public:
	SpanList()              //head头结点是空结点
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	~SpanList()//释放链表的每个节点
	{
		Span * cur = _head->_next;
		while (cur != _head)
		{
			Span* next = cur->_next;
			delete cur;
			cur = next;
		}
		delete _head;
		_head = nullptr;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}
	      
	void PushFront(Span* newspan)                //头插
	{
		Insert(_head->_next,newspan);
	}

	void PopFront()                              //头删
	{
		Erase(_head->_next);
	}

	void PushBack(Span* newspan)                 //尾插
	{
		Insert(_head,newspan);
	}

	void PopBack()                               //尾删
	{
		Erase(_head->_prev);
	}

	void Insert(Span* pos, Span* newspan)        //插入  维持双向链表  在pos前面插入
	{
		Span* prev = pos->_prev;

		prev->_next = newspan;
		newspan->_next = pos;
		newspan->_prev = prev;
		pos->_prev = newspan;
		
	}

	void Erase(Span* pos)                        //删除  要删除的结点的位置
	{
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	bool Empty()
	{
		return Begin() == End();
	}

	void Lock()
	{
		_mtx.lock();
	}

	void unlock()
	{
		_mtx.unlock();
	}

private:
	Span* _head;
	std::mutex _mtx;
};

inline static void* SystemAlloc(size_t num_page)        //向系统申请内存
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, num_page*(1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE); //win申请内存函数
#else
	// brk mmap等
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

inline static void SystemFree(void* ptr)                 //内存释放
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);                    //内存释放
#else
#endif
}