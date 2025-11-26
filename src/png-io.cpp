#include <cmath>
#include <expected>
#include <libpng/png.h>
#include <memory>
#include <stdexcept>
#include <vector>

#include "./png-io.hpp"
extern "C" {
#include "./cb_cookiefile.h"
#include "./util.h"
}




void cb_read_data(png_structp png, png_bytep data, png_size_t length) {
    void* cb_handle = (void*)png_get_io_ptr(png);
    const int nbytes = cb_fread(cb_handle, (char*)data, length);
    if(nbytes <= 0) {
        // EOF

    }
}

void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    printf(">>>> PNG:ERROR (%s)\n", error_msg);
    throw std::runtime_error("Unexpected libpng error");
}
void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
    printf(">>>> PNG:WARNING (%s)\n", warning_msg);
}




class PNG_Handle {
    public:
    png_structp png;
    png_infop   info;
    struct cb_handle cb_handle;

    uint32_t width;
    uint32_t height;

    PNG_Handle(png_structp png, png_infop info, struct cb_handle cb_handle):
        png(png), 
        info(info),
        cb_handle(cb_handle)
    {}

    ~PNG_Handle() {
        png_destroy_read_struct(&this->png, &this->info, NULL);
    }

    static std::expected<std::shared_ptr<PNG_Handle>, int> create(
        size_t      filesize,
        const void* read_file_callback_p,
        const void* read_file_handle
    ) {
        png_structp png = 
            png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info = png_create_info_struct(png);
        if (!png || !info) {
            return std::unexpected(PNG_INIT_LIB_FAILED);
        }

        auto png_handle_p = 
            std::make_shared<PNG_Handle>(
                png, 
                info,
                (struct cb_handle){
                    .size   = filesize,
                    .cursor = 0,
                    .read_file_callback = 
                        (read_file_callback_ptr_t) read_file_callback_p,
                    .read_file_handle = read_file_handle,
                }
            );


        png_set_read_fn(png, &png_handle_p->cb_handle, cb_read_data);
        png_set_error_fn(png, NULL, user_error_fn, user_warning_fn);

        png_read_info(png, info);
        png_uint_32 w = png_get_image_width(png, info);
        png_uint_32 h = png_get_image_height(png, info);
        png_byte color = png_get_color_type(png, info);
        png_byte depth = png_get_bit_depth(png, info);
        png_handle_p->width  = w;
        png_handle_p->height = h;

        // make sure data is 8-bit
        if (depth < 8)
            png_set_packing(png);
        if(depth == 16)
            png_set_strip_16(png);

        // palette to rgb
        if (color == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png);

        // gray to rgb
        if (color == PNG_COLOR_TYPE_GRAY) 
            png_set_gray_to_rgb(png);

        // no idea
        if (png_get_valid(png, info, PNG_INFO_tRNS)) 
            png_set_tRNS_to_alpha(png);
        
        color = png_get_color_type(png, info);
        if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png);

        // add a filler alpha channel if needed
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);

        png_read_update_info(png, info);
        
        //printf("w=%i h=%i c=%i d=%i\n", w, h, color, depth);

        return png_handle_p;
    }
};


int png_get_size(
    size_t      filesize,
    const void* read_file_callback_p,
    const void* read_file_handle,
    // outputs
    int32_t*    width,
    int32_t*    height,
    // return code (because of wasm issues)
    int*        returncode
) {
    if(returncode != NULL) *returncode = UNEXPECTED;

    try{
        std::expected<std::shared_ptr<PNG_Handle>, int> png_handle = 
            PNG_Handle::create(filesize, read_file_callback_p, read_file_handle);
        if(!png_handle) {
            if(returncode != NULL) *returncode = png_handle.error();
            return png_handle.error();
        }

        *width  = png_handle.value()->width;
        *height = png_handle.value()->height;
        
    } catch (...) {
        return UNEXPECTED;
    }

    if(returncode != NULL) *returncode = OK;
    return OK;
}



