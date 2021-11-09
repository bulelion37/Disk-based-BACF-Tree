#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <time.h>
#include <vector>
#include <queue>
#include "CFPTree.h"
#include "Control.h"

using namespace std;


void runLocal( );
void runCFP(string* args);
double** readNumFile(long *ds, int dim, string datasetFile, vector<string>& labels);
void SumCopy(double* src, double* dest);

char outputFileName[50] = "19634.txt";

int main()
{
	if (SVR_MODE == 0) runLocal();
	//else if (SVR_MODE == 1) runServer(args);
	//else if (SVR_MODE == 7) runServerSmallCluster(args);
	//else if (SVR_MODE == 100) runServerReal(args);
	//else if (SVR_MODE == 101) runLocalExample(args);
	return 0;
}

void runLocal()
{
	string *args = new string[7];
	args[0] = "C:\\Users\\bigdata\\Desktop\\Th1.0_Experiment\\C500_SIZE\\";	//path
	args[1] = "d10mil_9999804.txt";		//dataset file name
	args[2] = "2";			//dim
	args[3] = "1.0";			//Threshold
	args[4] = "2";			//LimitLevel
	args[5] = "10";		//maxNodeEntries
	args[6] = "6";		//maxBufferEntrie

	runCFP(args);
}

void runCFP(string* args) {
	
	string path = args[0];
	string datasetFile = args[1];
	dim = stoi(args[2]);
	currentThreshold = stod(args[3]);
	limitLevel = stoi(args[4]);
	maxNodeEntries = stoi(args[5]);
	maxBufferEntries = stoi(args[6]);

	distFunction = D0_DIST;
	bool applyMergingRefinement = true;

	//long build_strt_time = System.currentTimeMillis();
	//time_t build_strt_time = time(NULL);
	//long build_time = 0;
	//long tot_time = 0;

	//List<CFEntry> mc_list = null;

	vector<string> labels;
	path += datasetFile;
	long dataSize; 
	double** d = readNumFile(&dataSize, dim, path, labels);

	//System.out.printf("bef CFPTREe : %d ", Runtime.getRuntime().totalMemory());

	CFPTree* DiskbasedCFPTree = new CFPTree(maxNodeEntries, maxBufferEntries, currentThreshold, distFunction, applyMergingRefinement, dim);

	//DiskbasedCFPTree->setMemoryLimitMB(8192);
	//leaf_table = &LeafTable(dataSize);
	
	//CFPTree, CFNode, CFEntry 각각 크기 get
	//힙 사이즈 확인 후 class 사이즈 구하는 방법
	CFPTree forMemoryCalculateCFPTree = CFPTree();
	CFNode forMemoryCalculateCFNode = CFNode();
	CFEntry forMemoryCalculateCFEntry = CFEntry();
	Buffer forMemoryCalculateBuffer = Buffer();

	CFPTREE_PTR = sizeof(forMemoryCalculateCFPTree);
	CFNODE_PTR = sizeof(forMemoryCalculateCFNode);
	CFENTRY_PTR = sizeof(forMemoryCalculateCFEntry);
	BUFFER_PTR = sizeof(forMemoryCalculateBuffer);

	DiskbasedCFPTree->TreeLimit = CFPTREE_PTR + CFNODE_PTR;

	printf("CFPTREE SIZE : %d\nCFNODE SIZE : %d\nCFENTRY SIZE : %d\nBUFFER SIZE : %d\n", CFPTREE_PTR, CFNODE_PTR, CFENTRY_PTR, BUFFER_PTR);

	clock_t start, end; 
	double time_result;

	start = clock();

	for (int i = 0; i < dataSize; i++) {
		if (i == 149)
			i = 149;
		bool inserted = DiskbasedCFPTree->insertEntry(d[i], i, labels[i]);
			if (!inserted) {
				cout << "NOT INSERTED!" << endl;
				//system.exit(1);
			}
			//curTreeSize = CFPTREE_PTR + (DiskbasedCFPTree->node_id - 1) * (CFNODE_PTR + CFENTRY_PTR*maxNodeEntries);

			if (DiskbasedCFPTree->TreeLimit > INPUT_TREE_LIMIT_SIZE)
			{
				if (DEBUG_PRINT_NODE)
				{
					printf("---------------Before---------------------------------------\ni : %d\n", i);
					DiskbasedCFPTree->printAllTree();
					printf("-----------------------------------------------------\n\n");
				}

				printf("***************************************\n");
				printf("before flush %lld\n", DiskbasedCFPTree->TreeLimit);
				DiskbasedCFPTree->flushUnderLimitLevel();
				printf("after flush %lld\n", DiskbasedCFPTree->TreeLimit);

				if (DEBUG_PRINT_NODE)
				{
					printf("------------------After------------------------------------\ni : %d\n", i);
					DiskbasedCFPTree->printAllTree();
					printf("-----------------------------------------------------\n\n");
				}
			}
			printf("%d %lld %ld\n", i, DiskbasedCFPTree->TreeLimit, WithinThresholdCNT);

			if (DEBUG_PRINT_NODE)
			{
				printf("------------------------------------------------------\ni : %d\n", i);
				DiskbasedCFPTree->printAllTree();
				printf("-----------------------------------------------------\n\n");
			}
			delete(d[i]);
	}

	end = clock();
	time_result = (double)(end - start);
	
	long totalIO = TotalBufferReadCNT + TotalBufferWriteCNT + TotalReadCNT + TotalWriteCNT + TotalBufferDeleteWriteCNT;
	//printf("Total Time : %lf second\n", (time_result / CLOCKS_PER_SEC));
	//printf("Total I/O Count %ld\n", totalIO);


	if (KMEANS_TEST)
	{
		DiskbasedCFPTree->KMEANSflushBuffer();
		FILE* fpi = fopen("PrintAllTree.txt", "w");
		DiskbasedCFPTree->printAllTree(fpi);
		fclose(fpi);
	
		FILE* fpout = fopen(outputFileName, "w");
		fprintf(fpout,"Total Time : %lf second\n", (time_result / CLOCKS_PER_SEC));
		fprintf(fpout,"Total I/O Count %ld\n", totalIO);
		fclose(fpout);

		return;
	}
	else {
		//Insert 완료 후, LimitLevel 위의 노드들, 파일에 Write
		DiskbasedCFPTree->flushAboveLimitLevel();
		printf("done flush above\n");


		/*LimitLevel 위를 전부 파일에 쓴 후, BufferFile 탐색하여 flush.
		* 그 이유는 bufferFile 탐색할 때, 해당 노드를 파일에서 갖고 와야 하므로, 전부 파일에 있어야 함.
		* 그리고 buffer File flush시 split이 계속 되어 트리 위로 올라가야할 경우, 그렇게 하지 않고 queue에 넣으므로, flushabovelimitlevel 먼저하기.
		* buffer가 아직 flush되지 않은 파일들을 전부 flush.
		*/
		DiskbasedCFPTree->flushBufferFile();
		printf("done flush buffer\n");
	}
	
	end = clock();
	time_result = (double)(end - start);

	FILE* fpout = fopen(outputFileName, "w");
	fprintf(fpout, "Total Time : %lf second\n", (time_result / CLOCKS_PER_SEC));
	fclose(fpout);

	
		/*DiskbasedCFPTree->finishedInsertingData();
		mc_list = DiskbasedCFPTree->getLeafEntries();
		build_time = System.currentTimeMillis() - build_strt_time;
		cout << "depth[" << DiskbasedCFPTree->getDepth() << "] " << endl;

		System.out.print("entries[" + PrintUtil.f_sz.format(mc_list.size()) + "] time[" + PrintUtil.f_tm.format(build_time) + "] ");

		birchTree.calcRadius(distFunction);
		List<List<CFEntry>> mcs_list = birchTree.getMCSwRangeSearch(distFunction, currentThreshold);
		k = mcs_list.size();
		REALK = k;
		tot_time = System.currentTimeMillis() - build_strt_time;
		System.out.print("k[" + PrintUtil.f_sz.format(k) + "] ");*/
}

double** readNumFile(long* ds, int dim, string datasetFile, vector <string>& labels)
{
	/*
	*dimension 만큼의 double 파일 입력 받아서 
	*d에 저장 후, 해당 cluster 정보는 labels에 저장. 
	labels의 index는 들어온 index, 정보는 cluster 정보
	*/
	long dataSize = 0;
	char path[100];
	strcpy(path, datasetFile.c_str());

	FILE *fp = fopen(path, "r");

	if (fp == nullptr){
		cout << "파일을 여는 데 실패했습니다." << endl;
		//return -1;
	}
	
	while (!feof(fp)){
		char buff[100];
		fgets(buff, 100, fp);

		char *ptr = strtok(buff, " ");    //첫번째 strtok 사용.
		int x = dim;

		while (x>0)              //ptr이 NULL일때까지 (= strtok 함수가 NULL을 반환할때까지)
		{
			ptr = strtok(NULL, " ");     //자른 문자 다음부터 구분자 또 찾기
			x--;
		}
		string line(ptr);
		labels.push_back(line);
		dataSize++;
	}
	fclose(fp);
	
	*ds = dataSize;

	double** d = (double**)malloc(sizeof(double*)*dataSize);

	FILE *fp1 = fopen(path, "r");

	if (fp1 == NULL) {
		cout << "파일을 여는 데 실패했습니다." << endl;
		//return -1;
	}

	int i = 0;
	while (!feof(fp1)) {

		d[i] = (double*)malloc(sizeof(double)*dim);

		char buff[100];
		fgets(buff, 100, fp1);

		int j = 0;
		while (1)              
		{
			if (j == 0)
			{
				char *ptr = strtok(buff, " ");    //첫번째 strtok 사용.
				d[i][j] = atof(buff);
			}
			j++;

			if (j == dim)
				break;

			char *ptr = strtok(NULL, " ");     //자른 문자 다음부터 구분자 또 찾기
			d[i][j] = atof(ptr);
		}
		i++;
	}
	fclose(fp1);

	return d;
	//return dataSize;
}

//Class CFPTree
CFPTree::CFPTree(void) {}

CFPTree::CFPTree(int maxNodeEntries, int maxBufferEntries, double distThreshold, int distFunction, bool applyMergingRefinement, int dim) {
	maxNodeEntries = maxNodeEntries;
	maxBufferEntries = maxBufferEntries;
	distThreshold = distThreshold;
	newDistFunc = distFunction;
	dim = dim;

	this->root = new CFNode(this, applyMergingRefinement, true);
	this->leafListStart = root;
	this->InfoPool = new NodePool();
}


//Gets the start of the list of leaf nodes (remember: the first node is a dummy node)
CFNode* CFPTree::getLeafListStart() {
	return this->leafListStart;
}


//Gets the depth of cftree and threshold
int CFPTree::getDepth() { return this->depth; }

long CFPTree::getCFPTREE_PTR_SIZE()
{
	long size = 0;
	size = sizeof(this->leafListStart) + sizeof(this->dim) + sizeof(this->depth) + sizeof(this->newThreshold) + sizeof(this->newDistFunc)
		+ sizeof(this->TotalNodeCnt) + sizeof(this->node_id) + sizeof(this->rootLevel) + sizeof(this->TreeLimit) + sizeof(this->root) + sizeof(this->InfoPool);

	return size;
}

//@param limit memory limit in bytes
void CFPTree::setMemoryLimit(long limit) {
	this->TreeLimit = limit;
}

//@param limit memory limit in Mbytes
void CFPTree::setMemoryLimitMB(long limit) {
	this->TreeLimit = limit * 1024 * 1024;
}

//Inserts a single pattern vector into the OCFTree
/*bool insertEntry(double* x, string className)  {
	instanceIndex++;
	return insertEntry(x,instanceIndex, className);
}*/



