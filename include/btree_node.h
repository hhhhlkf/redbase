#if !defined(BTREE_NODE_H)
#define BTREE_NODE_H
#include "redbase.h"
#include "pf_filehandle.h"
#include "rm_rid.h"
#include "ix.h"
#include <iosfwd>
class BtreeNode
{
    friend class IX_IndexHandle;

private:
    // serialized
    char *keys; // should not be accessed directly as keys[] but with SetKey()
    RID *rids;
    int numKeys;
    // not serialized - common to all ix pages
    int attrLength;
    AttrType attrType;
    int order;
    // not serialized - convenience
    RID pageRID;

public:
    BtreeNode(AttrType attrType, int attrLength,
              PF_PageHandle &ph, bool newPage = true,
              int pageSize = PF_PAGE_SIZE);
    RC ResetBtreeNode(PF_PageHandle &ph, const BtreeNode &rhs);
    ~BtreeNode();
    int Destroy();
    RC IsValid() const;
    int GetMaxKeys() const;
    int GetNumKeys();
    int SetNumKeys(int newNumKeys);
    int GetLeft();
    int SetLeft(PageNum p);
    int GetRight();
    int SetRight(PageNum p);
    RC GetKey(int pos, void *&key) const;
    int SetKey(int pos, const void *newkey);
    int CopyKey(int pos, void *toKey) const;
    int Insert(const void *newkey, const RID &newrid);
    int Remove(const void *newkey, int kpos = -1);

    int FindKey(const void *&key, const RID &r = RID(-1, -1)) const;
    RID FindAddr(const void *&key) const;
    RID GetAddr(const int pos) const;
    int FindKeyPosition(const void *&key) const;
    RID FindAddrAtPosition(const void *&key) const;
    RC Split(BtreeNode *rhs);
    RC Merge(BtreeNode *rhs);
    void Print(ostream &os);

    // get/set pageRID
    RID GetPageRID() const;
    void SetPageRID(const RID &);

    int CmpKey(const void *k1, const void *k2) const;
    bool isSorted() const;
    void *LargestKey() const;
};



#endif // BTREE_NODE_H
