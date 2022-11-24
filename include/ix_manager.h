#if !defined(IX_MANAGER_H)
#define IX_MANAGER_H
#include "redbase.h" // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf_manager.h"
#include "ix_indexhandle.h"
#include "ix.h"
class IX_Manager
{
private:
    PF_Manager &pfm;

public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength,
                   int length, int pageSize = PF_PAGE_SIZE);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);
};

#endif // IX_MANAGER_H