//Insert a pattern vector with a specific associated pattern vector index.
bool CFPTree::insertEntry(double* x, int index, string className) {
	CFEntry* e = new CFEntry(x, index, className);
	this->TreeLimit += CFENTRY_PTR;
	return insertEntry(e);
}


//Inserts an entire CFEntry into the tree. Used for tree rebuilding.
bool CFPTree::insertEntry(CFEntry *e) {

	bool dontSplit = this->root->insertEntry(e);
	//		this.addNodeToLazyBuffer(root);
	if (!dontSplit) {
		// if dontSplit is false, it means there was not enough space to insert the new entry in the tree, 
		// therefore wee need to split the root to make more room
		splitRoot();

	}
	return true; // after root is split, we are sure x was inserted correctly in the tree, and we return true
}

////Splits the root to accommodate a new entry. The height of the tree grows by one.
void CFPTree::splitRoot() {

	// the split happens by finding the two entries in this node that are the most far apart
	// we then use these two entries as a "pivot" to redistribute the old entries into two new nodes

	this->depth++;

	if (this->depth > limitLevel)
		this->InfoPool = new NodePool();

	CFNode* curRoot = this->root;
	CFEntryPair* p = nullptr;
	p = curRoot->findFarthestEntryPair();

	//newEntry1, 2는 새로운 root node에서 아래를 가리키는 CFEntry
	CFEntry* newEntry1 = new CFEntry();
	//CFNode newNode1 = new CFNode(this, root.getMaxNodeEntries(),root.getDistThreshold(),root.getDistFunction(),root.applyMergingRefinement(),root.isLeaf());
	this->TreeLimit += CFENTRY_PTR;

	//root보다 level이 1 더 많은 CFNode1 생성. 
	CFNode* newNode1 = new CFNode(this, curRoot->leafStatus, curRoot->level + 1);
	newEntry1->setChildAll(newNode1);
	newNode1->setParentEntry(newEntry1);

	CFEntry* newEntry2 = new CFEntry();
	//CFNode newNode2 = new CFNode(this, root.getMaxNodeEntries(),root.getDistThreshold(),root.getDistFunction(),root.applyMergingRefinement(),root.isLeaf());
	this->TreeLimit += CFENTRY_PTR;

	//root보다 level이 1 더 많은 CFNode2 생성. 
	CFNode* newNode2 = new CFNode(this, curRoot->leafStatus, curRoot->level + 1);
	newEntry2->setChildAll(newNode2);
	newNode2->setParentEntry(newEntry2);

	// redistributes the entries in the root between newEntry1 and newEntry2
	// according to the distance to p.e1 and p.e2
	newEntry1->addToChild(p->e1);
	newEntry1->update(p->e1);
	newEntry1->childId = newNode1->id;
	newEntry2->addToChild(p->e2);
	newEntry2->update(p->e2);
	newEntry2->childId = newNode2->id;

	curRoot->redistributeEntries(curRoot->getEntries(), p, newEntry1, newEntry2);

	delete(p);

	//root.redistributeEntries(root.getEntries(),p,newEntry1,newEntry2);


	// the new root that hosts the new entries
	CFNode* newRoot = new CFNode(this, false, root->level, root->id);
	newRoot->addToEntryList(newEntry1);
	newEntry1->setParentNode(newRoot);
	newRoot->addToEntryList(newEntry2);
	newEntry2->setParentNode(newRoot);

	//자식 노드에 parentNode Id 추가. 
	if (newEntry1->parentNode != nullptr)
		newNode1->parentId = newEntry1->parentNode->id;
	if (newEntry2->parentNode != nullptr)
		newNode2->parentId = newEntry2->parentNode->id;

	// this updates the pointers to the list of leaves
	if (root->leafStatus == true) { // if root was a leaf
		leafListStart->setNextLeaf(newNode1);
		newNode1->setPreviousLeaf(leafListStart);
		newNode1->setNextLeaf(newNode2);
		newNode2->setPreviousLeaf(newNode1);

		if (Get_LeafTable)
		{//새롭게 split된 p.e1과 p.e2에 대한 LeafTable 정보 update.
			//newNode1->LeafTableUpdate(newNode1->entries, p->e1);
			//newNode2->LeafTableUpdate(newNode2->entries, p->e2);
		}
	}

	CFNode* pastRoot = this->root;

	//memory deallocate pastRoot's entries vector
	vector <pair <CFEntry*, bool>>().swap(this->root->entries);

	if (pastRoot->buff != nullptr)
	{
		vector<CFEntry*>().swap(pastRoot->buff->buff);
		delete(pastRoot->buff);
		this->TreeLimit -= BUFFER_PTR;
	}

	//memory deallocate pastRoot
	delete(pastRoot);
	this->TreeLimit -= CFNODE_PTR;

	// updates the root
	this->root = newRoot;
}

void CFPTree::nodeWrite(CFNode* tempNode, FILE* fp)
{
	TotalWriteCNT++;

	//Node의 entry에 해당하는 (N,LS,SS), entry의 childID 정보 저장 
	vector <int> entries_N(maxNodeEntries);
	vector <double*>entries_sumX(maxNodeEntries);
	vector <int>entries_childID(maxNodeEntries);
	int size = tempNode->entries.size();
	for (unsigned int i = 0; i < size ; i++)
	{
		CFEntry* temp = tempNode->entries[i].first;
		entries_N[i] = temp->n;
		entries_sumX[i] = temp->sumX;
		entries_childID[i] = temp->childId;
	}

	//Node의 buffer에 해당하는 (N,LS,SS) 정보 저장. buffer의 N은 무조건 1
	/*vector <double*>buffer_sumX(maxBufferEntries);
	int buffsize = 0; 
	if (tempNode->buff != nullptr)
		buffsize = tempNode->buff->buff.size();

	for (unsigned int i = 0; i < buffsize; i++)
	{
		CFEntry* temp = tempNode->buff->buff[i];
		buffer_sumX[i] = temp->sumX;
	}*/

	/*fprintf(fp, "%8x%p%2x", tempNode->id, tempNode->buff, tempNode->entries.size());
	int i = 0;
	for (i = 0; i < tempNode->entries.size(); i++)
	{
		CFEntry* temp = tempNode->entries[i].first;
		fprintf(fp, "%8x%.6lf%.6lf", temp->n, temp->sumX, temp->sumX2);
	}
	int noneint = 0;
	double nonedouble = 0.0;
	for(i ; i< maxNodeEntries ; i++)
		fprintf(fp, "%8x%.6lf%.6lf", noneint, nonedouble, nonedouble);*/
	IONode writeNode = IONode(tempNode);
	long oneNodeSize = sizeof(writeNode) + (sizeof(int) + sizeof(double*) + sizeof(int)) * maxNodeEntries;
	//+sizeof(double*) * maxBufferEntries;

	fseek(fp, oneNodeSize * writeNode.id, SEEK_SET);
	fwrite(&writeNode, sizeof(writeNode), 1, fp);
	fwrite(&(entries_N[0]), sizeof(int), maxNodeEntries, fp);
	fwrite(&(entries_sumX[0]), sizeof(double*), maxNodeEntries, fp);
	fwrite(&(entries_childID[0]), sizeof(int), maxNodeEntries, fp);
	//fwrite(&(buffer_sumX[0]), sizeof(double*), maxBufferEntries, fp);

	//memory deallocate vectors.
	vector<int>().swap(entries_N);
	vector<double*>().swap(entries_sumX);
	vector<int>().swap(entries_childID);
}
CFNode* CFPTree::nodeRead(int treeNode_ID, bool leafStatus)
{
	TotalReadCNT++;

	FILE* fp = fopen(NodePool_path, "rb");
	IONode readNode;
	long oneNodeSize = sizeof(readNode) + (sizeof(int) + sizeof(double*) + sizeof(int)) * maxNodeEntries;
	//+sizeof(double*) * maxBufferEntries;
	fseek(fp, oneNodeSize * treeNode_ID, SEEK_SET);
	fread(&readNode, sizeof(readNode), 1, fp);
	
	vector <int> entries_N(maxNodeEntries);
	vector <double*>entries_sumX(maxNodeEntries);
	vector <int>entries_childID(maxNodeEntries);
	//vector <double*>buffer_sumX(maxBufferEntries);
	fread(&(entries_N[0]), sizeof(int), maxNodeEntries, fp);
	fread(&(entries_sumX[0]), sizeof(double*), maxNodeEntries, fp);
	fread(&(entries_childID[0]), sizeof(int), maxNodeEntries, fp);
	//fread(&(buffer_sumX[0]), sizeof(double*), maxBufferEntries, fp);

	fclose(fp);

	CFNode* gotNode = new CFNode();
	gotNode->cfptree = this;
	gotNode->leafStatus = leafStatus;
	gotNode->id = readNode.id;
	gotNode->parentId = readNode.parentId;
	gotNode->entries = vector<pair<CFEntry*, bool>>(maxNodeEntries);

	for (int i = 0; i < readNode.entrySize; i++)
	{
		if (entries_N[i] == NULL)
			i = i;
		CFEntry* newEntry = new CFEntry(entries_N[i], entries_sumX[i]);
		gotNode->entries[i] = make_pair(newEntry, false);
		newEntry->childId = entries_childID[i];
		newEntry->parentNode = gotNode;
	}
	if (readNode.entrySize != maxNodeEntries)
	{
		int cnt = maxNodeEntries - readNode.entrySize;
		while(cnt--)
			gotNode->entries.pop_back();
	}

	//buffer가 존재한 node의 경우
	if (readNode.buffFlag) {
		Buffer* newBuf = new Buffer(this);
		newBuf = bufferRead(gotNode->id);
		newBuf->treeNode_Ptr = gotNode;
		gotNode->buff = newBuf;

		/*newBuf->treeNode_ID = gotNode->id;
		newBuf->treeNode_Ptr = gotNode;

		for (int i = 0; i < readNode.buffEntrySize; i++)
		{
			CFEntry* newEntry = new CFEntry(1, buffer_sumX[i]);
			newBuf->buff[i] = newEntry;
		}
		if (readNode.buffEntrySize != maxBufferEntries)
		{
			int cnt = maxBufferEntries - readNode.buffEntrySize;
			while (cnt--)
				newBuf->buff.pop_back();
		}*/
	}
	else
		gotNode->buff = nullptr;

	/*if (readNode.buffEntrySize > 0)
	{
		Buffer* newBuf = new Buffer(this);
		newBuf->treeNode_ID = gotNode->id;
		newBuf->treeNode_Ptr = gotNode;

		for (int i = 0; i < readNode.buffEntrySize; i++)
		{
			CFEntry* newEntry = new CFEntry(1, buffer_sumX[i]);
			newBuf->buff[i] = newEntry;
		}
		if (readNode.buffEntrySize != maxBufferEntries)
		{
			int cnt = maxBufferEntries - readNode.buffEntrySize;
			while (cnt--)
				newBuf->buff.pop_back();
		}
		gotNode->buff = newBuf;
	}*/

	return gotNode;
}

