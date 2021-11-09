#ifndef CFPTREE_H
#define CFPTREE_H
#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
#include <cstdlib>
#define DOUBLE_MAX         1.7976931348623158e+308

using namespace std;

class LeafTable;
class CFNode;
class CFEntry;
class CFEntryPair;
class Buffer;
class NodePool;
class NodeInfo;
class CFPTree;

LeafTable* leaf_table = nullptr;

//Centroid Distance D0-4
const int D0_DIST = 0;
const int D1_DIST = 1;
const int D2_DIST = 2;
const int D3_DIST = 3;
const int D4_DIST = 4;

int dim = 0;
double currentThreshold = 0.0;
int limitLevel = 3;
unsigned int maxNodeEntries = 0;
unsigned int maxBufferEntries = 0;
unsigned int maxPoolSize = 4;
int distFunction = D0_DIST;
bool Get_LeafTable = true;

//I/O COUNT
unsigned readCnt = 0;
unsigned writeCnt = 0;

bool firstNodeWrite = true;
bool firstBufferWrite = true;

//search node's entries and if closest entry is in the memory, becomes true, else false
bool ClosestEntryinMememory = true;

//Buffer File을 훑으면서 한 개씩 내릴 때, 해당 부분이 true가 됨. 
bool flushBufferOneByOne = false;

//Buffer Flush Type
bool BUF_FLUSH_SEQ = true;
char Tree_path[50] = "C:\\Users\\admin\\Desktop\\Dataset\\TreeNode";

//NodePool I/O
char NodePool_path[50] = "NodeInfo.bin";

//Buffer I/O
char Buffer_path[50] = "BufferInfo.bin";


//Buffer 저장되어 있는 파일에서 flush 할 때 필요한 Queue
queue<CFEntry*> NotInsertedQueue;

class CFPTree {

public:
	//dummy node that points to the list of leaves. used for fast retrieval of final subclusters
	CFNode* leafListStart = NULL;
	int dim = 2;

	//depth of tree and newThreshold
	int depth = 1;
	double newThreshold = 0.0;
	int newDistFunc = 0;

	/*int maxNodeEntries = 0;
	int maxBufferEntries = 0;
	double distThreshold = 0;
	int distFunction = 0;
	int LevelLimit = 0;*/

	//NodeLinkedList* NodePool = nullptr;	

	//전체 node갯수 count, -> node_id 일정하게 1씩 증가할 수 있도록. 
	int TotalNodeCnt = 1;
	int node_id = 1;
	int rootLevel = 1;
	unsigned long long TreeLimit = 0;

	//The root node of the Diskbased Buffer CFPlus Tree
	CFNode* root = nullptr;

	NodePool* InfoPool;

	CFPTree();
	CFPTree(int maxNodeEntries, int maxBufferEntries, double distThreshold, int distFunction, bool applyMergingRefinement, int dim);
	~CFPTree(){}

	CFNode* getLeafListStart();
	int getDepth();
	long getCFPTREE_PTR_SIZE();
	//int getMaxNodeEntries();
	//double getNewThreshold();
	void setMemoryLimit(long limit);
	void setMemoryLimitMB(long limit);

	//Inserts a single pattern vector into the OCFTree
	/*bool insertEntry(double* x, string className)  {
		instanceIndex++;
		return insertEntry(x,instanceIndex, className);
	}*/

	//Insert a pattern vector with a specific associated pattern vector index.
	bool insertEntry(double* x, int index, string className);
	bool insertEntry(CFEntry* e);
	void splitRoot();	   

	void flushUnderLimitLevel();
	void flushAboveLimitLevel();
	void flushBufferFile();
	void KMEANSflushBuffer();

	void nodeWrite(CFNode* tempNode, FILE* fp);
	CFNode* nodeRead(int treeNode_ID, bool leafStatus);

	void bufferWrite(Buffer* tempBuffer);
	Buffer* bufferRead(int treeNode_ID);

