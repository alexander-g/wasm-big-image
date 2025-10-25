#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "./util.h"






int compute_step_and_fill(
    int image_width,
    int image_height,
    int src_x,
    int src_y,
    int src_width,
    int src_height,
    int dst_width,
    int dst_height,
    step_and_fill_struct* out
) {
    // actual number of pixels to read
    int read_height = src_height;
    int read_width  = src_width;
    // clamp to image bounds
    if (src_x + src_width > image_width)
        read_width = image_width - src_x;
    if (src_y + src_height > image_height)
        read_height = image_height - src_y;
    if (read_width == 0 || read_height == 0)
        return INVALID_SIZES;
    
    const double step_y = (double)src_height / dst_height;
    const double step_x = (double)src_width / dst_width;
    // maximum pixel within the output buffer
    const int fill_height = min( round(read_height / step_y), dst_height);
    const int fill_width  = min( round(read_width / step_x), dst_width );
    
    *out = (step_and_fill_struct) {
        step_x,
        step_y,
        fill_width,
        fill_height,
        read_width,
        read_height
    };
    return OK;
}



void copy_and_resize_from_patch_into_output(
    const uint8_t* src_buffer,
    // position of src_buffer within the full input image
    int      src_x,
    int      src_y,
    int      src_width,
    int      src_height,
    int      image_height,

    uint8_t* dst_buffer,
    // starting coordinates of dst_buffer within the full input image
    int      dst_x,
    int      dst_y,
    // size of dst_buffer
    int      dst_width,
    int      dst_height,
    // how many pixels in dst_buffer to actually fill
    int      fill_width,
    int      fill_height,
    double   step_x,
    double   step_y,
    // bytes / px
    size_t   pixel_size
) {
    // libtiff antics, rows start at the bottom, but not in the last strip
    const int src_buffer_y_start = src_height - 1;
    //const int src_buffer_y_start = 
    //    min(src_height - 1, src_height - 1 - (image_height - src_y));

    for(int output_y = 0; output_y < fill_height; output_y++){
        const int image_y = dst_y + round(output_y * step_y);
        if(image_y < src_y)
            continue;
        if(image_y >= src_y + src_height)
            break;

        
        // discard offset rows
        int src_buffer_y = src_buffer_y_start - (image_y - src_y);
        // should not do anything but just in case
        src_buffer_y = max(0, src_buffer_y);

        const int src_base = src_buffer_y * src_width;
        const int dst_base = output_y * dst_width;
        
        for(int output_x = 0; output_x < fill_width; output_x++){
            //const int image_x = dst_x + round(output_x * step_x);
            const int image_x = dst_x + floor(output_x * step_x);

            if(image_x < src_x)
                continue;
            if(image_x >= src_x + src_width)
                break;

            const int src = src_base + (image_x - src_x);
            const int dst = dst_base + output_x;

            ((uint32_t*)dst_buffer)[dst] = ((uint32_t*)src_buffer)[src];
        }
    }

}
