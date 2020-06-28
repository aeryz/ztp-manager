
//
// Created by aeryz on 11/20/19.
//


#ifndef ZTP_DATATYPES_H
#define ZTP_DATATYPES_H

#include <iostream>

enum ScanStatus
{
    FAILED = -1,
    ONGOING,
    STOPPED,
    DONE
};

enum ServerErrorCode
{
    NOT_FOUND       = 404,
    SUCCESS         = 200,
    INTERNAL        = 500,
    NOT_IMPLEMENTED = 501,
    BAD_REQUEST     = 404,
    UNKNOWN         = 502
};

struct ServerResponseData
{
    enum ServerErrorCode err_code;
    std::string data;

    ServerResponseData() : err_code(SUCCESS) {}
    ServerResponseData(enum ServerErrorCode e_code, std::string data) : err_code(e_code), data(data) {}
};

#define ZTP_DB "ztp-dev"

#endif //ZTP_DATATYPES_H