	void printAllTree();
	void printAllTree(FILE* fp);

	void WriteLeafInfo();

};

class CFNode {

public:
	//vector <CFEntry*> entries; // stores the CFEntries for this node
	vector <pair <CFEntry*, bool>> entries;  // stores the CFEntries for this node, bool : if the entry child is in the memory 
	bool leafStatus = false; // if true, this is a leaf

	CFNode* nextLeaf = nullptr; // pointer to the next leaf (if not a leaf, pointer will be null)
	int nextLeafId = 0;

	CFNode* previousLeaf = nullptr; // pointer to the previous leaf (if not a leaf, pointer will be null)
	int previousLeafId = 0;

	CFEntry* parentEntry = nullptr;
	int parentId = 0;

	int id = 0; //file id
	CFPTree* cfptree = nullptr;
	Buffer* buff = nullptr;
	int depth = -1;
	int level = 1;	//CFNode의 level

	NodeInfo* NodePool_Ptr = nullptr;
	bool inNodePool = false;

	CFNode(void);
	CFNode(CFPTree* inTree, bool leafStatusl);
	CFNode(CFPTree* inTree, bool leafStatus, int NodeLevel);
	CFNode(CFPTree* inTree, bool leafStatus, int NodeLevel, int NodeID);
	~CFNode() {}

	long getCFNODE_PTR_SIZE();
	CFNode* getNextLeaf();
	bool isNextLeafNull();
	void setNextLeaf(CFNode* n);
	void setNextLeafNull();
	CFNode* getPreviousLeaf();
	bool isPreviousLeafNull();
	void setPreviousLeaf(CFNode* n);
	void setPreviousLeafNull();
	void setParentEntry(CFEntry* parent);
	void addToEntryList(CFEntry* e);
	vector<pair<CFEntry*,bool>> getEntries();
	bool insertEntry(CFEntry *e);

	void insertIntoLeafEntry(CFEntry* e);
	void insertIntoLeafEntry(vector<CFEntry*> thisNodeEntries, CFEntry* e);
	void insertIntoLeafEntry(vector<CFEntry*> thisNodeEntries, CFEntry* e, CFEntry* sumEntry);
	void insertIntoLeafEntry(CFEntry* e, CFEntry* sumEntry);
	bool insertlistEntry(vector<CFEntry*> listInsert);
	void insertlistEntryLast(vector<CFEntry*> listInsert);
	void LeafTableUpdate(vector<CFEntry*> thisNodeEntries, CFEntry* e);

	bool bufferFree();
	bool bufferFreeSeq();
	void bufferFreeLast();

	CFEntry* findClosestEntry(vector<pair<CFEntry*,bool>> thisNodeEntries, CFEntry* e);
	CFEntry* findClosestEntry(vector<pair<CFEntry*,bool>> thisNodeEntries, CFEntry* e, int *index);
	int findClosestEntryIdx(CFEntry *e);
	CFEntryPair* findFarthestEntryPair(vector<CFEntry*>entries);
	CFEntryPair* findFarthestEntryPair();
	CFEntryPair* splitEntry(CFEntry *closest);
	CFEntry* FindClosestBuffer(CFEntry* newE1, CFEntry* newE2, CFEntry* BuffEntry);

	void redistributeEntries(vector<pair<CFEntry*,bool>> oldEntries, CFEntryPair* farEntries, CFEntry* newE1, CFEntry* newE2);
	void ClosestEntry(CFEntry* newE1, CFEntry* newE2);
	CFEntry* FindClosest(CFEntry* newE1, CFEntry* newE2);
};