int png_read_patch(
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


    try{
        std::expected<std::shared_ptr<PNG_Handle>, int> expect_png_handle = 
            PNG_Handle::create(filesize, read_file_callback_p, read_file_handle);
        if(!expect_png_handle) {
            if(returncode != NULL) *returncode = expect_png_handle.error();
            return expect_png_handle.error();
        }
        std::shared_ptr<PNG_Handle> png_handle = expect_png_handle.value();

        step_and_fill_struct snf;
        int rc = compute_step_and_fill(
            png_handle->width, 
            png_handle->height, 
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

        const int rowbytes = png_get_rowbytes(png_handle->png, png_handle->info);
        std::vector<png_byte> rowbuffer(rowbytes, 0x00);
        int cursor_y = -1;

        for(int output_y = 0; output_y < snf.fill_height; output_y++){
            const int image_y = src_y + round(output_y * snf.step_y);

            
            uint8_t* rowptr = rowbuffer.data();
            while(image_y > cursor_y) {
                if(cursor_y >= ((int)png_handle->height)-1)
                    break;
                
                png_read_rows(png_handle->png, &rowptr, NULL, 1);
                cursor_y++;
            }

            copy_and_resize_from_patch_into_output(
                rowptr,
                /**src_x = */ 0,
                image_y,
                png_handle->width,
                /*height = */ 1,
                png_handle->height,
                (uint8_t*)dst_buffer,
                src_x,
                src_y,
                dst_width,
                dst_height,
                snf.fill_width,
                snf.fill_height,
                snf.step_x,
                snf.step_y,
                /*pixelsize=*/4   //unused
            );

        }

    } catch(...) {
        if(returncode != NULL) *returncode = UNEXPECTED;
        return UNEXPECTED;
    }

    if(returncode != NULL) *returncode = OK;
    return OK;
}





void cb_png_mem_write(png_structp png, png_bytep data, png_size_t length) {
    std::vector<uint8_t> *buffer = (std::vector<uint8_t>*)png_get_io_ptr(png);
    if(!buffer || !data)
        return;
    
    static_assert( sizeof(*buffer->data()) == sizeof(*data) );
    buffer->insert(buffer->end(), data, data + length);
}


struct PNG_WriteHandle {
    png_structp png;
    png_infop   info;
    std::vector<uint8_t> buffer;

    PNG_WriteHandle(png_structp png, png_infop info):
        png(png),
        info(info) {
        // pre-allocating 256KB by default
        this->buffer.reserve(256*1024);
    }

    ~PNG_WriteHandle() {
        png_destroy_write_struct(&this->png, &this->info);
    }


    static std::expected<std::shared_ptr<PNG_WriteHandle>, int> create() {
        png_structp png = 
            png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info = png_create_info_struct(png);
        if (!png || !info) {
            return std::unexpected(PNG_INIT_LIB_FAILED);
        }

        auto png_handle_p = std::make_shared<PNG_WriteHandle>(png, info);
        
        png_set_write_fn(png, &png_handle_p->buffer, cb_png_mem_write, NULL);
        png_set_error_fn(png, NULL, user_error_fn, user_warning_fn);

        return png_handle_p;
    }
};


/** Convert pixels in `data` to binary (0 or 255) */
std::vector<uint8_t> convert_to_binary(const uint8_t* data, size_t size) {
    std::vector<uint8_t> output(size);
    for(int i = 0; i < size; i++)
        output[i] = (data[i] > 0) * 255;
    return output;
}


/** Compress raw single-channel data to a binary png.
    Pixels are converted to boolean where not zero. */
std::expected<Buffer_p, int> png_compress_image(
    const uint8_t* data, 
    int32_t width, 
    int32_t height,
    // 1: binary, 3: rgb
    int channels
) {
    try{
        const auto expect_png_handle = PNG_WriteHandle::create();
        if(!expect_png_handle)
            return std::unexpected(expect_png_handle.error());
        
        const std::shared_ptr<PNG_WriteHandle> png_handle = *expect_png_handle;

        std::vector<uint8_t> databuffer;
        int png_color_type;
        int row_stride;
        if(channels == 1) {
            png_color_type = PNG_COLOR_TYPE_GRAY;
            databuffer = convert_to_binary(data, width * height);
            row_stride = width;
        } else if(channels == 3) {
            png_color_type = PNG_COLOR_TYPE_RGB;
            databuffer.assign(data, data + static_cast<size_t>(width) * height * 3);
            row_stride = width * 3;
        } else 
            return std::unexpected(INVALID_CHANNELS);

        /* IHDR: grayscale, 8-bit depth */
        png_set_IHDR(
            png_handle->png, 
            png_handle->info,
            width, 
            height,
            /* bit depth = */ 8, 
            png_color_type,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_BASE,
            PNG_FILTER_TYPE_BASE
        );
        png_write_info(png_handle->png, png_handle->info);

        for(int y = 0; y < height; ++y)
            png_write_row(
                png_handle->png, 
                (png_const_bytep)databuffer.data() + y*row_stride
            );
        png_write_end(png_handle->png, png_handle->info);

        
        // custom deleter that owns the handle
        auto png_deleter = [handle = std::move(png_handle)](Buffer* b) mutable {
            delete b;
        };
        return Buffer_p(
            new Buffer{ png_handle->buffer.data(), png_handle->buffer.size() }, 
            std::move(png_deleter)
        );
    } catch (...) {
        return std::unexpected(UNEXPECTED);
    }
}


