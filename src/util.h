


enum Error { 
    OK = 0,
    
    BUFFER_TOO_SMALL = -1,
    INVALID_SIZES    = -2, 
    NEGATIVE_OFFSETS = -3, 
    OFFSETS_OUT_OF_BOUNDS = -4,

    MALLOC_FAILED   = -997,
    UNEXPECTED      = -998,
    NOT_IMPLEMENTED = -999,
};



#define min(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#define max(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })




typedef struct  {
    // step sizes for interpolation
    double step_x;
    double step_y;

    // how many pixels in the output buffer to fill
    int fill_width;
    int fill_height;

    // how many pixels in the source buffer to read
    int read_width;
    int read_height;
} step_and_fill_struct;


/** Compute internal parameters */
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
);



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
);