#include "./util.hpp"



FileHandle::FileHandle(FILE* f, size_t size): f(f), size(size) {
}

FileHandle::~FileHandle() {
    fclose(this->f);
}


std::optional<std::shared_ptr<FileHandle>> FileHandle::open(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f)
        return std::nullopt;
    
    if (fseek(f, 0, SEEK_END) != 0) 
        return std::nullopt;

    const long size = ftell(f);
    if(size < 0)
        return std::nullopt;
    
    if( fseek(f, 0, SEEK_SET) != 0)
        return std::nullopt;
    
    return std::shared_ptr<FileHandle>( new FileHandle( f, (size_t)size ) );
}


int FileHandle::read_callback(
    const void* handle,
    void*       dstbuf, 
    uint64_t    start, 
    uint64_t    size
) {
    FileHandle* file_handle = (FileHandle*) handle;

    const int rc = fseek(file_handle->f, start, SEEK_SET);
    if(rc != 0)
        return -1;
    return fread(dstbuf, 1, size, file_handle->f);
}

