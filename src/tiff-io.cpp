#include <cmath>
#include <expected>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <tiffio.h>

#include "./tiff-io.hpp"
extern "C" {
#include "./cb_cookiefile.h"
#include "./util.h"
}



int tiff_read(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    void*       buffer,
    size_t      buffersize
){
    int rc;

    const auto expect_tiff = TIFF_Handle::create(
        filesize, 
        (read_file_callback_ptr_t)read_file_callback_p, 
        read_file_handle
    );
    if(!expect_tiff)
        return expect_tiff.error();
    const std::shared_ptr<TIFF_Handle> tiff = expect_tiff.value();
    
    const size_t npixels = (size_t)tiff->width * tiff->height;
    const size_t required_size = npixels * sizeof(uint32_t);
    if(buffersize < required_size) 
        return BUFFER_TOO_SMALL; 
    
    rc = TIFFReadRGBAImageOriented(
        (TIFF*)tiff->tif, 
        tiff->width, 
        tiff->height, 
        (uint32_t*)buffer, 
        ORIENTATION_TOPLEFT, 
        /*stopOnError=*/1
    );
    if(!rc)
        return TIFF_READ_FULL_FAILED;
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


    // 32bit rgba
    const size_t pixel_size = sizeof(uint32_t);
    const size_t nbytes_required = dst_width * dst_height * pixel_size;
    if (buffersize < nbytes_required) 
        return BUFFER_TOO_SMALL;

    uint8_t *out = (uint8_t*)buffer;

    const uint32_t n_strips = TIFFNumberOfStrips(tif);
    uint32_t rows_per_strip = 0;
    rc = TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
    if(rc != 1 || rows_per_strip == 0)
        return TIFF_GET_ROWS_PER_STRIP_FAILED;

    memset(out, 0, nbytes_required);
    const uint32_t nbytes_strip  = image_width * rows_per_strip * pixel_size;
    uint8_t* stripbuffer = (uint8_t*)malloc(nbytes_strip);
    if(stripbuffer == NULL)
        return MALLOC_FAILED;
    

    step_and_fill_struct snf;
    rc = compute_step_and_fill(
        image_width, 
        image_height, 
        src_x, 
        src_y, 
        src_width, 
        src_height, 
        dst_width, 
        dst_height, 
        &snf
    );
    if(rc != OK)
        return rc;
    
    for(int output_y = 0; output_y < snf.fill_height; ){
        const int image_y = src_y + round(output_y * snf.step_y);
        // TODO:  use TIFFComputeStrip()
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
            return TIFF_READ_STRIP_FAILED;
        }

        // libtiff antics, rows start at the bottom, but not in the last strip
        double strip_y = 
            min(rows_per_strip - 1, image_height - 1 - strip * rows_per_strip);
        // discard offset rows
        strip_y -= image_y % rows_per_strip;
        // should not do anything but just in case
        strip_y  = max(0, strip_y);

        for(; output_y < snf.fill_height; output_y++){
            // stop if we need to read a new strip
            // TODO: use TIFFComputeStrip()
            const int image_y2 = src_y + round(output_y * snf.step_y);
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
                snf.fill_width,
                snf.step_x,
                pixel_size,
                out
            );
            strip_y = max(strip_y - snf.step_y, 0);
        }
    }
    free(stripbuffer);
    return OK;
}





