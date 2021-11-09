#pragma once
#include <limits.h>

//LEAF FROM 1 CODE

static int SVR_MODE = 0;
static int REALK = 0;

//PUT_BUFFER_FROM_BOTTOM : N �Ʒ��������� N�� ��忡 BUFFER ����
static int PUT_BUFFER_FROM_BOTTON = 1;

//����� �ϱ� ����, node�� �ش� data���� index ����. 
static bool FOR_DEBUG_INDEX = true;

//true�̸� ��� node�� buffer 
static bool ALL_BUFFER = false;

//true�̸� ��� node�� buffer ����
static bool NO_BUFFER = false;

static bool KMEANS_TEST = true;

//leaf�� ���� n��ŭ ���� ��忡 buffer ����. ���� 1�̸� leaf �ٷ� ��, 2�̸� leaf���� 2 level ���� buffer ����. 
static int BUFFER_LEVEL_FROM_LEAF = 1;

//���ϴ� Tree�� size
unsigned long long INPUT_TREE_LIMIT_SIZE = 1024*1024*1;
//ULLONG_MAX;

bool DEBUG_PRINT_NODE = false;

//BUFFER NODE ����
long curBufferNodeCnt = 0;

//CFPTree �� node ���� ũ��
long CFPTREE_PTR = 0;
long CFNODE_PTR = 0;
long CFENTRY_PTR = 0;
long BUFFER_PTR = 0;

//IO COUNT
long TotalReadCNT = 0;
long TotalWriteCNT = 0;
long TotalBufferReadCNT = 0;
long TotalBufferWriteCNT = 0;
long TotalBufferDeleteWriteCNT = 0;
long TotalAfterInsertionWriteCNT = 0;

long WithinThresholdCNT = 0;
