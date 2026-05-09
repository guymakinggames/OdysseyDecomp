#pragma once

#include <basis/seadTypes.h>

struct LinkIdPairBytes {
    u8 current;
    u8 previous;
};

struct LinkIdPair {
    union {
        u16 packed = 0;
        LinkIdPairBytes bytes;
    };

    void clear() { packed = 0; }
    void setCurrent(u8 value) { bytes.current = value; }
    void shiftCurrentToPrevious() {
        bytes.previous = bytes.current;
        bytes.current = 0;
    }
    bool isCurrentOrPreviousSet() const { return packed > 0xff || (packed & 0xff) != 0; }
    bool isCurrentSet() const { return bytes.current != 0; }
    bool isPreviousSet() const { return bytes.previous != 0; }
    void clearPrevious() { bytes.previous = 0; }
    void setPrevious(u8 value) { bytes.previous = value; }
};

static_assert(sizeof(LinkIdPair) == 0x2);
