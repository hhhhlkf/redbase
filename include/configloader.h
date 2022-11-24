#if !defined(CONFIGLOADER_H)
#define CONFIGLOADER_H
#include <iostream>
#include "config.h"
#include "redbase.h"
#include "nlohmann/json.hpp"
#include <fstream>

using json = nlohmann::json;
class configloader
{
private:
    /* data */
    config *cfg;
    const char* path;

public:
    configloader(string path)
    {
        this->path = path.c_str();
        cfg = nullptr;
    };
    ~configloader();
    RC loadConfig();
    config* getConfig() { return cfg; }
    config* getConfig() { return cfg; }
};

#endif // CONFIGLOADER_H
