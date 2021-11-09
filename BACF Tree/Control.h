#pragma once
#include <limits.h>

//LEAF FROM 1 CODE

static int SVR_MODE = 0;
static int REALK = 0;

//PUT_BUFFER_FROM_BOTTOM : N 아래에서부터 N인 노드에 BUFFER 삽입
static int PUT_BUFFER_FROM_BOTTON = 1;

//디버깅 하기 위해, node에 해당 data들의 index 저장. 
static bool FOR_DEBUG_INDEX = true;

//true이면 모든 node에 buffer 
static bool ALL_BUFFER = false;

//true이면 모든 node에 buffer 삭제
static bool NO_BUFFER = false;

static bool KMEANS_TEST = true;

//leaf로 부터 n만큼 위의 노드에 buffer 삽입. 만일 1이면 leaf 바로 위, 2이면 leaf보다 2 level 위에 buffer 삽입. 
static int BUFFER_LEVEL_FROM_LEAF = 1;

//원하는 Tree의 size
unsigned long long INPUT_TREE_LIMIT_SIZE = 1024*1024*1;
//ULLONG_MAX;

bool DEBUG_PRINT_NODE = false;

//BUFFER NODE 개수
long curBufferNodeCnt = 0;

//CFPTree 각 node 제외 크기
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
