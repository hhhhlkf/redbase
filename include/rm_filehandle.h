#if !defined(RM_FILEHANDLE_H)
#define RM_FILEHANDLE_H
#include "rm.h"
#include "pf.h"
#include "pf_filehandle.h"
#include <vector>
#include "rm_rid.h"
#include "rm_record.h"
class RM_FileHandle
{
    friend class RM_Manager;
    friend class RM_FileScan;
public:
    RC InsertRec(const char *pData, int length);
    RC GetRec(const RID &rid, RM_Record &rec) const;
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