void CFPTree::bufferWrite(Buffer* tempBuffer)
{
	TotalBufferWriteCNT++;
	int deletedBufferEntryCnt = 0;

	FILE* fpbuf = fopen(Buffer_path, "rb+");

	if (fpbuf == nullptr || firstBufferWrite)
	{
		fpbuf = fopen(Buffer_path, "wb");
		fclose(fpbuf);
		fpbuf = fopen(Buffer_path, "rb+");
		firstBufferWrite = false;
	}

	vector <double*>entries_sumX(maxBufferEntries);
	
	int size = tempBuffer->buff.size();
	for (unsigned int i = 0; i < size; i++)
	{
		CFEntry* temp = tempBuffer->buff[i];
		entries_sumX[i] = temp->sumX;
		deletedBufferEntryCnt++;
		delete(temp);
	}

	IOBuffer writeBuffer = IOBuffer(tempBuffer);
	long oneBufferSize = sizeof(writeBuffer) + (sizeof(double*)) * maxBufferEntries;

	fseek(fpbuf, oneBufferSize * writeBuffer.id, SEEK_SET);
	fwrite(&writeBuffer, sizeof(writeBuffer), 1, fpbuf);
	fwrite(&(entries_sumX[0]), sizeof(double*), maxBufferEntries, fpbuf);
	
	fclose(fpbuf);

	//memory deallocate vector
	vector<double*>().swap(entries_sumX);

	this->TreeLimit -= CFENTRY_PTR * deletedBufferEntryCnt;
}

Buffer* CFPTree::bufferRead(int treeNode_ID)
{
	TotalBufferReadCNT++;

	FILE* fpbuf = fopen(Buffer_path, "rb+");
	IOBuffer readBuffer;
	long oneBufferSize = sizeof(readBuffer) + (sizeof(double*)) * maxBufferEntries;
	fseek(fpbuf, oneBufferSize * treeNode_ID, SEEK_SET);
	fread(&readBuffer, sizeof(readBuffer), 1, fpbuf);

	vector <double*>entries_sumX(maxBufferEntries);
	fread(&(entries_sumX[0]), sizeof(double*), maxBufferEntries, fpbuf);

	//id가 -1이면 삭제된 buffer이므로 바로 close, -1이 아닐 경우에만 읽어주고 다시 삭제시켜줌. 
	if (readBuffer.id != -1) {
		//buffer 부분을 읽었으니, 해당 부분의 id에 -1을 써주어, 삭제되었음을 알 수 있게 함. 
		int delteKey = -1;
		fseek(fpbuf, oneBufferSize * treeNode_ID, SEEK_SET);
		fwrite(&delteKey, sizeof(delteKey), 1, fpbuf);
		TotalBufferDeleteWriteCNT++;
	}
	//삭제된 경우이므로, 0으로 초기화 시켜줌. 
	else
		readBuffer.buffEntrySize = 0;

	fclose(fpbuf);

	Buffer* gotBuffer = new Buffer();
	gotBuffer->cfptree = this;
	gotBuffer->treeNode_ID = treeNode_ID;
	gotBuffer->buff = vector<CFEntry*>(maxBufferEntries);
	
	for (int i = 0; i < readBuffer.buffEntrySize; i++)
	{
		CFEntry* newEntry = new CFEntry(1, entries_sumX[i]);
		gotBuffer->buff[i] = newEntry;
	}

	if (readBuffer.buffEntrySize != maxBufferEntries)
	{
		int cnt = maxBufferEntries - readBuffer.buffEntrySize;
		while (cnt--)
			gotBuffer->buff.pop_back();
	}

	return gotBuffer;
}

void CFPTree::flushUnderLimitLevel()
{
	long deletedNodeCnt = 0;
	long deletedBufferCnt = 0;
	long deletedEntryCnt = 0;

	/*주의 !!
	* r+b 또는 rb+ : 읽고 쓰기 위해 2진 파일을 엽니다. 파일이 있어야 합니다.
	* w+b 또는 wb+ : 읽고 쓰기 위해 비어 있는 2진 파일을 작성합니다. 파일이 있으면 논리 파일이 아닌 경우 해당 컨텐츠를 지웁니다.
	* wb+는 컨텐츠 지우므로, rb+로 해야 함!!
	*/
	
	FILE* fp = fopen(NodePool_path, "rb+");

	if (fp == NULL || firstNodeWrite)	
	{
		fp = fopen(NodePool_path, "wb");
		fclose(fp);
		fp = fopen(NodePool_path, "rb+");
		firstNodeWrite = false;
	}
	
	//Level Order Traversal Of CFPTree
	if (this->root == NULL)
		return;
	
	queue <CFNode*> q;
	q.push(this->root);
	while (!q.empty())
	{
		int n = q.size();

		// If this node has children 
		while (n > 0)
		{
			// Dequeue an item from queue and print it 
			CFNode* p = q.front();
			q.pop();
			
			//insert 시 node에 방문했을 때, 부모와 level이 같으면 1씩 증가하는 방식으로 level이 다뤄짐
			//flush 할 때 방문 안 한 노드가 있을 수 있으므로, 해당 방법으로 진행. 
			/*if (p != this->root)
			{
				if (p->level == p->parentEntry->parentNode->level)
					p->level++;
			}*/

			// Enqueue all children of the dequeued item 
			// p가 leaf Node가 아닌 경우에만 traversal
			if (!p->leafStatus)
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					//insert 시 node에 방문했을 때, 부모와 level이 같으면 1씩 증가하는 방식으로 level이 다뤄짐
					//flush 할 때 방문 안 한 노드가 있을 수 있으므로, 해당 방법으로 진행. 
					//단, 파일에 쓰고 난 후, 다시 진행될 때, 메모리에 없을 수 있으므로, 메모리에 존재할 때만 아래 조건 진행
					if (p->entries[i].second && p->entries[i].first->child != nullptr) {
						if (p->entries[i].first->child->level <= p->level)
							p->entries[i].first->child->level = p->level+1;
					}

					//nullptr이면 메모리 상에 없는 것이므로 탐색 하지 X
					if(p->entries[i].first->child != nullptr)
						q.push(p->entries[i].first->child);
				}
			}

			if (p->level == limitLevel + 1)
			{
				p->parentEntry->child = nullptr;
				//p의 parentNode에 entries의 second 값들이 true이면 false로 바꿔줌. 
				int size = p->parentEntry->parentNode->entries.size();
				for (int i = 0; i < size; i++)
					p->parentEntry->parentNode->entries[i].second = false;
			}

			if (p->level > limitLevel)
			{
				//만일 buffer가 존재한 node를 write했다면 해당 버퍼 삭제한 개수 count
				if (p->level == this->depth - BUFFER_LEVEL_FROM_LEAF && p->buff != nullptr)
				{
					this->bufferWrite(p->buff);
					delete(p->buff);
					deletedBufferCnt++;
				}

				this->nodeWrite(p, fp);
				deletedNodeCnt++;

				//생성한 p의 CFEntry들을 모두 delete
				for (int i = 0; i < p->entries.size(); i++)
				{
					delete(p->entries[i].first);
					deletedEntryCnt++;
				}
				//CFNode p delete
				delete(p);
			}

			n--;
		}
	}
	
	fclose(fp);
	this->TreeLimit -= ( (deletedNodeCnt * CFNODE_PTR) + (CFENTRY_PTR * deletedEntryCnt) + (deletedBufferCnt * BUFFER_PTR));
	queue<CFNode*> empty;
	swap(q, empty);
}

void CFPTree::flushAboveLimitLevel()
{

	/*주의 !!
	* r+b 또는 rb+ : 읽고 쓰기 위해 2진 파일을 엽니다. 파일이 있어야 합니다.
	* w+b 또는 wb+ : 읽고 쓰기 위해 비어 있는 2진 파일을 작성합니다. 파일이 있으면 논리 파일이 아닌 경우 해당 컨텐츠를 지웁니다.
	* wb+는 컨텐츠 지우므로, rb+로 해야 함!!
	*/

	FILE* fp = fopen(NodePool_path, "rb+");

	//Level Order Traversal Of CFPTree
	if (this->root == NULL)
		return;

	queue <CFNode*> q;
	q.push(this->root);
	while (!q.empty())
	{
		int n = q.size();

		// If this node has children 
		while (n > 0)
		{
			// Dequeue an item from queue and print it 
			CFNode* p = q.front();
			q.pop();

			// Enqueue all children of the dequeued item 
			// p가 leaf Node가 아닌 경우에만 traversal
			if (!p->leafStatus)
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					//nullptr이면 메모리 상에 없는 것이므로 탐색 하지 X
					if (p->entries[i].first->child != nullptr) {
						if (p->entries[i].first->child->level <= limitLevel)
							q.push(p->entries[i].first->child);
					}
				}
			}

			this->nodeWrite(p, fp);
			TotalAfterInsertionWriteCNT++;

			//생성한 p의 CFEntry들을 모두 delete
			for (int i = 0; i < p->entries.size(); i++)
				delete(p->entries[i].first);
			//CFNode p delete
			delete(p);

			n--;
		}
	}
	fclose(fp);
	queue<CFNode*> empty;
	swap(q, empty);
}

void CFPTree::flushBufferFile()
{
	/*
	* buffer file을 읽어서 남아 있는 buffer 들을 전부 flush 하는 함수.
	*/

	//Buffer를 하나씩 flush 해주는 flag true로 변환.
	flushBufferOneByOne = true;

	TotalBufferReadCNT++;

	FILE* fpbuf = fopen(Buffer_path, "rb+");
	if (fpbuf == nullptr)
		printf("No such File!\n");

	IOBuffer readBuffer;
	long oneBufferSize = sizeof(readBuffer) + (sizeof(double*)) * maxBufferEntries;
	
	//bufferFileSize에 파일 사이즈 저장. 
	//fseek(fpbuf, 0, SEEK_END);
	//long bufferFileSize = ftell(fpbuf);

	//fseek(fpbuf, 0, SEEK_SET);
	//long curFilePtr = ftell(fpbuf);
	int cnt = 0;
	while(1)
	{
		if (feof(fpbuf))
			break;
		if(fread(&readBuffer, sizeof(readBuffer), 1, fpbuf) < 0)
			break;
		vector <double*>entries_sumX(maxBufferEntries);
		fread(&(entries_sumX[0]), sizeof(double*), maxBufferEntries, fpbuf);

		//printf("%d %d %d \n", cnt++, readBuffer.id, readBuffer.buffEntrySize);
		
		//id가 -1이면 삭제된 buffer이므로 바로 close, -1이 아닐 경우에만 읽어주고 다시 삭제시켜줌. 
		if (readBuffer.id > 0 && readBuffer.buffEntrySize >0 && cnt == readBuffer.id) {
			//buffer 부분을 읽었으니, 해당 부분의 id에 -1을 써주어, 삭제되었음을 알 수 있게 함. 
			//int delteKey = -1;
			//fseek(fpbuf, oneBufferSize * readBuffer.id, SEEK_SET);
			//fwrite(&delteKey, sizeof(delteKey), 1, fpbuf);
			//TotalBufferDeleteWriteCNT++;

			Buffer* gotBuffer = new Buffer();
			gotBuffer->cfptree = this;
			gotBuffer->treeNode_ID = readBuffer.id;
			gotBuffer->buff = vector<CFEntry*>(maxBufferEntries);

			for (int i = 0; i < readBuffer.buffEntrySize; i++)
			{
				CFEntry* newEntry = new CFEntry(1, entries_sumX[i]);
				gotBuffer->buff[i] = newEntry;
			}

			if (readBuffer.buffEntrySize != maxBufferEntries)
			{
				int cnt = maxBufferEntries - readBuffer.buffEntrySize;
				while (cnt--)
					gotBuffer->buff.pop_back();
			}

			CFNode *curNode = nodeRead(gotBuffer->treeNode_ID, false);
			curNode->buff = gotBuffer;
			
			curNode->bufferFree();

		}
		//삭제된 경우이므로, 0으로 초기화 시켜줌. 
		else
			readBuffer.buffEntrySize = 0;

		//fseek(fpbuf, oneBufferSize, SEEK_CUR);
		//curFilePtr = ftell(fpbuf);
		cnt++;
	}
	

	fclose(fpbuf);
}

