#if !defined(RELATIONTABLE_H)
#define RELATIONTABLE_H

#include "../redbase.h"
#include "propertyTable.h"
#include <vector>
#include <string>
class relationTable
{
private:
    int id;
    string relName;
    string creator;
    int propsNum;
    int rcdLen;
    int totalNum;

public:
    vector<propertyTable *> propsLink;
    relationTable(int id, string relName, string creator, int propsNum, int totalNum)
        : id(id), relName(relName), creator(creator), propsNum(propsNum), totalNum(totalNum){};
    ~relationTable(){};
    RC getRcdLen()
    {
        return this->rcdLen;
    };
    RC setRcdLen(int rcdLen)
    {
        if (rcdLen < 0 || rcdLen > MAXSTRINGLEN)
            return -1;
        else
        {
            this->rcdLen = rcdLen;
            return 0;
        }
    };
};

#endif // RELATIONTABLE_H
