#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <memory>
#include <setjmp.h>
#include <vector>

#include <jpeglib.h>
#include <turbojpeg.h>

#include "./jpeg-io.hpp"
extern "C" {
#include "./cb_cookiefile.h"
#include "./util.h"
}



struct my_error_mgr { 
    jpeg_error_mgr pub; 
    jmp_buf setjmp_buffer; 
};
typedef struct my_error_mgr* my_error_ptr;

void my_error_exit(j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message) (cinfo);

    throw std::runtime_error("Unexpected libjpeg error");
    //longjmp(myerr->setjmp_buffer, 1);
}


typedef struct cb_srcmgr_handle {
    struct jpeg_source_mgr pub;
    
    jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    //jpeg_error_mgr jerr;

    struct cb_handle cb_handle;
    uint8_t buffer[4096];
} cb_srcmgr_handle;


void jpeg_srcmgr_init_source(j_decompress_ptr cinfo) {
    //printf(">>>>>> jpeg_srcmgr_init_source() <<<<<<<<\n");
}

boolean jpeg_srcmgr_fill_input_buffer(j_decompress_ptr cinfo){
    cb_srcmgr_handle* src = (cb_srcmgr_handle*)cinfo->src;
    const int nbytes = 
        cb_fread((void*)&src->cb_handle, (char*) src->buffer, sizeof(src->buffer));
    if(nbytes <= 0) {
        // no more data, inserting EOI marker
        src->buffer[0] = (JOCTET)0xFF;
        src->buffer[1] = (JOCTET)JPEG_EOI;
        src->pub.next_input_byte = src->buffer;
        src->pub.bytes_in_buffer = 2;
        return true;
    }
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = (size_t)nbytes;
    return true;
}

void jpeg_srcmgr_skip_input_data(j_decompress_ptr cinfo, long nbytes){
    if(nbytes <= 0)
        return;
    
    cb_srcmgr_handle* src = (cb_srcmgr_handle*)cinfo->src;
    long remaining = src->pub.bytes_in_buffer - nbytes;

    if(remaining < 0) {
        const int64_t rc = cb_fseek((void*)&src->cb_handle, -remaining, SEEK_CUR);
        remaining = 0;
    }

    src->pub.next_input_byte += nbytes;
    src->pub.bytes_in_buffer = remaining;
}


void jpeg_srcmgr_term_source(j_decompress_ptr cinfo){
    printf(">>>>>> jpeg_srcmgr_term_source() <<<<<<<<\n");
    // no-op ?
}



std::expected<std::shared_ptr<cb_srcmgr_handle>, int> jpeg_via_cb_init(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle
) {
    int rc;

    std::shared_ptr<cb_srcmgr_handle> srcmgr;
    try {
        srcmgr = std::make_shared<cb_srcmgr_handle>(cb_srcmgr_handle{
            .cb_handle = (struct cb_handle){
                .size   = filesize,
                .cursor = 0,
                .read_file_callback = 
                    (read_file_callback_ptr_t) read_file_callback_p,
                .read_file_handle   = read_file_handle,
            }
        });
    } catch (const std::bad_alloc& e) {
        return std::unexpected(MALLOC_FAILED);;
    }

    jpeg_decompress_struct& cinfo = srcmgr->cinfo;
    cinfo.err = jpeg_std_error(&srcmgr->jerr.pub);
    srcmgr->jerr.pub.error_exit = my_error_exit;

    jpeg_create_decompress(&cinfo);

    cinfo.src = &srcmgr->pub;
    
    cinfo.src->bytes_in_buffer = 0;
    cinfo.src->next_input_byte = NULL;
    cinfo.src->init_source       = jpeg_srcmgr_init_source;
    cinfo.src->fill_input_buffer = jpeg_srcmgr_fill_input_buffer;
    cinfo.src->skip_input_data   = jpeg_srcmgr_skip_input_data;
    cinfo.src->resync_to_restart = jpeg_resync_to_restart; //default
    cinfo.src->term_source       = jpeg_srcmgr_term_source;

    rc = jpeg_read_header(&cinfo, TRUE);
    if(rc != 1) 
        return std::unexpected(JPEG_READ_HEADER_FAILED);
    

    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;

    cinfo.out_color_space = JCS_EXT_RGBA;
    cinfo.out_color_components = 4;

    rc = jpeg_start_decompress(&cinfo);
    if(rc != 1) 
        return std::unexpected(JPEG_START_DECOMPRESS_FAILED);

    if (cinfo.output_components != 4) 
        return std::unexpected(JPEG_UNSUPPORTED_N_CHANNELS);

    return srcmgr;
}


