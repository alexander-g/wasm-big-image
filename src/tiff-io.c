#include <math.h>
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
    int src_x,         // column within the stripbuffer
    int image_width,   // width of the strip / image
    int dst_width,     // width of the output buffer
    int read_width,    // width to read, might be smaller than patch width if oob
    size_t pixel_size,
    uint8_t* out
) {
    const int src = (strip_y * image_width + src_x) * pixel_size;
    const int dst = output_y * dst_width * pixel_size;
    memcpy(out + dst, stripbuffer + src, (size_t)read_width * pixel_size);
}

static void copy_and_resize_scanline(
    const uint8_t* stripbuffer,
    int      strip_y,      // row within the stripbuffer
    int      output_y,     // row within the outputbuffer
    int      src_x,        // column within the stripbuffer
    int      image_width,  // width of the strip / image
    int      dst_width,    // width of the output buffer
    int      fill_width,   // # of pixels to fill, can be smaller than dst width
    double   step_x,       // step size
    size_t   pixel_size,
    uint8_t* out
) {
    const int src_base = strip_y * image_width * pixel_size;
    const int dst_base = output_y * dst_width * pixel_size;

    for(int output_x = 0; output_x < fill_width; output_x++){
        const int strip_x = src_x + (int)round(output_x * step_x);

        const int src = src_base + (int)round(strip_x) * pixel_size;
        const int dst = dst_base + output_x * pixel_size;

        out[dst+0] = stripbuffer[src+0];
        out[dst+1] = stripbuffer[src+1];
        out[dst+2] = stripbuffer[src+2];
        out[dst+3] = stripbuffer[src+3];

        
    }
}



static int tiff_read_patch_strips(
    TIFF*   tif,
    int     image_width,
    int     image_height,
    int     src_x,
    int     src_y,
    int     src_width,
    int     src_height,
    int     dst_width,
    int     dst_height,
    void*   buffer,
    size_t  buffersize
) {
    int rc;

    // last pixel to read
    int read_height = src_height;
    int read_width  = src_width;
    // clamp to image bounds
    if (src_x + src_width > image_width)
        read_width = image_width - src_x;
    if (src_y + src_height > image_height)
        read_height = image_height - src_y;
    if (read_width == 0 || read_height == 0)
        return -3;

    // 32bit rgba
    const size_t pixel_size = sizeof(uint32_t);
    const size_t nbytes_required = dst_width * dst_height * pixel_size;
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

    const double step_y = (double)src_height / dst_height;
    const double step_x = (double)src_width / dst_width;
    // maximum pixel within the output buffer
    const int fill_height = min( round(read_height / step_y), dst_height);
    const int fill_width  = min( round(read_width / step_x), dst_width );
    
    for(int output_y = 0; output_y < fill_height; ){
        const int image_y = src_y + round(output_y * step_y);
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
        double strip_y = 
            min(rows_per_strip - 1, image_height - 1 - strip * rows_per_strip);
        // discard offset rows
        strip_y -= image_y % rows_per_strip;
        // should not do anything but just in case
        strip_y  = max(0, strip_y);

        for(; output_y < fill_height; output_y++){
            // stop if we need to read a new strip
            const int image_y2 = src_y + round(output_y * step_y);
            const int strip2 = image_y2 / rows_per_strip;
            if(strip != strip2)
                break;

            copy_and_resize_scanline(
                stripbuffer,
                (int) round(strip_y),
                output_y, 
                src_x, 
                image_width, 
                dst_width,
                fill_width,
                step_x,
                pixel_size,
                out
            );
            strip_y = max(strip_y - step_y, 0);
        }
    }
    free(stripbuffer);
    return 0;
}



int tiff_read_patch(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    int         src_x,
    int         src_y,
    int         src_width,
    int         src_height,
    int         dst_width,
    int         dst_height,
    void*       dst_buffer,
    size_t      dst_buffersize,
    int*        returncode
){  
    if(src_x < 0
    || src_y < 0
    || src_height < 1
    || src_width  < 1
    || dst_height < 1
    || dst_width  < 1
    || dst_buffersize < (dst_height * dst_width * 4) )
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
    if(rc != 0) {
        if(returncode != NULL) *returncode = rc;
        return rc;
    }

    if(src_x >= image_width 
    || src_y >= image_height){
        TIFFClose(tif);
        if(returncode != NULL) *returncode = -2;
        return -2;
    }

    if(TIFFIsTiled(tif)) {
        // TODO
        printf("ERROR: reading tiled tiffs not implemented.\n");
        rc = -999;
    } else {
        rc = tiff_read_patch_strips(
            tif, 
            image_width, 
            image_height,
            src_x, 
            src_y,
            src_width,
            src_height,
            dst_width,
            dst_height,
            dst_buffer,
            dst_buffersize
        );
    }
    TIFFClose(tif); 
    if(returncode != NULL) *returncode = rc;
    return rc;
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
    int64_t rc = cb_fseek(handle, offset, whence);
    return rc;
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
    void *pv;
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
        cb_fclose(handle);  // = free()
    return tif;
}

