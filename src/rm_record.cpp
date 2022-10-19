#include "rm_record.h"
#include <cstdlib>
#include <cstring>
RM_Record::RM_Record()
    : recordSize(-1), data(NULL), rid(-1, -1)
{
}

RM_Record::~RM_Record()
{
    if (data != NULL)
    {
        delete[] data;
    }
}
RC RM_Record::GetData(char *&pData, int &length) const
{
    if (data != NULL && recordSize != -1)
    {
        pData = data;
        length = recordSize;
        return 0;
    }
    else
        return RM_NULLRECORD;
}
RC RM_Record::Set(char *pData, int size, RID rid)
{
    if (recordSize != -1 && (size != recordSize))
        return RM_RECSIZEMISMATCH;
    recordSize = size;
    this->rid = rid;
    if (data == NULL)
        data = new char[recordSize];
    memcpy(data, pData, size);
    return 0;
}
RC RM_Record::GetRid(RID &rid) const
{
    if (data != NULL && recordSize != -1)
    {
        rid = this->rid;
        return 0;
    }
    else
        return RM_NULLRECORD;
}