void CFPTree::KMEANSflushBuffer()
{
	FILE* fpleaf = fopen("LeafNodeInfo.txt", "w");
	//Level Order Traversal Of CFPTree
	if (this->root == NULL)
		return;

	queue <CFNode*> q;
	q.push(this->root);
	while (!q.empty())
	{
		int n = q.size();

		// If this node has children 
		while (n > 0)
		{
			// Dequeue an item from queue and print it 
			CFNode* p = q.front();
			q.pop();

			
			if (p->buff != nullptr)
				p->bufferFreeLast();
			if (!p->leafStatus)
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					if (p->entries[i].second && p->entries[i].first->child != nullptr) {
						if (p->entries[i].first->child->level <= p->level)
							p->entries[i].first->child->level = p->level + 1;
					}

					//nullptr이면 메모리 상에 없는 것이므로 탐색 하지 X
					if (p->entries[i].first->child != nullptr) {
						q.push(p->entries[i].first->child);
					}
				}
			}
			else
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					int n = p->entries[i].first->n;
					double sumX = p->entries[i].first->sumX[0];
					double sumY = p->entries[i].first->sumX[1];

					for (int j = 0; j < p->entries[i].first->inputIndex.size(); j++)
					{
						if(j != p->entries[i].first->inputIndex.size()-1)
							fprintf(fpleaf, "%d/", p->entries[i].first->inputIndex[j]);
						else
							fprintf(fpleaf, "%d|", p->entries[i].first->inputIndex[j]);
					}
					fprintf(fpleaf, "%lf %lf\n", sumX / n, sumY / n);
				}
			}
			
			n--;
		}
	}
	queue<CFNode*> empty;
	swap(q, empty);
	fclose(fpleaf);
}

void CFPTree::printAllTree()
{

	//Level Order Traversal Of CFPTree
	if (this->root == NULL)
		return;

	queue <CFNode*> q;
	q.push(this->root);
	int nodeNum = 0;
	int entryNum = 0;
	int bufferNum = 0;
	while (!q.empty())
	{
		int n = q.size();

		// If this node has children 
		while (n > 0)
		{
			// Dequeue an item from queue and print it 
			CFNode* p = q.front();
			q.pop();

			printf("/////////////////////////////////////\n");
			printf("Node ID : %d, Level : %d \n", p->id, p->level);
			nodeNum++;
			if (p->buff != nullptr)
			{
				for (int i = 0; i < p->buff->buff.size(); i++)
				{
					printf("Buffer entry : %d %d %lf %lf\n", p->buff->buff[i]->inputIndex[0], p->buff->buff[i]->n, p->buff->buff[i]->sumX[0], p->buff->buff[i]->sumX[1]);
					entryNum++;
				}
				bufferNum++;
			}
			if (!p->leafStatus)
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					if (p->entries[i].second && p->entries[i].first->child != nullptr) {
						if (p->entries[i].first->child->level <= p->level)
							p->entries[i].first->child->level = p->level + 1;
					}

					printf("Child Node ID : %d ", p->entries[i].first->childId);
					entryNum++;
					//nullptr이면 메모리 상에 없는 것이므로 탐색 하지 X
					if (p->entries[i].first->child != nullptr) {
						q.push(p->entries[i].first->child);
						printf(" : on memory \n");
					}
					else
						printf(" : on disk \n");
				}
			}
			else
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					printf("Leaf entry : ");
					for(int j = 0; j < p->entries[i].first->inputIndex.size(); j++)
						printf("%d/", p->entries[i].first->inputIndex[j]);
					printf("(N,LS,SS) : (%d,%lf,%lf)\n", p->entries[i].first->n, p->entries[i].first->sumX[0], p->entries[i].first->sumX[1]);
					entryNum++;
				}
			}
			printf("/////////////////////////////////////\n\n");

			
			n--;
		}
	}
	printf("Total Node : %d, Entry : %d, Buffer : %d\n", nodeNum, entryNum, bufferNum);
	queue<CFNode*> empty;
	swap(q, empty);
}

void CFPTree::printAllTree(FILE *fp)
{
	//Level Order Traversal Of CFPTree
	if (this->root == NULL)
		return;

	queue <CFNode*> q;
	q.push(this->root);
	while (!q.empty())
	{
		int n = q.size();

		// If this node has children 
		while (n > 0)
		{
			// Dequeue an item from queue and print it 
			CFNode* p = q.front();
			q.pop();

			fprintf(fp, "/////////////////////////////////////\n");
			fprintf(fp, "Node ID : %d, Level : %d Leafstatus %d \n", p->id, p->level, p->leafStatus);
			if (p->buff != nullptr)
			{
				for (int i = 0; i < p->buff->buff.size(); i++)
					fprintf(fp, "Buffer entry : %d %d %lf %lf\n", p->buff->buff[i]->inputIndex, p->buff->buff[i]->n, p->buff->buff[i]->sumX[0], p->buff->buff[i]->sumX[1]);
			}
			if (!p->leafStatus)
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					if (p->entries[i].second && p->entries[i].first->child != nullptr) {
						if (p->entries[i].first->child->level <= p->level)
							p->entries[i].first->child->level = p->level + 1;
					}

					fprintf(fp, "Child Node ID : %d ", p->entries[i].first->childId);
					//nullptr이면 메모리 상에 없는 것이므로 탐색 하지 X
					if (p->entries[i].first->child != nullptr) {
						q.push(p->entries[i].first->child);
						fprintf(fp, " : on memory \n");
					}
					else
						fprintf(fp, " : on disk \n");
				}
			}
			else
			{
				for (int i = 0; i < p->entries.size(); i++)
				{
					fprintf(fp, "Leaf entry : %d %d %lf %lf\n", p->entries[i].first->inputIndex, p->entries[i].first->n, p->entries[i].first->sumX[0], p->entries[i].first->sumX[1]);
				}
			}
			fprintf(fp,"/////////////////////////////////////\n\n");


			n--;
		}
	}
	queue<CFNode*> empty;
	swap(q, empty);
}

//Class CFNode
CFNode::CFNode(void) {}
CFNode::CFNode(CFPTree* inTree, bool leafStatusl)
{
	this->cfptree = inTree;
	this->id = inTree->node_id++;
	//id가 증가되었다는 것은 Node가 추가된 것. 
	inTree->TreeLimit += CFNODE_PTR;

	this->entries = vector<pair<CFEntry*,bool>>(maxNodeEntries);
	this->leafStatus = leafStatus;
}

CFNode::CFNode(CFPTree* inTree, bool leafStatus, int NodeLevel) {

	this->cfptree = inTree;
	this->leafStatus = leafStatus;
	this->id = inTree->node_id++;
	//id가 증가되었다는 것은 Node가 추가된 것. 
	inTree->TreeLimit += CFNODE_PTR;

	this->level = NodeLevel;
	inTree->TotalNodeCnt++;
	
	if (ALL_BUFFER && this->cfptree->depth != 1)
	{
		this->buff = new Buffer(inTree);
		this->buff->treeNode_ID = this->id;
		inTree->TreeLimit += BUFFER_PTR;
	}
	else {
		if(NO_BUFFER)
			this->buff = nullptr;
		else if (inTree->depth > BUFFER_LEVEL_FROM_LEAF && this->level == inTree->depth - BUFFER_LEVEL_FROM_LEAF)
		{
			this->buff = new Buffer(inTree);
			this->buff->treeNode_ID = this->id;
			inTree->TreeLimit += BUFFER_PTR;
		}
		else
			this->buff = nullptr;
	}
}

//root or 어떤 Node를 split 할 때, 새로운 node를 생성하되 id는 같아야함.  
CFNode::CFNode(CFPTree* inTree, bool leafStatus, int NodeLevel, int NodeID) {

	this->cfptree = inTree;
	this->leafStatus = leafStatus;
	this->id = NodeID;
	this->level = NodeLevel;
	inTree->TreeLimit += CFNODE_PTR;

	if (ALL_BUFFER && this->cfptree->depth != 1)
	{
		this->buff = new Buffer(inTree);
		inTree->TreeLimit += BUFFER_PTR;
	}
	else {
		if (NO_BUFFER)
			this->buff = nullptr;
		else if (inTree->depth > BUFFER_LEVEL_FROM_LEAF && this->level == inTree->depth - BUFFER_LEVEL_FROM_LEAF)
		{
			this->buff = new Buffer(inTree);
			inTree->TreeLimit += BUFFER_PTR;
		}
		else
			this->buff = nullptr;
	}
}

long CFNode::getCFNODE_PTR_SIZE()
{
	long size = 0;
	size = sizeof(this->entries) + sizeof(this->leafStatus) + sizeof(this->nextLeaf) + sizeof(this->nextLeafId) + sizeof(this->previousLeaf)
		+ sizeof(this->previousLeafId) + sizeof(this->parentEntry) + sizeof(this->parentId)+ sizeof(this->id) + sizeof(this->cfptree) + sizeof(this->buff) + sizeof(this->depth)
		+ sizeof(this->level) + sizeof(this->NodePool_Ptr) + sizeof(this->inNodePool) ;

	return size;
}
CFNode* CFNode::getNextLeaf() {
	return this->nextLeaf;
}
bool CFNode::isNextLeafNull() {
	return this->nextLeaf == nullptr;
}
void CFNode::setNextLeaf(CFNode* n) {
	if (n == nullptr)
		this->nextLeafId = 0;
	else
		this->nextLeafId = n->id;
	this->nextLeaf = n;
}
void CFNode::setNextLeafNull() {
	this->nextLeaf = nullptr;
}

CFNode* CFNode::getPreviousLeaf() {
	return this->previousLeaf;
}
bool CFNode::isPreviousLeafNull() {
	return this->previousLeaf == nullptr;
}
void CFNode::setPreviousLeaf(CFNode* n) {
	this->previousLeaf = n;
}
void CFNode::setPreviousLeafNull() {
	this->previousLeaf = nullptr;
}

void CFNode::addToEntryList(CFEntry* e) {
	this->entries.push_back(make_pair(e,true));
}

vector<pair<CFEntry*,bool>> CFNode::getEntries() {
	return this->entries;
}

void CFNode::setParentEntry(CFEntry* parent) { this->parentEntry = parent; }

