#if !defined(PROPERTYTABLE_H)
#define PROPERTYTABLE_H
#include "../redbase.h"
#include <string>
class propertyTable
{
private:
public:
    string propName;
    RC length;
    string typeName;
    RC offset;
    string nullFlag;
    propertyTable(const char *propName, RC length, const char *typeName, const char *nullFlag)
        : propName(propName), length(length), typeName(typeName), nullFlag(nullFlag){};
    ~propertyTable();
};

#endif // PROPERTYTABLE_H
