#if !defined(INITLIB_H)
#define INITLIB_H
#include "datalib/relationTable.h"
#include "redbase.h"
#include <vector>
#include <string>
#include <string.h>

class initLib
{
private:
    vector<string> split(string str, string pattern);

public:
    initLib();
    ~initLib();
    relationTable *dataLibrary;
    char *exdata(char *record, int &size);
    string rexdata(char *record, int size);
};

#endif // INITLIB_H