bool CFNode::insertEntry(CFEntry *e) {

	//If the root is splitted, node(level >=3)'s level is not updated, make it update.  
	CFEntry *tempEntry = this->parentEntry;
	if (this->level >= 2 && this->level <= tempEntry->parentNode->level)
	{
		this->level = tempEntry->parentNode->level + 1;

		//만일 limitLevel보다 큰 경우, InfoPool에 insert. 
		//if (this->level > limitLevel)
			//this->cfptree->InfoPool->push_back(this);
	}

	if (NO_BUFFER)
		this->buff = nullptr;
	else if (ALL_BUFFER && this->buff == nullptr && this->cfptree->depth != 1)
	{
		this->buff = new Buffer(cfptree);
		cfptree->TreeLimit += BUFFER_PTR;
	}
	else {
		if (cfptree->depth > BUFFER_LEVEL_FROM_LEAF && this->level == cfptree->depth - BUFFER_LEVEL_FROM_LEAF && this->buff == nullptr)
		{
			this->buff = new Buffer(cfptree);
			cfptree->TreeLimit += BUFFER_PTR;
		}
	}



	//If the node is the buffer node. 
	if (this->buff != nullptr)
	{
		if (this->buff->buff.size() < maxBufferEntries) {
			this->buff->buff.push_back(e);
			return true;
		}
		else if (this->buff->buff.size() == maxBufferEntries) {
			//먼저 buff에 push하고 flush. 
			this->buff->buff.push_back(e);
			bool result = bufferFree();
			//this->buff->buff.push_back(e);
			return result;

		}
		else {
			return false;
		}
	}
	else {
		if (this->entries.size() == 0) {

			// if the node is empty we can insert the entry directly here

			//entry size가 0이면 항상 leaf node임. If this node is a leaf node, put in the leafEntries
			this->insertIntoLeafEntry(e);
			//entries.add(e);
			return true; // insert was successful. no split necessary
		}
		int closestIndex = 0;

		//아래처럼 하면 새로운 closest가 계속 생겨서 낭비됨. 
		//CFEntry* closest = new CFEntry(findClosestEntry(entries, e, &closestIndex));

		CFEntry* closest = findClosestEntry(entries, e, &closestIndex);


		bool dontSplit = false;

		if (!ClosestEntryinMememory)
		{
			//만일 closest의 child가 메모리에 없으면, child 노드를 읽어야 함. 
			//이 때, 현재 노드 레벨이 depth 바로 위이면, 읽어야 하는 노드가 leaf node이므로 leafstatus true 전달. 
			CFNode* readNode = new CFNode();
			if (this->level == this->cfptree->depth - 1)
				readNode = this->cfptree->nodeRead(closest->childId, true);
			else
				readNode = this->cfptree->nodeRead(closest->childId, false);

			readNode->level = this->level+1;
			readNode->parentEntry = this->entries[closestIndex].first;
			closest->child = readNode;

			//위에 줄로 실제로 tree에 할당 안되므로, 아래 줄로 할당. 
			this->entries[closestIndex].first->child = readNode;
			this->entries[closestIndex].second = true;
			ClosestEntryinMememory = true;

			this->cfptree->TreeLimit += (CFNODE_PTR + CFENTRY_PTR * maxNodeEntries);
			if (closest->child->buff != nullptr)
				this->cfptree->TreeLimit += (BUFFER_PTR + CFENTRY_PTR * maxBufferEntries);

			printf("read #%d NODE at ID NUMBER %d NODE , TRESSIZE : %d\n", closest->child->id, this->id, this->cfptree->TreeLimit);
		}

		if (closest->child != nullptr && !this->leafStatus) { 	// if closest has a child we go down with a recursive call

			//dontSplit = this->insertEntry(e);
			dontSplit = closest->child->insertEntry(e);
			if (dontSplit) {
				closest->update(e);

				// this updates the CF to reflect the additional entry
				return true;
			}
			else {
				// if the node below /closest/ didn't have enough room to host the new entry
				// we need to split it
				CFEntryPair* splitPair = splitEntry(closest);

				//기존 entry를 split하였는데, split한 entry 둘 중 하나라도 maxNodeEntries보다 큰 경우.  
				while (splitPair->e1->getChildSize() > maxNodeEntries || splitPair->e2->getChildSize() > maxNodeEntries) {
					CFEntryPair* tempPair = splitPair;
					//e1이 maxNodeEntries보다 더 많은 entries가 들어간 경우. 
					if (splitPair->e1->getChildSize() > maxNodeEntries)
					{
						//CFEntryPair* splitPair1 = new CFEntryPair();
						splitPair = splitEntry(splitPair->e1);
					}
					//e2이 maxNodeEntries보다 더 많은 entries가 들어간 경우. 
					else if (splitPair->e2->getChildSize() > maxNodeEntries)
					{
						//CFEntryPair* splitPair2 = new CFEntryPair();
						splitPair = splitEntry(splitPair->e2);
					}
					delete(tempPair);
				}
				delete(splitPair);
				// after adding the new entries derived from splitting /closest/ to this node,
				// if we have more than maxEntries we return false, 
				// so that the parent node will be split as well to redistribute the "load"
				if (this->entries.size() > maxNodeEntries) {
					return false;
				}
			}
		}
		else if (closest->isWithinThreshold(e, currentThreshold, distFunction)) {
			// if  dist(closest,e) <= T, /e/ will be "absorbed" by /closest/
			this->insertIntoLeafEntry(e, closest);
			this->entries[closestIndex].first = closest;
			WithinThresholdCNT++;
			return true; // no split necessary at the parent level
		}
		else if (this->entries.size() < maxNodeEntries) {
			// if /closest/ does not have children, and dist(closest,e) > T
			// if there is enough room in this node, we simply add e to it
			this->insertIntoLeafEntry(e);
			//entries.add(e);
			return true; // no split necessary at the parent level
		}
		else { // not enough space on this node
			this->insertIntoLeafEntry(e);
			//entries.add(e); // adds it momentarily to this node
			return false;   // returns false so that the parent entry will be split
		}
	}
}

void CFNode::insertIntoLeafEntry(CFEntry* e)
{
	this->entries.push_back(make_pair(e,true));
	//int* LeafTableRow = e->LeafTableAddr;

	//1번 index에는 속한 node_id, 2번 index에는 nodeentries의 index. 
	//LeafTableRow[1] = this->id;
	//LeafTableRow[2] = find(this->entries.begin(), this->entries.end(), e) - this->entries.begin();
}

void CFNode::insertIntoLeafEntry(vector<CFEntry*> thisNodeEntries, CFEntry* e) {

	thisNodeEntries.push_back(e);
	//int* LeafTableRow = e->LeafTableAddr;

	//1번 index에는 속한 node_id, 2번 index에는 nodeentries의 index. 
	//LeafTableRow[1] = e->parentNode->id;
	//LeafTableRow[2] = find(thisNodeEntries.begin(), thisNodeEntries.end(), e) - thisNodeEntries.begin();
}

void CFNode::insertIntoLeafEntry(vector<CFEntry*> thisNodeEntries, CFEntry* e, CFEntry* sumEntry)
{
	/*
	* e를 sumEntry에 추가.
	* sumEntry에 e에 대한 (N,LS,SS)만 update 시켜주면 됨.
	* 그리고 LeafTable에 e에 해당하는 data 값 update.
	*/

	sumEntry->update(e);

	//LeafTable update
	//int* LeafTableRow = e->LeafTableAddr;
	//1번 index에는 속한 node_id, 2번 index에는 nodeentries의 index. 
	//LeafTableRow[1] = e->parentNode->id;
	//LeafTableRow[2] = find(thisNodeEntries.begin(), thisNodeEntries.end(), e) - thisNodeEntries.begin();
}

void CFNode::insertIntoLeafEntry(CFEntry * e, CFEntry * sumEntry)
{
	/*
	* e를 sumEntry에 추가.
	* sumEntry에 e에 대한 (N,LS,SS)만 update 시켜주면 됨.
	* 그리고 LeafTable에 e에 해당하는 data 값 update.
	*/

	sumEntry->update(e);

	//LeafTable update
	//int* LeafTableRow = e->LeafTableAddr;
	//1번 index에는 속한 node_id, 2번 index에는 nodeentries의 index. 
	//LeafTableRow[1] = e->parentNode->id;
	//LeafTableRow[2] = find(thisNodeEntries.begin(), thisNodeEntries.end(), e) - thisNodeEntries.begin();
}

bool CFNode::bufferFree() {
	if (BUF_FLUSH_SEQ) {
		return bufferFreeSeq();
	}
}

bool CFNode::bufferFreeSeq() {

	int readNodeCnt = 0;
	int readBufferCnt = 0;
	for (unsigned int i = 0; i < this->entries.size(); i++)
	{
		//buffer를 flush 해야 하는데, child 들이 memory에 없는 경우(false 갖고 있음) : nodeRead 필요. 
		if (!this->entries[i].second)
		{
			CFNode* readNode = new CFNode();
			
			if(BUFFER_LEVEL_FROM_LEAF == 1)
				readNode = this->cfptree->nodeRead(entries[i].first->childId, true);
			else
				readNode = this->cfptree->nodeRead(entries[i].first->childId, false);

			readNode->level = this->level + 1;
			readNode->parentEntry = this->entries[i].first;
			
			//읽었으므로, 현재 노드의 child에 읽은 노드를 연결시키고, 메모리에 존재하므로 true로 바꿔줌. 
			this->entries[i].first->child = readNode;
			this->entries[i].second = true;

			if (this->cfptree->depth > BUFFER_LEVEL_FROM_LEAF && readNode->level == this->cfptree->depth - BUFFER_LEVEL_FROM_LEAF)
				readBufferCnt++;
			else
				readNode->buff = nullptr;

			readNodeCnt++;
		}
	}

	if (!flushBufferOneByOne) {
		this->cfptree->TreeLimit += readNodeCnt * (CFNODE_PTR + CFENTRY_PTR * maxNodeEntries);
		if (readBufferCnt)
			this->cfptree->TreeLimit += readBufferCnt * (BUFFER_PTR + CFENTRY_PTR * maxBufferEntries);

		if (readNodeCnt > 0)
			printf("Buffer FLUSH : read %d NODE at ID NUMBER %d NODE , TRESSIZE : %d\n", readNodeCnt, this->id, this->cfptree->TreeLimit);
	}

	//child node들의 level update 안되어 있으면 update
	for (unsigned int i = 0; i < this->entries.size(); i++)
	{
		if (this->entries[i].first->child != nullptr)
		{
			int tempLv = this->entries[i].first->child->level;
			if (this->level >= tempLv && tempLv >= 2)
				this->entries[i].first->child->level = this->level + 1;
		}
	}

	vector<vector<CFEntry*>>cfentry;
	if(this->entries.size()==0)
		cfentry = vector<vector<CFEntry*>>(this->buff->buff.size());
	else
		cfentry = vector<vector<CFEntry*>>(this->entries.size());

	for (unsigned int j = 0; j < cfentry.size(); j++)
		cfentry[j] = vector<CFEntry*>(); 

	vector<CFEntry*>buff_in = this->buff->buff;

	//buffer 내용 초기화 
	this->buff->buff.clear();

	if(this->entries.size() == 0)
	{
		for (unsigned int idx = 0; idx < buff_in.size(); idx++) {
			CFEntry* e = buff_in[idx];
			this->insertIntoLeafEntry(e);
		}
	}

	else {
		for (unsigned int idx = 0; idx < buff_in.size(); idx++) {
			CFEntry* e = buff_in[idx];
			int closest_id = findClosestEntryIdx(e);
			cfentry[closest_id].push_back(e);
		}


		vector<bool>boolArray = vector<bool>(this->entries.size());
		for (int idx = 0; idx < cfentry.size(); idx++) {
			CFEntry* closest = this->entries[idx].first;

			//만약에 limitlevel보다 크고, 메모리에 존재하고 있으면, infopool에 pushback
			/*if (closest->child->level > limitLevel && this->entries[idx].second)
			{
				if (!this->entries[idx].first->child->inNodePool)
				{
					this->cfptree->InfoPool->push_back(closest->child);
					this->entries[idx].first->child->inNodePool = true;
				}
			}

			//메모리에 존재하지 않으면, disk로부터 읽기.
			if ( (!this->entries[idx].second) || (this->entries[idx].first->child == nullptr) )
				this->cfptree->InfoPool->nodeRead(closest->childId);*/

			boolArray[idx] = closest->child->insertlistEntry(cfentry[idx]);
		}

		int totalSize = cfentry.size();
		vector <CFEntry*> tempEntries;
		for (int i = 0; i < totalSize; i++)
		{
			CFEntry* temp = this->entries[i].first;
			tempEntries.push_back(temp);
		}

		//for (int idx = 0; idx < cfentry.size(); idx++) {
		for (int idx = 0; idx < totalSize; idx++) {
			if (!boolArray[idx]) {
				// if the node below /closest/ didn't have enough room to host the new entry
				// we need to split it
				CFEntryPair* splitPair = nullptr;

				//아래 주석 줄로 하면, 뒤에 추가되서 split 되야 하는데 안 되는 경우가 생김. 
				splitPair = splitEntry(tempEntries[idx]);
				//splitPair = splitEntry(this->entries[idx].first);

				//this.bacftree.addNodeToLazyBuffer(closest.getChild());

				//기존 entry를 split하였는데, split한 entry 둘 중 하나라도 maxNodeEntries보다 큰 경우.  
				while (splitPair->e1->getChildSize() > maxNodeEntries || splitPair->e2->getChildSize() > maxNodeEntries) {

					CFEntryPair* tempPair = splitPair;
					//e1이 maxNodeEntries보다 더 많은 entries가 들어간 경우. 
					if (splitPair->e1->getChildSize() > maxNodeEntries)
					{
						//CFEntryPair* splitPair1 = new CFEntryPair();
						//splitPair1 = splitEntry(splitPair->e1);
						splitPair = splitEntry(splitPair->e1);
					}
					//e2이 maxNodeEntries보다 더 많은 entries가 들어간 경우. 
					else if (splitPair->e2->getChildSize() > maxNodeEntries)
					{
						//CFEntryPair* splitPair2 = new CFEntryPair();
						//splitPair2 = splitEntry(splitPair->e2);
						splitPair = splitEntry(splitPair->e2);
					}
					delete(tempPair);
				}

				if (entries.size() > maxNodeEntries) {
				}
				else { // splitting stops at this node
					//if (applyMergingRefinement) // performs step 4 of insert process (see BIRCH paper, Section 4.3)
						//mergingRefinement(splitPair);
				}
				delete(splitPair);
			}
			//			this.nodeWrite();
		}

		//memory deallocated vectors
		vector<bool>().swap(boolArray);
		vector<CFEntry*>().swap(buff_in);
		vector<vector<CFEntry*>>().swap(cfentry);
		vector<CFEntry*>().swap(tempEntries);
	}

	if (this->entries.size() > maxNodeEntries) {
		return false;
	}
	else { // splitting stops at this node
		return true;
	}
}