static int tiff_read_patch_tiles(
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
    step_and_fill_struct snf;
    rc = compute_step_and_fill(
        image_width, 
        image_height, 
        src_x, 
        src_y, 
        src_width, 
        src_height, 
        dst_width, 
        dst_height, 
        &snf
    );
    if(rc != OK)
        return rc;


    uint32_t tile_width = 0, tile_height = 0, tiledepth = 0;
    rc = TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
    rc = TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_height);
    rc = TIFFGetField(tif, TIFFTAG_TILEDEPTH, &tiledepth);
    const uint32_t ntiles = TIFFNumberOfTiles(tif);
    // only zero depth supported
    if (tile_width == 0 || tile_height == 0 || tiledepth != 0 || ntiles == 0)
        return TIFF_GET_TILE_SIZES_FAILED; 
    
    // number of tiles in x and y dimensions
    const int ntiles_x = (image_width + tile_width -1) / tile_width;
    const int ntiles_y = (image_height + tile_height -1) / tile_height;
    

    // 32bit rgba
    const size_t pixel_size = sizeof(uint32_t);
    const size_t nbytes_tile = tile_width * tile_height * pixel_size;
    void* tilebuffer = malloc(nbytes_tile);
    if(tilebuffer == NULL) 
        return MALLOC_FAILED;
    
    // a boolean array indicating if a tile has been processed
    uint8_t* tiles_processed = (uint8_t*)malloc(ntiles);
    if(tiles_processed == NULL)  {
        free(tilebuffer);
        return MALLOC_FAILED;
    }
    memset(tiles_processed, 0, ntiles);

    for(int output_y = 0; output_y < snf.fill_height; output_y++){
        const int image_y = src_y + round(output_y * snf.step_y);

        for(int output_x = 0; output_x < snf.fill_width; output_x++){
            const int image_x = src_x + round(output_x * snf.step_x);

            uint32_t tile = TIFFComputeTile(tif, image_x, image_y, 0, 0);
            if(tiles_processed[tile])
                continue;
            
            tiles_processed[tile] = 1;
            
            const int tile_x = image_x - image_x % tile_width;
            const int tile_y = image_y - image_y % tile_height;
            
            //memset(tilebuffer, 0, nbytes_tile);
            rc = TIFFReadRGBATileExt(tif, tile_x, tile_y, (uint32_t*)tilebuffer, 0);
            if(rc != 1) {
                free(tilebuffer);
                free(tiles_processed);
                return TIFF_READ_TILE_FAILED;
            }

            
            copy_and_resize_from_patch_into_output(
                (const uint8_t*)tilebuffer,
                tile_x,
                tile_y,
                tile_width,
                tile_height,
                image_height,
                (uint8_t*)buffer,
                src_x,
                src_y,
                dst_width,
                dst_height,
                snf.fill_width,
                snf.fill_height,
                snf.step_x,
                snf.step_y,
                pixel_size
            );
        }

    }

    free(tilebuffer);
    free(tiles_processed);
    return OK;
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
    || src_y < 0)
        return NEGATIVE_OFFSETS;
    
    if(src_height < 1
    || src_width  < 1
    || dst_height < 1
    || dst_width  < 1)
        return INVALID_SIZES;

    if(dst_buffersize < (dst_height * dst_width * 4) )
        return BUFFER_TOO_SMALL;
    
    const auto expect_tiff = TIFF_Handle::create(
        filesize, 
        (read_file_callback_ptr_t)read_file_callback_p, 
        read_file_handle
    );
    if(!expect_tiff)
        return expect_tiff.error();
    const std::shared_ptr<TIFF_Handle> tiff = expect_tiff.value();

    int rc;
    if(src_x >= tiff->width 
    || src_y >= tiff->height){
        rc = OFFSETS_OUT_OF_BOUNDS;
        if(returncode != NULL) *returncode = rc;
        return rc;
    }

    if(TIFFIsTiled((TIFF*)tiff->tif)) {
        rc = tiff_read_patch_tiles(
            (TIFF*)tiff->tif, 
            tiff->width, 
            tiff->height,
            src_x, 
            src_y,
            src_width,
            src_height,
            dst_width,
            dst_height,
            dst_buffer,
            dst_buffersize
        );
    } else {
        rc = tiff_read_patch_strips(
            (TIFF*)tiff->tif, 
            tiff->width, 
            tiff->height,
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
    if(returncode != NULL) *returncode = rc;
    return rc;
}





toff_t _tiff_client_size(thandle_t handle) {
    return ((struct cb_handle*) handle)->size;
}

// just to silence warnings
tmsize_t _tiff_client_read(thandle_t handle, void* buffer, tmsize_t size){
    return cb_fread(handle, (char*)buffer, size);
}

// just to silence warnings
toff_t _tiff_client_seek(thandle_t handle, toff_t offset, int whence) {
    int64_t rc = cb_fseek(handle, offset, whence);
    return rc;
}

// just to silence warnings
int _tiff_client_close(thandle_t handle) {
    //return cb_fclose(handle);  // NOTE: cb_handle free is handled by ~TIFF_Handle
    return 0;
}

// writing disabled
tmsize_t _tiff_client_write(thandle_t handle, void* buffer, tmsize_t size) {
    return 0;
}



TIFF_Handle::TIFF_Handle(void* tif, std::shared_ptr<struct cb_handle> cb_handle):
    tif(tif), 
    cb_handle(cb_handle)
{}

TIFF_Handle::~TIFF_Handle() {
    TIFFClose(static_cast<TIFF*>(this->tif));
}

std::expected<std::shared_ptr<TIFF_Handle>, int> TIFF_Handle::create(
    size_t filesize,
    const read_file_callback_ptr_t read_file_callback_p,
    const void* read_file_handle
) {
    const auto cb_handle_sp = std::make_shared<struct cb_handle>(
        (struct cb_handle){
            .size   = filesize,
            .cursor = 0,
            .read_file_callback = read_file_callback_p,
            .read_file_handle   = read_file_handle,
        }
    );

    TIFF* tif = TIFFClientOpen(
        /* filename   = */ "filename.tiff", 
        /* mode       = */ "r", 
        /* clientdata = */ cb_handle_sp.get(), 
        /* readproc   = */ _tiff_client_read, 
        /* writeproc  = */ _tiff_client_write, 
        /* seekproc   = */ _tiff_client_seek, 
        /* closeproc  = */ _tiff_client_close, 
        /* sizeproc   = */ _tiff_client_size, 
        /* mapproc    = */ NULL, 
        /* unmapproc  = */ NULL
    );
    if(tif == NULL)
        return std::unexpected(TIFF_OPEN_FAILED);

    const auto tif_handle = std::make_shared<TIFF_Handle>((void*)tif, cb_handle_sp);

    int rc;
    int w,h;
    rc = TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    if(rc != 1 || w <= 0)
        return std::unexpected(TIFF_GET_IMAGE_WIDTH_FAILED);
    
    rc = TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    if(rc != 1 || h <= 0)
        return std::unexpected(TIFF_GET_IMAGE_HEIGHT_FAILED);

    tif_handle->width = w;
    tif_handle->height = h;

    return tif_handle;
}
