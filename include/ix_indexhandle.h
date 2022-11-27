#if !defined(IX_INDEXHANDLE_H)
#define IX_INDEXHANDLE_H
#include "redbase.h" // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"
#include "btree_node.h"
struct IX_FileHdr
{
    int numPages;     // # of pages in the file
    int pageSize;     // size per index node - usually PF_PAGE_SIZE
    PageNum rootPage; // addr of root page
    int pairSize;     // size of each (key, RID) pair in index
    int order;        // order of btree
    int height;       // height of btree
    AttrType attrType;
    int attrLength;
};

const int IX_PAGE_LIST_END = -1;

class IX_IndexHandle
{
    friend class IX_Manager;

private:
    RC GetThisPage(PageNum p, PF_PageHandle &ph) const;

    IX_FileHdr hdr;
    bool bFileOpen;
    PF_FileHandle *pfHandle;
    bool bHdrChanged;
    BtreeNode *root;  // root in turn points to the other nodes
    BtreeNode **path; // list of nodes that is the path to leaf as a result of a search.
                      //
    int *pathP;       // 父母在路径中的位置指向子节点。 the parent in the path the points to the child node.
                      //

    void *treeLargest; // largest key in the entire tree
public:
    IX_IndexHandle(/* args */);
    ~IX_IndexHandle();
    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Search an index entry
    // return -ve if error
    // 0 if found
    // IX_KEYNOTFOUND if not found
    RC Search(void *pData, RID &rid);

    // Force index files to disk
    RC ForcePages();

    RC Open(PF_FileHandle *pfh, int pairSize, PageNum p, int pageSize);
    RC GetFileHeader(PF_PageHandle ph);
    // persist header into the first page of a file for later
    RC SetFileHeader(PF_PageHandle ph) const;

    bool HdrChanged() const { return bHdrChanged; }
    int GetNumPages() const { return hdr.numPages; }
    AttrType GetAttrType() const { return hdr.attrType; }
    int GetAttrLength() const { return hdr.attrLength; }

    RC GetNewPage(PageNum &pageNum);
    RC DisposePage(const PageNum &pageNum);

    RC IsValid() const;

    // return NULL if the key is bad
    // otherwise return a pointer to the leaf node where key might go
    // also populates the path member variable with the path
    BtreeNode *FindLeaf(const void *pData);
    BtreeNode *FindSmallestLeaf();
    BtreeNode *FindLargestLeaf();
    BtreeNode *DupScanLeftFind(BtreeNode *right,
                               void *pData,
                               const RID &rid);
    // hack for indexscan::OpOptimize
    BtreeNode *FindLeafForceRight(const void *pData);

    BtreeNode *FetchNode(RID r) const;
    BtreeNode *FetchNode(PageNum p) const;
    // void ResetNode(BtreeNode*& old, PageNum p) const;
    // Reset to the BtreeNode at the RID specified within Btree
    // void ResetNode(BtreeNode*& old, RID r) const;

    // get/set height
    int GetHeight() const;
    void SetHeight(const int &);

    BtreeNode *GetRoot() const;
    void Print(ostream &, int level = -1, RID r = RID(-1, -1)) const;

    RC Pin(PageNum p);
    RC UnPin(PageNum p);
};

#endif // IX_INDEXHANDLE_H
