#include <stdlib.h>
#include <tiffio.h>

#include "./tiff-io.h"
#include "./cb_cookiefile.h"



typedef void (*finished_callback_ptr_t)(int rc);



TIFF* tiff_client_open(
    size_t                   size, 
    read_file_callback_ptr_t read_file_callback,
    const void*              read_file_handle
);

int tiff_get_size(
    size_t      filesize,
    const read_file_callback_ptr_t read_file_callback_p,
    const void* read_file_handle,
    size_t*     width,  // TODO: make this explicit uint64_t
    size_t*     height,
    TIFF**      tif_p
) {
    TIFF* tif = tiff_client_open(filesize, read_file_callback_p, read_file_handle);
    if(tif == NULL)
        return -1;
    
    int rc;
    int w,h;
    rc = TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    if(rc != 1 || w <= 0){
        TIFFClose(tif);
        return -2;
    }
    rc = TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    if(rc != 1 || h <= 0){
        TIFFClose(tif);
        return -3;
    }
    *width  = (size_t) w;
    *height = (size_t) h;
    if(tif_p != NULL)
        *tif_p = tif;
    
    return 0;
}

int tiff_read(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    void*       buffer,
    size_t      buffersize
){
    int rc;
    size_t w,h;
    TIFF* tif;
    rc = tiff_get_size(filesize, read_file_callback_p, read_file_handle, &w, &h, &tif);
    if(rc != 0){
        return rc;
    }
    
    const size_t npixels = (size_t)w * h;
    const size_t required_size = npixels * sizeof(uint32_t);
    if(buffersize < required_size) { 
        TIFFClose(tif); 
        return -5; 
    }
    if(!TIFFReadRGBAImageOriented(tif, w, h, buffer, ORIENTATION_TOPLEFT, 0)) {
        TIFFClose(tif); 
        return -6;
    }
    
    return 0;
}




toff_t _tiff_client_size(thandle_t handle) {
    return ((struct cb_handle*) handle)->size;
}

// just to silence warnings
tmsize_t _tiff_client_read(thandle_t handle, void* buffer, tmsize_t size){
    return cb_fread(handle, buffer, size);
}

// just to silence warnings
toff_t _tiff_client_seek(thandle_t handle, toff_t offset, int whence) {
    return cb_fseek(handle, offset, whence);
}

// just to silence warnings
int _tiff_client_close(thandle_t handle) {
    return cb_fclose(handle);
}

// writing disabled
tmsize_t _tiff_client_write(thandle_t handle, void* buffer, tmsize_t size) {
    return 0;
}



TIFF* tiff_client_open(
    size_t                   size, 
    read_file_callback_ptr_t read_file_callback,
    const void*              read_file_handle
){
    struct cb_handle* handle = malloc(sizeof(struct cb_handle));
    if (!handle) 
        return (NULL);
    *handle = (struct cb_handle){
        .cursor = 0,
        .size   = size,
        .read_file_callback = read_file_callback,
        .read_file_handle   = read_file_handle,
    };
    
    TIFF* tif = TIFFClientOpen(
        /* filename   = */ "filename.tiff", 
        /* mode       = */ "r", 
        /* clientdata = */ handle, 
        /* readproc   = */ _tiff_client_read, 
        /* writeproc  = */ _tiff_client_write, 
        /* seekproc   = */ _tiff_client_seek, 
        /* closeproc  = */ _tiff_client_close, 
        /* sizeproc   = */ _tiff_client_size, 
        /* mapproc    = */ NULL, 
        /* unmapproc  = */ NULL
    );
    if(tif == NULL)
        cb_fclose(handle);
    return tif;
}