class CFEntry {
public:
	//int inputIndex = 0;
	vector<int> inputIndex;
	CFNode* child = nullptr;
	CFNode* parentNode = nullptr;
	int childId = 0;
	//int subclusterID = -1; // the unique id the describes a subcluster (valid only for leaf entries)
	double radius = 0.0;	// radius. only used in entries not buffers
	int n = 0; // number of patterns summarized by this entry
	double* sumX = new double[dim];
	double* sumX2 = new double[dim];
	//int* LeafTableAddr;
	/**
	 * This makes a deep copy of the CFEntry e.
	 * WARNING: we do not make a deep copy of the child!!!
	 *
	 * @param e the entry to be cloned
	 */
	CFEntry();
	CFEntry(const CFEntry &e);
	CFEntry(CFEntry *e);
	CFEntry(double* x, string className);
	CFEntry(double* x, int index, string className);
	CFEntry(int n, double* x);
	~CFEntry() {}

	bool operator== (const CFEntry &tempEntry) const {
		return (childId == tempEntry.childId && child == tempEntry.child && parentNode == tempEntry.parentNode);
	}

	bool hasChild();

	CFNode* getChild();

	void setChildAll(CFNode* n);
	void setChild(CFNode* n);
	void setChildNull();
	bool isChildNull();
	void addToChild(CFEntry* e);
	void setParentNode(CFNode* parent);
	int getChildSize();
	long getCFENTRY_PTR_SIZE();

	void setSubclusterID(int id);
	int getSubclusterID();

	double distance(CFEntry* e, int distFunction);
	double d0(CFEntry* e1, CFEntry *e2);

	void update(CFEntry e);
	void update(CFEntry *e);

	bool isWithinThreshold(CFEntry* e, double threshold, int distFunction);
};

class CFEntryPair {

public:
	CFEntry* e1;
	CFEntry* e2;

	CFEntryPair() {}
	~CFEntryPair() {}

	CFEntryPair(CFEntryPair *p);
	CFEntryPair(CFEntry e1, CFEntry e2);
	CFEntryPair(CFEntry *e1, CFEntry *e2);

	/*bool equals(CFEntryPair p) {

		if (e1.equals(p.e1) && e2.equals(p.e2))
			return true;

		if (e1.equals(p.e2) && e2.equals(p.e1))
			return true;

		return false;
	}*/
};

class Buffer {

public:
	vector<CFEntry*> buff;
	CFPTree* cfptree = nullptr;
	CFNode* treeNode_Ptr = nullptr;
	int treeNode_ID = 0;

	Buffer();
	Buffer(CFPTree *inTree);
	Buffer(CFNode n);
	~Buffer() {}

	long getBUFFER_PTR_SIZE();
};

class LeafTable {
public:
	int **Leaf_Table;

	LeafTable() {}
	LeafTable(int datasize);
	~LeafTable() {}
};

class NodeInfo
{
public:
	CFNode* TreeNode_ptr = nullptr;
	//bool* inMemory_ptr = nullptr;
	//int TreeNode_ID = 0;
	NodeInfo* prev = nullptr;
	NodeInfo* next = nullptr;

	NodeInfo(CFNode tempNode);
	NodeInfo(CFNode *tempNode);
	~NodeInfo() {}
};

class NodePool
{
public:
	NodeInfo* head = nullptr;
	NodeInfo* tail = nullptr;

	int curNodeInfoCnt = 0;
	bool doFlush = false;

	NodePool();
	~NodePool() {}
	void push_back(CFNode tempCF);
	void push_back(CFNode* tempCF);
	void remove(CFNode* tempCF);
	void hit_go_tail(CFNode tempNode);
	void NodePool_Flush();

	void nodeWrite(CFNode* tempNode, FILE* fp);
	void nodeRead(int treeNode_ID);
};

class IONode
{
public:
	int id;
	int parentId;
	int entrySize;
	//int buffEntrySize;
	bool buffFlag;
	/*vector <int> entries_N;
	vector <double*> entries_sumX;
	//vector <double*> entries_sumX2;
	vector <int> entries_childID;
	*/

	IONode();
	IONode(CFNode *curNode);
	~IONode() {}
};

class IOBuffer
{
public:
	int id;
	int buffEntrySize;

	IOBuffer();
	IOBuffer(Buffer* curBuffer);
	~IOBuffer() {}
};

#endif