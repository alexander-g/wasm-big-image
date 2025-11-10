#pragma once

// TODO: convert util.h to c++



#include <cstdint>
#include <memory>


struct Buffer {
    uint8_t* data;
    uint64_t size;
};

typedef std::shared_ptr<Buffer> Buffer_p;

