#pragma once

// TODO: convert util.h to c++


#include <cstdio>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>

#include <unsupported/Eigen/CXX11/Tensor>



struct Buffer {
    uint8_t* data;
    uint64_t size;
};

typedef std::shared_ptr<Buffer> Buffer_p;



struct ImageSize {
    uint32_t width;
    uint32_t height;
};

struct BoxXYWH {
    int32_t  x;
    int32_t  y;
    uint32_t w;
    uint32_t h;
};

/** shape: [height, width, 4] */
typedef Eigen::Tensor<uint8_t, 3, Eigen::RowMajor> EigenRGBAMap;

typedef Eigen::TensorSlicingOp<
    const Eigen::array<Eigen::Index, 3>, 
    const Eigen::array<Eigen::Index, 3>, 
    EigenRGBAMap 
> EigenRGBASlice;

struct EigenRGBACrop {
    EigenRGBASlice slice;
    BoxXYWH coordinates;
};




const std::string
    INVALID_SRC_BOX = "NN::push::Invalid src_box";

// std::expected with a constant string error as explanation
template<typename T>
using expected_s = std::expected<T, std::string_view>;




struct NearestNeighborStreamingInterpolator {
    EigenRGBAMap    output_buffer;
    const BoxXYWH   crop_box;
    const ImageSize dst_size;
    const bool      full;


    /** Factory function. Crop input data at `crop_box` and resize, so that the 
        final result has size `dst_size`. If not `full`, will not store the full
        result, will only return a crop of the transformed data at a time.
        Returns an error on bad allocation. */
    static std::expected<NearestNeighborStreamingInterpolator, int> create(
        const BoxXYWH&   crop_box, 
        const ImageSize& dst_size, 
        bool full, 
        int  n_channels
    );
    
    /** Feed new data, which has the coordinates `src_box` within the original 
        image. Returns the transformed data (i.e. view of the output buffer).*/
    const expected_s<EigenRGBACrop>
        push_image_rows(const EigenRGBAMap& src, const BoxXYWH& src_box);


    private:
    NearestNeighborStreamingInterpolator(
        const BoxXYWH& crop_box,
        const ImageSize& dst_size,
        bool  full,
        int   n_channels
    );

};





struct FileHandle {
    // factory function
    static std::optional<std::shared_ptr<FileHandle>> open(const char* path);
    
    // callback to pass to image read functions
    static int read_callback(const void*, void*, uint64_t, uint64_t);

    // file size, also needed by image read functions
    size_t size;

    ~FileHandle();

    private:
    FileHandle(FILE*, size_t);
    FILE* f;
};




// TODO: combine with the above
typedef Eigen::TensorSlicingOp<
    const Eigen::array<Eigen::Index, 3>, 
    const Eigen::array<Eigen::Index, 3>, 
    const EigenRGBAMap 
> EigenRGBAConstSlice;



const std::string
    ROW_SLICE_INVALID_INDEX = "row_slice::invalid index";

/** Extract a row from an image tensor. Shape [1,W,C] */
expected_s<EigenRGBAConstSlice> row_slice(const EigenRGBAMap& t, Eigen::Index i);