//extern "C" {
int jpeg_read_patch(
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
    // return code (because of wasm issues)
    int*        returncode
) {
    int rc;
    if(returncode != NULL) *returncode = UNEXPECTED;

    if(src_x < 0
    || src_y < 0){
        if(returncode != NULL) *returncode = NEGATIVE_OFFSETS;
        return NEGATIVE_OFFSETS;
    }
    
    if(src_height < 1
    || src_width  < 1
    || dst_height < 1
    || dst_width  < 1){
        if(returncode != NULL) *returncode = INVALID_SIZES;
        return INVALID_SIZES;
    }

    if(dst_buffersize < (dst_height * dst_width * 4) ){
        if(returncode != NULL) *returncode = BUFFER_TOO_SMALL;
        return BUFFER_TOO_SMALL;
    }

    try {

    const auto ok = jpeg_via_cb_init(
        filesize,
        read_file_callback_p,
        read_file_handle
    );
    if(!ok) {
        if(returncode != NULL) *returncode = ok.error();
        return ok.error();
    }

    auto& cinfo = ok->get()->cinfo;
    if(src_x >= cinfo.image_width 
    || src_y >= cinfo.image_height){
        rc = OFFSETS_OUT_OF_BOUNDS;
        if(returncode != NULL) *returncode = rc;
        return rc;
    }


    step_and_fill_struct snf;
    rc = compute_step_and_fill(
        cinfo.image_width, 
        cinfo.image_height, 
        src_x, 
        src_y, 
        src_width, 
        src_height, 
        dst_width, 
        dst_height, 
        &snf
    );
    if(rc != OK) {
        if(returncode != NULL) *returncode = rc;
        return rc;
    }


    int scanline_x = src_x;
    // NOTE: cropping set after jpeg_start_decompress
    jpeg_crop_scanline(&cinfo, (JDIMENSION*)&scanline_x, (JDIMENSION*)&snf.read_width);

    const int nbytes_row = cinfo.output_width * cinfo.out_color_components;
    std::vector<uint8_t> rowbuffer(nbytes_row);


    int cursor_y = 0;
    cursor_y += jpeg_skip_scanlines(&cinfo, src_y) - 1;


    for(int output_y = 0; output_y < snf.fill_height; output_y++){
        const int image_y = src_y + round(output_y * snf.step_y);

        uint8_t* rowptr = rowbuffer.data();
        while(image_y > cursor_y)
            cursor_y += jpeg_read_scanlines(&cinfo, &rowptr, 1);
        
        copy_and_resize_from_patch_into_output(
            rowptr,
            scanline_x,
            image_y,
            cinfo.output_width,
            /*height=*/1,
            cinfo.image_height,
            (uint8_t*)dst_buffer,
            src_x,
            src_y,
            dst_width,
            dst_height,
            snf.fill_width,
            snf.fill_height,
            snf.step_x,
            snf.step_y,
            cinfo.out_color_components
        );
    }

    } catch (...) {
        return UNEXPECTED;
    }

    printf("TODO: jpeg_destroy_decompress()\n");

    if(returncode != NULL) *returncode = OK;
    return OK;
}


int jpeg_get_size(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    // outputs
    int32_t*    width,
    int32_t*    height,
    // return code (because of wasm issues)
    int*        returncode
) {
    int rc;
    if(returncode != NULL) *returncode = UNEXPECTED;
    try {
        const auto ok = jpeg_via_cb_init(
            filesize,
            read_file_callback_p,
            read_file_handle
        );
        if(ok) {
            *width  = ok->get()->cinfo.image_width;
            *height = ok->get()->cinfo.image_height;
        }
        rc = ok? OK : ok.error();
    } catch (...) {
        return UNEXPECTED;
    }

    if(returncode != NULL) *returncode = rc;
    return rc;
}




std::expected<Buffer_p, int> jpeg_compress(
    // input
    const uint8_t* rgba, 
    int32_t width, 
    int32_t height
) {
    tjhandle handle = tjInitCompress();
    if(!handle) 
        return std::unexpected(TURBOJPEG_INIT_FAILED);
    
    unsigned char *jpeg_buf = NULL;
    unsigned long jpeg_size = 0;
    const int subsamp = TJSAMP_444; // keep alpha ignored, full chroma
    const int quality = 95;

    int rc;
    rc = tjCompress2(
        handle,
        rgba,
        width,
        0,
        height,
        TJPF_RGBA,
        &jpeg_buf,
        &jpeg_size,
        subsamp,
        quality,
        TJFLAG_FASTDCT
    );
    tjDestroy(handle);
    if(rc != 0) 
        return std::unexpected(TURBOJPEG_COMPRESS_FAILED);


    const auto jpeg_deleter = [](Buffer* b){ 
        if(!b)
            return;
        tjFree(b->data); 
        delete b;
    };
    return Buffer_p( new Buffer{jpeg_buf, jpeg_size}, jpeg_deleter );
}



//} //extern "C"
