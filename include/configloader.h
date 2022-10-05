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
    const Ptr path;
public:
    configloader(const Ptr path) : path(path)
    {
        cfg = nullptr;
    };
    ~configloader();
    RC loadConfig();
};

#endif // CONFIGLOADER_H