void CFNode::bufferFreeLast() {

	int readNodeCnt = 0;
	int readBufferCnt = 0;
	

	//child node들의 level update 안되어 있으면 update
	for (unsigned int i = 0; i < this->entries.size(); i++)
	{
		if (this->entries[i].first->child != nullptr)
		{
			int tempLv = this->entries[i].first->child->level;
			if (this->level >= tempLv && tempLv >= 2)
				this->entries[i].first->child->level = this->level + 1;
		}
	}

	vector<vector<CFEntry*>>cfentry;
	if (this->entries.size() == 0)
		cfentry = vector<vector<CFEntry*>>(this->buff->buff.size());
	else
		cfentry = vector<vector<CFEntry*>>(this->entries.size());

	for (unsigned int j = 0; j < cfentry.size(); j++)
		cfentry[j] = vector<CFEntry*>();

	vector<CFEntry*>buff_in = this->buff->buff;

	//buffer 내용 초기화 
	this->buff->buff.clear();

	for (unsigned int idx = 0; idx < buff_in.size(); idx++) {
		CFEntry* e = buff_in[idx];
		int closest_id = findClosestEntryIdx(e);
		cfentry[closest_id].push_back(e);
	}


	for (int idx = 0; idx < cfentry.size(); idx++) {
		CFEntry* closest = this->entries[idx].first;
		closest->child->insertlistEntryLast(cfentry[idx]);
	}

	//memory deallocated vectors
	vector<CFEntry*>().swap(buff_in);
	vector<vector<CFEntry*>>().swap(cfentry);
	
}

bool CFNode::insertlistEntry(vector<CFEntry*> listInsert){
	//buffer
	bool result = true;

	for(unsigned int i = 0; i< listInsert.size(); i++)
	{
		CFEntry *e = listInsert[i];
		CFEntry* closest = findClosestEntry(this->entries, e);

		if (closest->isWithinThreshold(e, currentThreshold, distFunction)) {
			// if  dist(closest,e) <= T, /e/ will be "absorbed" by /closest/
			closest->update(e);
			result = result && true; // no split necessary at the parent level
		}
		else {
			//안 들어가면 큐에 저장. 
			if (flushBufferOneByOne)
				NotInsertedQueue.push(e);
			else if (entries.size() < maxNodeEntries) {
				// if /closest/ does not have children, and dist(closest,e) > T
				// if there is enough room in this node, we simply add e to it
				this->entries.push_back(make_pair(e, true));
				result = result && true; // no split necessary at the parent level
			}
			else { // not enough space on this node
				this->entries.push_back(make_pair(e, true)); // adds it momentarily to this node
				result = result && false;   // returns false so that the parent entry will be split
			}
		}
	}
	return result;
}

void CFNode::insertlistEntryLast(vector<CFEntry*> listInsert) {
	//buffer

	for (unsigned int i = 0; i < listInsert.size(); i++)
	{
		CFEntry* e = listInsert[i];
		CFEntry* closest = findClosestEntry(this->entries, e);

		if (closest->isWithinThreshold(e, currentThreshold, distFunction)) {
			// if  dist(closest,e) <= T, /e/ will be "absorbed" by /closest/
			closest->update(e);
		}
		else {
			this->entries.push_back(make_pair(e, true)); // adds it momentarily to this node

		}
	}
}

int CFNode::findClosestEntryIdx(CFEntry *e) {
	double minDist = DOUBLE_MAX;
	int idx = 0;
	for (unsigned int j = 0; j < this->entries.size(); j++) {
		//bufferfree할 때, 아래 노드 방문

		//second가 true이면, memory 상에 존재. false이면 memory 상에 없으므로, disk에서 가져와야 함. 
		if (!entries[j].second)
			this->cfptree->InfoPool->nodeRead(this->entries[j].first->childId);

		double d = this->entries[j].first->distance(e, distFunction);
		if (d < minDist) {
			minDist = d;
			idx = j;
		}
	}
	return idx;
}

CFEntry* CFNode::findClosestEntry(vector<pair<CFEntry*,bool>> thisNodeEntries, CFEntry* e) {
	double minDist = DOUBLE_MAX;
	CFEntry* closest = nullptr;

	for (unsigned int i = 0; i < thisNodeEntries.size(); i++)
	{
		CFEntry* c = thisNodeEntries[i].first;
		double d = c->distance(e, distFunction);
		if (d < minDist) {
			minDist = d;
			closest = c;
		}
	}
	return closest;
}

CFEntry* CFNode::findClosestEntry(vector<pair<CFEntry*,bool>> thisNodeEntries, CFEntry* e, int *index) {
	double minDist = DOUBLE_MAX;
	CFEntry* closest = nullptr;

	for (unsigned int i = 0; i < thisNodeEntries.size(); i++)
	{
		CFEntry* c = thisNodeEntries[i].first;
		double d = c->distance(e, distFunction);
		if (d < minDist) {
			minDist = d;
			closest = c;
			*index = i;
			
			//closest의 child 노드가 메모리에 있는지 없는지 전역변수에 저장. 
			ClosestEntryinMememory = thisNodeEntries[i].second;
		}
	}
	return closest;
}

CFEntryPair* CFNode::splitEntry(CFEntry* closest) {

	int closest_index = 0;
	bool inMemory = true;
	CFEntry* oldEntry = nullptr;
	//= find(this->entries.begin(), this->entries.end(), closest) - this->entries.begin();
	for (int i = 0; i < this->entries.size(); i++)
	{
		//split 하려고 entries를 search 하는데, 해당 노드가 메모리에 없는 경우,  READ
		if (!this->entries[i].second)
		{
			// 만일 closest의 child가 메모리에 없으면, child 노드를 읽어야 함.
			//이 때, 현재 노드 레벨이 depth 바로 위이면, 읽어야 하는 노드가 leaf node이므로 leafstatus true 전달. 
			CFNode* readNode = new CFNode();
			if (this->level == this->cfptree->depth - 1)
				readNode = this->cfptree->nodeRead(this->entries[i].first->childId, true);
			else
				readNode = this->cfptree->nodeRead(this->entries[i].first->childId, false);

			readNode->level = this->level + 1;
			readNode->parentEntry = this->entries[i].first;

			//위에 줄로 실제로 tree에 할당 안되므로, 아래 줄로 할당. 
			this->entries[i].first->child = readNode;
			this->entries[i].second = true;

			this->cfptree->TreeLimit += (CFNODE_PTR + CFENTRY_PTR * maxNodeEntries);
			if (readNode->buff != nullptr)
				this->cfptree->TreeLimit += (BUFFER_PTR + CFENTRY_PTR * maxBufferEntries);
		}

		if (this->entries[i].first->child->id == closest->child->id)
		{
			closest_index = i;
			oldEntry = this->entries[i].first;
			//삭제하려는 노드가 메모리상에 존재하는지 확인하는 bool 
			//if(closest->child->level > limitLevel)
				//inMemory = closest->parentNode->entries[i].second;
			break;
		}
	}

	this->entries.erase(this->entries.begin() + closest_index);  //this will be substitute by two new entries


	CFNode* oldNode = closest->child;
	/*CFNode* oldNode = new CFNode();
	oldNode = closest->child;*/
	

	//만일 oldNode가 limitLevel을 넘어서 NodePool에서 관리되고 있는 경우. 
	//NodePool* tempPool = new NodePool();
	//tempPool = this->cfptree->InfoPool;

	//NodePool에서 해당 노드 삭제. 
	//if (oldNode->level > limitLevel && inMemory)
		//tempPool->remove(oldNode);

	//vector<CFEntry*> oldEntries = oldNode->entries;
	CFEntryPair* p = oldNode->findFarthestEntryPair();
	vector<pair<CFEntry*,bool>> oldEntries = oldNode->entries;

	CFEntry* newEntry1 = new CFEntry();
	//CFNode newNode1 = new CFNode(oldNode.cfptree, maxNodeEntries,distThreshold,distFunction,applyMergingRefinement,oldNode.isLeaf());
	this->cfptree->TreeLimit += CFENTRY_PTR;

	//기존 oldNode와 같은 id와 level 갖는 노드.
	CFNode* newNode1 = new CFNode(oldNode->cfptree, oldNode->leafStatus, oldNode->level, oldNode->id);
	newEntry1->setChildAll(newNode1);
	newNode1->setParentEntry(newEntry1);

	CFEntry* newEntry2 = new CFEntry();
	//CFNode newNode2 = new CFNode(oldNode.cfptree, maxNodeEntries,distThreshold,distFunction,applyMergingRefinement,oldNode.isLeaf());
	this->cfptree->TreeLimit += CFENTRY_PTR;

	//기존 oldNode와 level만 같고 id가 다른 노드.
	CFNode* newNode2 = new CFNode(oldNode->cfptree, oldNode->leafStatus, oldNode->level);
	newEntry2->setChildAll(newNode2);
	newNode2->setParentEntry(newEntry2);

	newEntry1->addToChild(p->e1);
	newEntry1->update(p->e1);
	newEntry1->childId = newNode1->id;

	newEntry2->addToChild(p->e2);
	newEntry2->update(p->e2);
	newEntry2->childId = newNode2->id;

	oldNode->redistributeEntries(oldEntries, p, newEntry1, newEntry2);

	if (oldNode->leafStatus == true) { // we do this to preserve the pointers in the leafList 

		CFNode* prevL = oldNode->getPreviousLeaf();
		CFNode* nextL = oldNode->getNextLeaf();

		newNode1->setNextLeaf(newNode2);
		newNode2->setPreviousLeaf(newNode1);
		if (prevL != nullptr) {
			prevL->setNextLeaf(newNode1);
			newNode1->setPreviousLeaf(prevL);
		}

		if (nextL != nullptr) {
			nextL->setPreviousLeaf(newNode2);
			newNode2->setNextLeaf(nextL);
		}

		if (Get_LeafTable)
		{//새롭게 split된 p.e1과 p.e2에 대한 LeafTable 정보 update.
			//newNode1->LeafTableUpdate(newNode1->entries, p->e1);
			//newNode2->LeafTableUpdate(newNode2->entries, p->e2);
		}
	}

	//closest node가 split되었는데, closest node가 buffer가 있어서 buffer안의 entry도 split 되어야 하는 경우.
	if (closest->child->buff != nullptr)
	{
		//newEntry1, newEntry2 중 buffer 내부의 entry와 더 가까운 쪽의 buffer에 삽입.  채우기 !, 
		//Buffer가 full될 일은 없음. 해당 노드가 2개로 split된 것이므로. 
		vector <CFEntry*> tempBuffer = closest->getChild()->buff->buff;
		for (unsigned int i = 0; i < tempBuffer.size(); i++)
		{
			CFEntry* tempEntry = tempBuffer[i];
			FindClosestBuffer(newEntry1, newEntry2, tempEntry)->child->buff->buff.push_back(tempEntry);
		}

		vector<CFEntry*>().swap(closest->child->buff->buff);
		delete(closest->child->buff);
		this->cfptree->TreeLimit -= BUFFER_PTR;
	}


	// redistributes the entries in n between newEntry1 and newEntry2
	// according to the distance to p.e1 and p.e2
	this->addToEntryList(newEntry1);
	newEntry1->setParentNode(this);
	this->addToEntryList(newEntry2);
	newEntry2->setParentNode(this);

	//자식 노드에 부모 노드 Id 추가
	if (newEntry1->parentNode != nullptr)
		newNode1->parentId = newEntry1->parentNode->id;
	if (newEntry2->parentNode != nullptr)
		newNode2->parentId = newEntry2->parentNode->id;

	delete(oldNode);
	delete(oldEntry);
	this->cfptree->TreeLimit -= (CFNODE_PTR + CFENTRY_PTR);
	delete(p);

	

	CFEntryPair* newPair = new CFEntryPair(newEntry1, newEntry2);

	//만일 oldNode가 limitLevel을 넘어서 NodePool에서 관리되어야 할 때, 새로운 node들 NodePool에 추가. 
	/*if (oldNode->level > limitLevel)
	{
		tempPool->push_back(newNode1);
		newNode1->inNodePool = true;
		tempPool->push_back(newNode2);
		newNode2->inNodePool = true;
	}*/

	return newPair;
}
CFEntry* CFNode::FindClosestBuffer(CFEntry* newE1, CFEntry* newE2, CFEntry* BuffEntry) {

	double dist1 = newE1->distance(BuffEntry, distFunction);
	double dist2 = newE2->distance(BuffEntry, distFunction);

	if (dist1 <= dist2)
		return newE1;
	else
		return newE2;

}

