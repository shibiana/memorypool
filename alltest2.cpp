#include <vector>
#include <stdlib.h>
#include "ConcurrentMalloc.h"


void testMalloc(size_t nworks, size_t rounds, size_t ntimes)  //线程数  执行轮次  每轮malloc多少次
{
	std::vector<std::thread> vthread(nworks);  //申请nworks个线程
	size_t malloc_costtime = 0;                //malloc所花费的时间
	size_t free_costtime = 0;                  //释放所花费的时间

	for (size_t k = 0; k < nworks; k++)
	{
		vthread[k] = std::thread([&, k]()
		{
			std::vector<void*> v;
			v.reserve(ntimes);                  //先申请ntimes的空间  比一个个自动申请效率要更高

			for (size_t j = 0; j < rounds; j++) //轮次
			{
				size_t begin1 = clock();         //记录起始时间
				for (size_t i = 0; i < ntimes; i++) //每轮malloc次数
				{
					v.push_back(malloc(16));     //malloc的大小必须为16的倍数
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();

				v.clear();

				malloc_costtime += end1 - begin1;
				free_costtime += end2 - begin2;
			}
		});


	}

	for (auto& t : vthread)   //回收线程
		t.join();

	printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime);

	printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime);

	printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
		nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}


// 单轮次申请释放次数 线程数 轮次
void testConcurrentMalloc(size_t nworks, size_t rounds, size_t ntimes)
{
	std::vector<std::thread> vthread(nworks);
	size_t malloc_costtime = 0;
	size_t free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]()
		{
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					v.push_back(ConcurrentMalloc(16));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += end1 - begin1;
				free_costtime += end2 - begin2;
			}
		});
	}

	for (auto& t : vthread)
		t.join();

	printf("%u个线程并发执行%u轮次，每轮次concurrent malloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime);

	printf("%u个线程并发执行%u轮次，每轮次concurrent free %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime);

	printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
		nworks, nworks*rounds*ntimes, malloc_costtime + free_costtime);
}

int main()
{
	cout << "==========================================================" << endl;
	testMalloc(8, 100, 10000);
	cout << endl << endl;
	testConcurrentMalloc(8, 100, 10000);
	cout << "==========================================================" << endl;

	system("pause");
	return 0;
}