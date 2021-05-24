#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <boost\pool\pool.hpp>
#include <stddef.h>

using namespace std;

static int RADIX_INSERT_VALUE_OCCUPY = -1;
static int RADIX_INSERT_VALUE_SAME = -2;
static int RADIX_DELETE_ERROR = -3;



#define BITS 2

const int radix_tree_height = sizeof(int) * 8 / BITS;//基数树的高度

//将32位的树先向左移动若干位，再向右移动30位，保留对应pos的数
#define CHECK_BITS(key,pos)((((unsigned int)(key))<<sizeof(int)*8-((pos)+1)*BITS)>>(sizeof(int)*8-BITS))

//基数树节点:每个节点的父节点  四个孩子  和其本身对应的值
template<typename T>
struct radix_node_t {
	radix_node_t<T>* child[4];
	radix_node_t<T>* parent;
	T value; //二进制  00 01 10 11
};

//基数树的头节点
template<typename T>
struct radix_head {
	//根节点
	radix_node_t<T>* root;
	size_t number; //已经存储进来多少个节点
};

template<typename T>
class radix {
public:
	typedef int k_type;
	typedef T v_type;
	radix():_pool(sizeof(radix_node_t<T>)) {
		//先申请开奖
		t = (radix_head<T>*)malloc(sizeof(radix_head<T>));
		t->number = 0;
		t->root = (radix_node_t<T>*)_pool.malloc();
		for (int i = 0; i < 4; i++)
		{
			t->root->child[i] = nullptr;
		}
		t->root->value = T();
	}

	radix_node_t<T>* find(k_type key)
	{
		int i = 0, temp;
		radix_node_t<T>* node;
		node = t->root;
		while (node && i < radix_tree_height)
		{
			temp = CHECK_BITS(key,i);
			i++;
			node = node->child[temp];
		}
		if (node == nullptr || node->value == v_type())
			return nullptr;
		return node;
	}

	int insert(k_type key, v_type value)
	{
		int i, temp;
		radix_node_t<T>* node, *child;
		node = t->root;
		for (int i = 0; i < radix_tree_height; i++)
		{
			temp = CHECK_BITS(key, i);

			if (node->child[temp] == nullptr)
			{
				child = (radix_node_t<T>*)_pool.malloc();
				if (!child) return NULL;

				//显示构造
				for (int i = 0; i < 4; i++)
				{
					child->child[i] = nullptr;
				}
				child->value = v_type();

				child->parent = node;
				node->child[temp] = child;
				node = node->child[temp];
			}
			else
				node = node->child[temp];


		}
		//已经插入返回-2
		if (node->value == value)return RADIX_INSERT_VALUE_SAME;
		//插入节点有其他值返回-3
		//if (node->value != nullptr)return RADIX_INSERT_VALUE_OCCUPY;
		node->value = value;
		//计数加1
		++t->number;
		return 0;
	}

	int Delete(k_type key)
	{
		radix_node_t<T>* tar = find(key);
		if (tar == nullptr)
		{
			cout << "删除失败" << endl;
			return RADIX_DELETE_ERROR;
		}
		else
		{
			radix_node_t<T>* par = tar->parent;
			tar->value = v_type();
			_pool.free(tar);
			--t->number;
			for (int i = 0; i < 4; i++)
			{
				if (par->child[i] == tar)
				{
					par->child[i] = nullptr;
					break;
				}
			}
		}
		return 0;
	}


private:
	radix_head<T>* t;
	boost::pool<> _pool;

};
