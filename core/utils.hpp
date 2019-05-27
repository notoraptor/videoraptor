//
// Created by notoraptor on 23/07/2018.
//

#ifndef VIDEORAPTORBATCH_UTILS_HPP
#define VIDEORAPTORBATCH_UTILS_HPP

#include "HWDevices.hpp"

#ifdef WIN32
#define SEPARATOR '\\'
#define OTHER_SEPARATOR '/'
#else
#define SEPARATOR '/'
#define OTHER_SEPARATOR '\\'
#endif

#define PIXEL_FMT AV_PIX_FMT_RGBA

inline char* copyString(const char* initialString) {
    size_t stringLength = strlen(initialString);
    char* stringCopy = new char[stringLength + 1];
    memcpy(stringCopy, initialString, stringLength + 1);
    return stringCopy;
}

HWDevices* getHardwareDevices();


#endif //VIDEORAPTORBATCH_UTILS_HPP
