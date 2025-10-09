#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#include "./tiff-io.h"
#include "./cb_cookiefile.h"
#include "./util.h"



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
    if(!TIFFReadRGBAImageOriented(tif, w, h, buffer, ORIENTATION_TOPLEFT, 1)) {
        TIFFClose(tif); 
        return -6;
    }
    
    return 0;
}



static void copy_scanline(
    const uint8_t* stripbuffer, 
    int strip_y,       // row within the stripbuffer
    int output_y,      // row within the outputbuffer
    int offset_x,      // column within the stripbuffer
    int image_width,   // width of the strip / image
    int patch_width,   // width of the output buffer
    int read_width,    // width to read, might be smaller than patch width if oob
    size_t pixel_size,
    uint8_t* out
) {
    const int src = (strip_y * image_width + offset_x) * pixel_size;
    const int dst = output_y * patch_width * pixel_size;
    memcpy(out + dst, stripbuffer + src, (size_t)read_width * pixel_size);
}

static int tiff_read_patch_strips(
    TIFF*   tif,
    int     image_width,
    int     image_height,
    int     offset_x,
    int     offset_y,
    int     patch_width,
    int     patch_height,
    void*   buffer,
    size_t  buffersize
) {
    int rc;

    int read_height = patch_height;
    int read_width  = patch_width;
    // clamp to image bounds
    if (offset_x + patch_width > image_width)
        read_width = image_width - offset_x;
    if (offset_y + patch_height > image_height)
        read_height = image_height - offset_y;
    if (read_width == 0 || read_height == 0)
        return -3;

    // 32bit rgba
    const size_t pixel_size = sizeof(uint32_t);
    const size_t nbytes_required = patch_width * patch_height * pixel_size;
    if (buffersize < nbytes_required) 
        return -4;

    uint8_t *out = (uint8_t*)buffer;

    const uint32_t n_strips = TIFFNumberOfStrips(tif);
    uint32_t rows_per_strip = 0;
    rc = TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
    if(rc != 1 || rows_per_strip == 0)
        return -5;

    memset(out, 0, nbytes_required);
    const uint32_t nbytes_strip  = image_width * rows_per_strip * pixel_size;
    uint8_t* stripbuffer = malloc(nbytes_strip);
    if(stripbuffer == NULL)
        return -6;
    
    for(int output_y = 0; output_y < read_height; ){
        const int image_y = offset_y + output_y;
        const int strip = image_y / rows_per_strip;
        const int first_scanline_of_strip = strip * rows_per_strip;
        

        memset(stripbuffer, 0, nbytes_strip);
        rc = TIFFReadRGBAStripExt(
            tif, 
            first_scanline_of_strip, 
            (uint32_t*)stripbuffer, 
            /*stop_on_error = */ 1
        );
        if(rc != 1) {
            free(stripbuffer);
            return -7;
        }

        // libtiff antics, rows start at the bottom, but not in the last strip
        int strip_y = min(rows_per_strip - 1, image_height - 1 - strip * rows_per_strip);
        // discard offset rows
        strip_y -= image_y % rows_per_strip;

        for(; output_y < read_height && strip_y >= 0; output_y++, strip_y--){
            copy_scanline(
                stripbuffer, 
                strip_y,
                output_y, 
                offset_x, 
                image_width, 
                patch_width,
                read_width,
                pixel_size,
                out
            );
        }
    }
    free(stripbuffer);
    return 0;
}



int tiff_read_patch(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    int         offset_x,
    int         offset_y,
    int         patch_width,
    int         patch_height,
    void*       buffer,
    size_t      buffersize
){  
    if(offset_x < 0
    || offset_y < 0
    || patch_height < 1
    || patch_width  < 1
    || buffersize < (patch_height * patch_width * 4) )
        return -1;
    

    int rc;
    size_t image_width, image_height;
    TIFF* tif;
    rc = tiff_get_size(
        filesize, 
        read_file_callback_p, 
        read_file_handle, 
        &image_width, 
        &image_height, 
        &tif
    );
    if(rc != 0)
        return rc;

    if(offset_x >= image_width 
    || offset_y >= image_height){
        TIFFClose(tif);
        return -2;
    }



    if(TIFFIsTiled(tif)) {
        // TODO
        return -999;
    } else {
        rc = tiff_read_patch_strips(
            tif, 
            image_width, 
            image_height,
            offset_x, 
            offset_y, 
            patch_width,
            patch_height,
            buffer,
            buffersize
        );
    }
    TIFFClose(tif); 
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

