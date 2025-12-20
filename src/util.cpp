

#include "./util.h"
// macro collision with util.h
#undef max

#include "./util.hpp"




NearestNeighborStreamingInterpolator::NearestNeighborStreamingInterpolator(
    const BoxXYWH& crop_box,
    const ImageSize& dst_size,
    bool  full,
    int   n_channels
) :
    output_buffer(
        /*height*/   full? (int)(dst_size.height) : 0,
        /*width*/          (int)(dst_size.width), 
        /*channels*/        n_channels
    ),
    crop_box(crop_box),
    dst_size(dst_size),
    full(full)
{
    output_buffer.setZero();
}

std::expected<NearestNeighborStreamingInterpolator, int> 
NearestNeighborStreamingInterpolator::create(
    const BoxXYWH& crop_box,
    const ImageSize& dst_size,
    bool full,
    int  n_channels
) {
    if (dst_size.width == 0 || dst_size.height == 0)
        return std::unexpected(INVALID_SIZES);
    if (crop_box.w == 0 || crop_box.h == 0)
        return std::unexpected<int>(INVALID_SIZES);
    if (crop_box.x < 0 || crop_box.y < 0) 
        return std::unexpected<int>(NEGATIVE_OFFSETS);
    

    try {
        return NearestNeighborStreamingInterpolator(
            crop_box, 
            dst_size, 
            full, 
            n_channels
        );
    } catch (const std::bad_alloc&) {
        return std::unexpected<int>(MALLOC_FAILED);
    } catch (...) {
        return std::unexpected<int>(UNEXPECTED);
    }
}



const expected_s<EigenRGBACrop>
NearestNeighborStreamingInterpolator::push_image_rows(
    const EigenRGBAMap& src, 
    const BoxXYWH& src_box
) {
    //printf("TODO: src_box could be just an offset, h+w already in src.dimensions()\n");
    
    const int n_channels = src.dimension(2);
    if(n_channels != this->output_buffer.dimension(2))
        return std::unexpected(INVALID_SRC_BOX.c_str());


    //printf("TODO: compute scale in the constructor\n"); // but not if dims change?
    const double scale_x = (double)(this->dst_size.width) / (double)(crop_box.w);
    const double scale_y = (double)(this->dst_size.height) / (double)(crop_box.h);

    const int32_t src_y0 = src_box.y;
    const int32_t src_y1 = src_box.y + (int32_t)(src_box.h); // exclusive

    // corresponding dst y range (clamped)
    int32_t dst_y0 = (int32_t)(std::floor((src_y0 - crop_box.y) * scale_y));
    int32_t dst_y1 = (int32_t)(std::floor((src_y1 - crop_box.y) * scale_y));
    if (dst_y0 < 0) 
        dst_y0 = 0;
    if (dst_y1 > (int32_t)(dst_size.height)) 
        dst_y1 = (int32_t)(dst_size.height);
    if (dst_y1 < 0)
        dst_y1 = 0;


    const int32_t src_x0 = src_box.x;
    const int32_t src_x1 = src_box.x + (int32_t)(src_box.w); // exclusive
    
    int32_t dst_x0 = (int32_t)(std::floor((src_x0 - crop_box.x) * scale_x));
    int32_t dst_x1 = (int32_t)(std::floor((src_x1 - crop_box.x) * scale_x));
    if (dst_x0 < 0) 
        dst_x0 = 0;
    if (dst_x1 > (int32_t)(dst_size.width)) 
        dst_x1 = (int32_t)(dst_size.width);
    if (dst_x1 < 0)
        dst_x1 = 0;

    
    const int32_t dst_h  = dst_y1 - dst_y0;
    const int32_t dst_w  = dst_x1 - dst_x0;
    int32_t dst_offset_y = 0;
    int32_t dst_offset_x = 0;
    if(!this->full){
        dst_offset_y = dst_y0;
        dst_offset_x = dst_x0;
        dst_y0 = 0;
        dst_x0 = 0;
        dst_y1 = dst_y1 - dst_offset_y;
        dst_x1 = dst_x1 - dst_offset_x;

        if(this->output_buffer.dimension(0) < dst_h
        || this->output_buffer.dimension(1) < dst_w)
            this->output_buffer = EigenRGBAMap(dst_h, dst_w, n_channels);
        this->output_buffer.setConstant(0x77);
    }

    
    for (int32_t dst_y = dst_y0; dst_y < dst_y1; dst_y++) {
        // nearest source y in crop coordinate space
        const double src_y_f = 
            ( ((double)(dst_y) + 0.5 + dst_offset_y) / scale_y ) 
            +  (double)(crop_box.y);
        const int32_t nearest_src_y = (int32_t)(std::floor(src_y_f));
        // NOTE: no check, handled by std::max below
        // if (nearest_src_y < src_y0)
        //     continue;
        if(nearest_src_y >= src_y1)
            break;

        // index inside src chunk
        const int32_t src_i = std::max(nearest_src_y - src_y0, 0);

        for (int32_t dst_x = dst_x0; dst_x < dst_x1; dst_x++) {
            const double src_x_f = 
                ( ((double)(dst_x) + 0.5 + dst_offset_x) / scale_x ) 
                +  (double)(crop_box.x);
            const int32_t nearest_src_x = (int32_t)(std::floor(src_x_f));
            // NOTE: no check, handled by std::max below
            // if (nearest_src_x <  src_box.x )
            //     continue;
            if(nearest_src_x >= src_box.x + (int32_t)(src_box.w))
                break;
            
            // index inside src chunk
            const int32_t src_j = std::max(nearest_src_x - src_box.x, 0);

            const Eigen::array<Eigen::Index, 3> offsets_dst = {dst_y, dst_x, 0};
            const Eigen::array<Eigen::Index, 3> offsets_src = {src_i, src_j, 0};
            const Eigen::array<Eigen::Index, 3> extents = {1, 1, n_channels};

            this->output_buffer.slice(offsets_dst, extents) = 
                src.slice(offsets_src, extents);
        }
    }


    //printf("TODO: check slicing dimensions:  %i %i - %i %i \n", dst_y0, dst_y1,   dst_x0, dst_x1);
    const Eigen::array<Eigen::Index, 3> offsets = {dst_y0, dst_x0, 0};
    const Eigen::array<Eigen::Index, 3> extents = {dst_h,  dst_w,  n_channels};
    return EigenRGBACrop{
        .slice = this->output_buffer.slice(offsets, extents),
        .coordinates = {
            .x = dst_x0 + dst_offset_x, 
            .y = dst_y0 + dst_offset_y, 
            .w = (uint32_t)dst_w, 
            .h = (uint32_t)dst_h
        }
    };
}






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





expected_s<EigenRGBAConstSlice> row_slice(const EigenRGBAMap& t, Eigen::Index i) {
    const Eigen::Index D0 = t.dimension(0);
    const Eigen::Index D1 = t.dimension(1);
    const Eigen::Index D2 = t.dimension(2);

    // clamp or assert index in range
    if(i < 0 || i >= D0)
        return std::unexpected(ROW_SLICE_INVALID_INDEX.c_str());

    const Eigen::array<Eigen::Index, 3> offsets = { i, 0, 0 };
    const Eigen::array<Eigen::Index, 3> extents = { 1, D1, D2 };
    return t.slice(offsets, extents);
}