CFEntryPair* CFNode::findFarthestEntryPair() {
	if (this->entries.size() < 2)
		printf("Pair Error!\n");

	double maxDist = -1;
	CFEntryPair* p = new CFEntryPair();
	int e1id = -1;
	int e2id = -1;
	for (unsigned int i = 0; i < this->entries.size() - 1; i++) {
		for (unsigned int j = i + 1; j < this->entries.size(); j++) {
			CFEntry* e1 = this->entries[i].first;
			CFEntry* e2 = this->entries[j].first;

			double dist = e1->distance(e2, distFunction);
			if (dist > maxDist) {
				p->e1 = e1;			e1id = i;
				p->e2 = e2;			e2id = j;
				maxDist = dist;
			}
		}
	}
	this->entries.erase(this->entries.begin() + e1id);
	this->entries.erase(this->entries.begin() + e2id - 1);
	return p;
}


CFEntryPair* CFNode::findFarthestEntryPair(vector<CFEntry*>entries) {
	if (entries.size() < 2)
		printf("Pair Error!\n");

	double maxDist = -1;
	CFEntryPair* p = new CFEntryPair();
	int e1id = -1;
	int e2id = -1;
	for (unsigned int i = 0; i < entries.size() - 1; i++) {
		for (unsigned int j = i + 1; j < entries.size(); j++) {
			CFEntry* e1 = entries[i];
			CFEntry* e2 = entries[j];

			double dist = e1->distance(e2, distFunction);
			if (dist > maxDist) {
				p->e1 = e1;			e1id = i;
				p->e2 = e2;			e2id = j;
				maxDist = dist;
			}
		}
	}
	entries.erase(entries.begin() + e1id);
	entries.erase(entries.begin() + e2id - 1);
	return p;
}

void CFNode::LeafTableUpdate(vector<CFEntry*> thisNodeEntries, CFEntry* e) {

	//* LeafTableRow = e->LeafTableAddr;
	CFEntry *tempEntry = new CFEntry(e);
	//1번 index에는 속한 node_id, 2번 index에는 nodeentries의 index. 
	//LeafTableRow[1] = e->parentNode->id;
	//LeafTableRow[2] = find(thisNodeEntries.begin(), thisNodeEntries.end(), tempEntry) - thisNodeEntries.begin();
}

void CFNode::redistributeEntries(vector<pair<CFEntry*,bool>> oldEntries, CFEntryPair* farEntries, CFEntry* newE1, CFEntry* newE2) {
	ClosestEntry(newE1, newE2);
}

void CFNode::ClosestEntry( CFEntry* newE1, CFEntry* newE2) {

	while (this->entries.size() != 0) {
		CFEntry* e = nullptr;
		e = this->FindClosest(newE1, newE2);

		double dist1 = newE1->distance(e, distFunction);
		double dist2 = newE2->distance(e, distFunction);

		if (dist1 <= dist2) {
			newE1->addToChild(e);
			newE1->update(e);

			if (Get_LeafTable) {
				//만일 leaf Node였으면, data에 대한 LeafNode index update
				//if (this->leafStatus == true)
					//LeafTableUpdate(newE1->child->entries, e);
			}
		}
		else {
			newE2->addToChild(e);
			newE2->update(e);

			if (Get_LeafTable) {
				//if (this->leafStatus == true)
					//LeafTableUpdate(newE1->child->entries, e);
			}
		}
	}
}

CFEntry* CFNode::FindClosest(CFEntry* newE1, CFEntry* newE2) {

	CFEntry* closest = nullptr;
	double dist = 99999999;
	int i = 0;
	int c_id = 0;

	for (unsigned i = 0;i< this->entries.size(); i++) {
		CFEntry *e = entries[i].first;
		double dist1 = newE1->distance(e, distFunction);
		double dist2 = newE2->distance(e, distFunction);

		if (dist1 <= dist2) {
			if (dist1 < dist) {
				dist = dist1;
				closest = e;
				c_id = i;
			}
		}
		else if (dist2 < dist1) {

			if (dist2 < dist) {
				dist = dist2;
				closest = e;
				c_id = i;
			}
		}
	}
	this->entries.erase(this->entries.begin() + c_id);
	return closest;
}

void SumCopy(double* src, double* dest)
{
	for (int i = 0; i < dim; i++)
		dest[i] = src[i];
}


//Class CFEntry
CFEntry::CFEntry() {
	fill_n(this->sumX, dim, (double)-1);
	fill_n(this->sumX2, dim, (double)-1);
}
CFEntry::CFEntry(const CFEntry &e) {
	this->inputIndex = e.inputIndex;
	this->n = e.n;
	SumCopy(e.sumX, this->sumX);
	SumCopy(e.sumX2, this->sumX2);
	this->child = e.child; // WARNING: we do not make a deep copy of the child!!!
}
CFEntry::CFEntry(CFEntry *e) {
	if(FOR_DEBUG_INDEX)
		this->inputIndex = e->inputIndex;
	this->n = e->n;
	SumCopy(e->sumX, this->sumX);
	SumCopy(e->sumX2, this->sumX2);
	this->child = e->child; // WARNING: we do not make a deep copy of the child!!!
	this->childId = e->childId;
}

CFEntry::CFEntry(double* x, string className) {
	CFEntry(x, 0, className);
}

CFEntry::CFEntry(double* x, int index, string className) {
	this->n = 1;
	this->inputIndex.push_back(index);
	//this->sumX = new double[dim];
	for (int i = 0; i < dim; i++)
		sumX[i] = x[i];

	//this->sumX2 = new double[dim];
	for (int i = 0; i < dim; i++)
		sumX2[i] = x[i] * x[i];

	//true cluster에서 c17이면 c 지워주고, 남은 숫자정보를 해당 index의 leafTable에 삽입. true cluster 정보는 3rd column
	className.erase(0,1);
	//leaf_table->Leaf_Table[index][3] = atoi(className.c_str());
	//this->subclusterID = index;
	//->LeafTableAddr = leaf_table->Leaf_Table[index];	//Leaf_Table주소 저장. 
}

CFEntry::CFEntry(int n, double* x)
{
	this->n = n;
	//this->sumX = new double[dim];
	for (int i = 0; i < dim; i++)
			sumX[i] = x[i];

	//this->sumX2 = new double[dim];
	for (int i = 0; i < dim; i++)
		sumX2[i] = x[i] * x[i];
}

long CFEntry::getCFENTRY_PTR_SIZE()
{
	long size = 0;
	size = sizeof(this->inputIndex) + sizeof(this->child) + sizeof(this->parentNode) + sizeof(this->childId) + sizeof(this->radius) + sizeof(this->n) + sizeof(this->sumX) + sizeof(this->sumX2);
	return size;
}

bool CFEntry::hasChild() {
	return (this->childId != 0);
}
CFNode* CFEntry::getChild() {
	return this->child;
}
void CFEntry::setChildAll(CFNode* n) {
	this->child = n;
	this->childId = n->id;
}
void CFEntry::setChild(CFNode* n) {
	this->child = n;
}
void CFEntry::setChildNull() {
	this->child = nullptr;
}
bool CFEntry::isChildNull() {
	return this->child == nullptr;
}
int CFEntry::getChildSize() {
	return child->entries.size();
}
void CFEntry::addToChild(CFEntry* e) {

	//farthest를 분배할 때, e->child가 null이면 메모리에 없다는 뜻이므로, false를 설정해줌. 
	if (e->child == NULL)
		this->child->entries.push_back(make_pair(e, false));
	else
		this->child->entries.push_back(make_pair(e, true));
	e->setParentNode(child);
}
void CFEntry::setParentNode(CFNode* parent) {
	this->parentNode = parent;
}


void CFEntry::setSubclusterID(int id) {
	//this->subclusterID = id;
}
int CFEntry::getSubclusterID() {
	//return this->subclusterID;
	return 0;
}

double CFEntry::distance(CFEntry* e, int distFunction) {
	double dist = DOUBLE_MAX;

	switch (distFunction) {

	case D0_DIST:
		dist = d0(this, e);
		break;
		/*case D1_DIST:
			dist = d1(this, e);
			break;
		case D2_DIST:
			dist = d2(this, e);
			break;
		case D3_DIST:
			dist = d3(this, e);
			break;
		case D4_DIST:
			dist = d4(this, e);
			break;*/
	}
	return dist;
}

double CFEntry::d0(CFEntry* e1, CFEntry* e2) {

	double dist = 0;
	for (int i = 0; i < dim; i++) {
		double diff = e1->sumX[i] / e1->n - e2->sumX[i] / e2->n;

		dist += diff * diff;
	}

	if (dist < 0)
		printf("d0 < 0 !!!\n");

	return sqrt(dist);
}

