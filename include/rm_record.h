#if !defined(RM_RECORD_H)
#define RM_RECORD_H
#include "rm.h"
#include "rm_rid.h"
class RM_Record
{
private:
    int recordSize;
    char *data;
    RID rid;

public:
    RM_Record(/* args */);
    ~RM_Record();
    RC GetData(char *&pData, int &length) const;
    RC Set(char *pData, int size, RID rid);
    RC GetRid(RID &rid) const;
};

#endif // RM_RECORD_H
