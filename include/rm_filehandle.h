#if !defined(RM_FILEHANDLE_H)
#define RM_FILEHANDLE_H
#include "rm.h"
#include "pf.h"
#include "pf_filehandle.h"
#include <vector>
#include "rm_rid.h"
class RM_FileHandle
{
    friend class RM_Manager;

public:
    RC InsertRec(const char *pData, int length);

private:
    bool openedFH;
    PF_FileHdr header;
    PF_FileHandle pfh;
    bool header_modified;

    vector<RID *> ridtable;

public:
    RM_FileHandle();
    ~RM_FileHandle();
};

#endif // RM_FILEHANDLE_H
