#include "./util.hpp"


int open_asset(const char* path, FILE** fp, size_t* fsize) {
    FILE* f = fopen(path, "rb");
    if(f == NULL)
        return -1;
    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return -1; 
    }
    const long size = ftell(f);
    if(size < 0) { 
        fclose(f); 
        return -1; 
    }

    *fp = f;
    *fsize = (size_t) size;
    return 0;
}


int mock_read_callback(void* handle, void* dstbuf, uint64_t start, uint64_t size) {
    //printf("\nDBG>>> %x %i %i\n", handle, start, size); fflush(stdout);

    FILE* fp = (FILE*) handle;
    fseek(fp, start, SEEK_SET);
    fread(dstbuf, 1, size, fp);

    // TODO
    return 0;
}


