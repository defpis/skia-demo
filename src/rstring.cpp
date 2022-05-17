#include "rstring.h"


UErrorCode setCommonData(std::string path) {
    std::ifstream file;
    file.open(path);

    file.seekg(0, std::ios::end);
    long length = file.tellg();
    file.seekg(0, std::ios::beg);

    char* buffer = new char[length];

    file.read(buffer, length);
    file.close();

    UErrorCode code = U_ZERO_ERROR;
    udata_setCommonData(buffer, &code);

    return code;
}