void CFEntry::update(CFEntry e) {

	this->inputIndex.push_back(e.inputIndex[0]);

	this->n += e.n;

	if (this->sumX[0] == (double)-1)
		copy(this->sumX, this->sumX + dim, e.sumX);
	else {
		for (int i = 0; i < dim; i++)
			this->sumX[i] += e.sumX[i];
	}

	if (this->sumX2[0] == (double)-1)
		copy(this->sumX2, this->sumX2 + dim, e.sumX2);
	else {
		for (int i = 0; i <dim; i++)
			this->sumX2[i] += e.sumX2[i];
	}
}

void CFEntry::update(CFEntry *e)
{	for(int i =0; i< e->inputIndex.size();i++)
		this->inputIndex.push_back(e->inputIndex[i]);

	this->n += e->n;

	if (this->sumX[0] == (double)-1)
		copy(e->sumX, e->sumX + dim, this->sumX);
		//copy(this->sumX, this->sumX + dim, e->sumX);
	else {
		for (int i = 0; i < dim; i++)
			this->sumX[i] += e->sumX[i];
	}

	if (this->sumX2[0] == (double)-1)
		copy(e->sumX2, e->sumX2 + dim, this->sumX2);
		//copy(this->sumX2, this->sumX2 + dim, e->sumX2);
	else {
		for (int i = 0; i < dim; i++)
			this->sumX2[i] += e->sumX2[i];
	}
}

bool CFEntry::isWithinThreshold(CFEntry* e, double threshold, int distFunction) {
	double dist = distance(e, distFunction);

	if (dist == 0 || dist <= threshold) // read the comments in function d0() about differences with implementation in R
		return true;

	return false;
}

//Class CFEntryPair
CFEntryPair::CFEntryPair(CFEntry e1, CFEntry e2) {
	this->e1 = &e1;
	this->e2 = &e2;
}

CFEntryPair::CFEntryPair(CFEntryPair * p)
{
	this->e1 = p->e1;
	this->e2 = p->e2;
}

CFEntryPair::CFEntryPair(CFEntry * e1, CFEntry * e2)
{
	this->e1 = e1;
	this->e2 = e2;
}

//Class Buffer
Buffer::Buffer() {}
Buffer::Buffer(CFPTree *inTree)
{
	this->cfptree = inTree;
	this->buff = vector<CFEntry*>();
}

Buffer::Buffer(CFNode n) {
	this->cfptree = n.cfptree;
	this->buff = vector<CFEntry*>();
	this->treeNode_Ptr = &n;
	this->treeNode_ID = n.id;
}

long Buffer::getBUFFER_PTR_SIZE()
{
	long size = 0;
	size = sizeof(CFEntry*)*maxBufferEntries + sizeof(this->treeNode_Ptr) + sizeof(this->cfptree) + sizeof(this->treeNode_ID);
	return size;
}

//Class LeafTable
LeafTable::LeafTable(int datasize) {
	Leaf_Table = new int*[datasize];

	// 0th col : input index , 1th col : Node id , 2nd col : Node Entry, 3rd col : True Cluster index 
	for (int i = 0; i < datasize; i++)
		Leaf_Table[i] = new int[4];
}

//Class NodePool
NodeInfo::NodeInfo(CFNode tempNode) {
	//this->TreeNode_ID = tempNode.id;
	this->TreeNode_ptr = &tempNode;
}

NodeInfo::NodeInfo(CFNode *tempNode) {
	//this->TreeNode_ID = tempNode.id;
	this->TreeNode_ptr = tempNode;
}

NodePool::NodePool() {};

void NodePool::push_back(CFNode tempCF)
{
	NodeInfo tempNode = NodeInfo(tempCF);

	//If NodePool is full, flush all node. 
	if (doFlush)
		NodePool_Flush();

	//LinkedList push_back 연산과 동일. 
	if (this->head == nullptr)
		this->head = &tempNode;
	else
	{
		NodeInfo* node = this->tail;
		node->next = &tempNode;
		tempNode.prev = node;
		this->tail = &tempNode;
	}
	this->curNodeInfoCnt++;

	if (this->curNodeInfoCnt == maxPoolSize)
		this->doFlush = true;
}
void NodePool::push_back(CFNode* tempCF)
{
	NodeInfo* tempNode = new NodeInfo(tempCF);

	//If NodePool is full, flush all node. 
	if (doFlush)
		NodePool_Flush();

	//LinkedList push_back 연산과 동일. 
	if (this->head == nullptr)
	{
		this->head = tempNode;
		this->tail = tempNode;
		tempCF->NodePool_Ptr = tempNode;
	}
	else
	{
		NodeInfo* node = this->tail;
		node->next = tempNode;
		tempNode->prev = node;
		this->tail = tempNode;
		tempCF->NodePool_Ptr = tempNode;
	}
	this->curNodeInfoCnt++;

	if (this->curNodeInfoCnt == maxPoolSize)
		this->doFlush = true;
}

void NodePool::remove(CFNode* tempCF)
{
	//NodeInfo* tempNode = &NodeInfo(tempCF);
	NodeInfo* tempNode = tempCF->NodePool_Ptr;

	//head를 delete
	if (this->head->TreeNode_ptr->id == tempNode->TreeNode_ptr->id)
	{
		this->head = tempNode->next;
		tempNode->next->prev = nullptr;
	}
	//tail을 delete
	else if (this->tail->TreeNode_ptr->id == tempNode->TreeNode_ptr->id)
	{
		this->tail = tempNode->prev;
		tempNode->prev->next = nullptr;
	}
	else
	{
		NodeInfo* prevNode = tempNode->prev;
		NodeInfo* nextNode = tempNode->next;
		prevNode->next = nextNode;
		nextNode->prev = prevNode;
	}
	free(tempNode);
	this->curNodeInfoCnt--;
	if (this->doFlush)
		this->doFlush = false;
}

void NodePool::hit_go_tail(CFNode tempNode)
{
	NodeInfo* tempInfo = tempNode.NodePool_Ptr;
	NodeInfo* nextNode = tempInfo->next;

	//tempNode가 linkedlist의 tail에 있는 경우. 아무런 수행할 필요 없음. 
	if ((int)(&(this->tail)) == (int)(&(tempInfo)))
		return;

	//tempNode가 linkedlist의 head에 있는 경우. 
	else if ((int)(&(this->head)) == (int)(&(tempInfo)))
	{
		this->head = nextNode;
		nextNode->prev = nullptr;
	}
	//link 중간에서 없애줌. 
	else
	{
		NodeInfo* prevNode = tempInfo->prev;
		prevNode->next = nextNode;
		nextNode->prev = prevNode;
	}
	//hit된 NodeInfo를 tail로 옮겨줌. 
	this->tail->next = tempInfo;
	this->tail = tempInfo;

}
void NodePool::NodePool_Flush()
{
	FILE* fp = fopen(NodePool_path, "wb+");
	NodeInfo* curNode = this->head;
	CFNode* curCFNode = curNode->TreeNode_ptr;

	int curLevelLimit = limitLevel;

	int iterCnt = maxPoolSize;
	while (iterCnt > 0)
	{
		if (curNode == nullptr)
			break;
		curCFNode = curNode->TreeNode_ptr;

		/*
		* DISK에 node 정보 write. 해당 node를 write했다고 해서 아래 다 탐방해서 write 할 필요 없음.
		* 어짜피 LimitLevel이거나 그 아래에 있는 node들은 NodePool에 있거나, Disk에 있기 때문에 굳이 탐방하느라
		* 시간 낭비할 필요 없음. LinkedList 순회하면 전부 Write 가능.
		*/

		//buffer flush되서 현재 entry에 많이 들어가 있는 경우, split해야 해서 방문하게 되므로 node에 write하지 않음. 
		if(curCFNode->entries.size()<=maxNodeEntries)
			nodeWrite(curCFNode,fp);

		/*
		* 만일 free하려고 하는 curCFNode의 level이 LevelLimit보다 1커서
		* curCFNode의 parentCFNode가 LevelLimit안에 들어온 경우.
		* 해당 parentCFNode의 entries정보는 그대로 유지하고,
		* curCFNode의 parentEntry의 childNode에 대한 ptr를 NULL로 설정.
		* 단, parentEntry의 childID는 NodeRead시 필요하므로 알고 있어야함.
		*/
		//if (curCFNode->level == curLevelLimit + 1)
		if (curCFNode->parentEntry->parentNode != nullptr)
		{
			curCFNode->parentEntry->child = nullptr;
			int size = curCFNode->parentEntry->parentNode->entries.size();
			for (int i = 0; i < size; i++)
			{
				if (curCFNode->id == curCFNode->parentEntry->parentNode->entries[i].first->childId)
				{
					curCFNode->parentEntry->parentNode->entries[i].second = false;
					break;
				}
			}
		}
		

		//다음 LinkedList로 이동 후, 현재 노드 free. 
		NodeInfo* tempNode = curNode;
		curNode = curNode->next;
		free(tempNode);
		this->curNodeInfoCnt--;
		iterCnt--;
	}
	this->head = nullptr;
	this->tail = nullptr;
	this->doFlush = false;
	fclose(fp);
}

void NodePool::nodeWrite(CFNode* tempNode, FILE* fp)
{
	/*fprintf(fp, "%8x%p%2x", tempNode->id, tempNode->buff, tempNode->entries.size());
	int i = 0;
	for (i = 0; i < tempNode->entries.size(); i++)
	{
		CFEntry* temp = tempNode->entries[i].first;
		fprintf(fp, "%8x%.6lf%.6lf", temp->n, temp->sumX, temp->sumX2);
	}
	int noneint = 0;
	double nonedouble = 0.0;
	for(i ; i< maxNodeEntries ; i++)
		fprintf(fp, "%8x%.6lf%.6lf", noneint, nonedouble, nonedouble);*/
	IONode writeNode = IONode(tempNode);
	fseek(fp, sizeof(writeNode) * writeNode.id, SEEK_SET);
	fwrite(&writeNode, sizeof(writeNode), 1, fp);
}
void NodePool::nodeRead(int treeNode_ID)
{
	FILE* fp = fopen(NodePool_path, "rb");
	IONode readNode;
	fseek(fp, sizeof(readNode) * treeNode_ID, SEEK_SET);
	fwrite(&readNode, sizeof(readNode), 1, fp);
	fclose(fp);
}

IONode::IONode()
{}

IONode::IONode(CFNode* curNode)
{
	this->id = curNode->id;
	this->entrySize = curNode->entries.size();
	if (curNode->level == curNode->cfptree->depth - BUFFER_LEVEL_FROM_LEAF)
	{
		if (curNode->buff == nullptr)
			this->buffFlag = false;
		else
			this->buffFlag = true;
	}
	else
		this->buffFlag = false;
	/*if (curNode->level == curNode->cfptree->depth - BUFFER_LEVEL_FROM_LEAF)
	{
		if (curNode->buff == nullptr)
			this->buffEntrySize = 0;
		else
			this->buffEntrySize = curNode->buff->buff.size();
	}
	else
		this->buffEntrySize = 0;

	this->entries_N = vector<int>(maxNodeEntries);
	this->entries_sumX = vector<double*>(maxNodeEntries);
	//this->entries_sumX2 = vector<double*>(maxNodeEntries);
	this->entries_childID = vector<int>(maxNodeEntries);

	int i = 0;
	for (i = 0; i < this->entrySize; i++)
	{
		CFEntry* temp = curNode->entries[i].first;
		this->entries_N[i] = temp->n;
		this->entries_sumX[i] = temp->sumX;
		//this->entries_sumX2[i] = temp->sumX2;
		this->entries_childID[i] = temp->childId;
	}*/
}

IOBuffer::IOBuffer()
{}

IOBuffer::IOBuffer(Buffer* curBuffer)
{
	this->id = curBuffer->treeNode_ID;
	this->buffEntrySize = curBuffer->buff.size();
}
