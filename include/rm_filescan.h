#if !defined(RM_FILESCAN_H)
#define RM_FILESCAN_H
#include "rm.h"
#include "rm_record.h"
#include "rm_filehandle.h"
#include "redbase.h"
#include "predicate.h"
class RM_FileScan
{
private:
    RM_FileHandle *prmh;
    RID current;
    bool bOpen;
    Predicate *pred;

public:
    RM_FileScan(/* args */);
    ~RM_FileScan();
    RC OpenScan(const RM_FileHandle &fileHandle,
                AttrType attrType,
                int attrLength,
                int attrOffset,
                CompOp compOp,
                void *value,
                ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);             // Get next matching record
};

#endif // RM_FILESCAN_H
