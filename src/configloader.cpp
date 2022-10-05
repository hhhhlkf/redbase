#include "configloader.h"
configloader::~configloader()
{
}

RC configloader::loadConfig()
{
    ifstream fin(path);
    json j;
    fin >> j;
    cfg = new config(j["bufferSize"],
                     j["diskSize"],
                     j["bufferBlock"]);
    // cout << j["bufferSize"] << " "
    //      << j["bufferBlock"] << " "
    //      << j["diskSize"] << endl;
    return 0;
}
