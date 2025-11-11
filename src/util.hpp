#pragma once

// TODO: convert util.h to c++



#include <cstdint>
#include <memory>
#include <optional>



struct Buffer {
    uint8_t* data;
    uint64_t size;
};

typedef std::shared_ptr<Buffer> Buffer_p;




struct FileHandle {
    // factory function
    static std::optional<std::shared_ptr<FileHandle>> open(const char* path);
    
    // callback to pass to image read functions
    static int read_callback(const void*, void*, uint64_t, uint64_t);

    // file size, also needed by image read functions
    size_t size;

    ~FileHandle();

    private:
    FileHandle(FILE*, size_t);
    FILE* f;
};

