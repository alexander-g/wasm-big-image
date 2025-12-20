#include "../src/util.hpp"

#include <cstdio>
#include <iostream>


int test_nn_interpolation_full() {

    const auto expect_nn = NearestNeighborStreamingInterpolator::create(
        /*crop_box=*/ {.x=20, .y=10, .w=10, .h=30},
        /*dst_size=*/ {.width=20, .height=60}, 
        /*full=*/ true
    );
    assert(expect_nn.has_value());
    auto nn = expect_nn.value();


    const EigenRGBAMap not_rgba(5, 100, 2);
    const auto expect_error0 = nn.push_image_rows(not_rgba, {0,0, 100, 5});
    assert(!expect_error0.has_value());
    assert(expect_error0.error() == INVALID_SRC_BOX);


    EigenRGBAMap rgba(5, 100, 4);
    rgba.setZero();
    Eigen::array<Eigen::Index, 3> offsets = {0, 20, 0}; // y,x,c
    Eigen::array<Eigen::Index, 3> extents = {5, 10, 4}; // w,h,c
    rgba.slice(offsets, extents) = EigenRGBAMap(extents).setConstant(255);

    const auto expect_slice0 = nn.push_image_rows(rgba, {.x=0, .y=0, .w=100, .h=5});
    assert(expect_slice0.has_value());
    const EigenRGBAMap slice0 = expect_slice0->slice.eval();
    assert( slice0.dimension(0) == 0 );
    assert( slice0.dimension(1) == 20 );  // == dst_size.width
    assert( slice0.dimension(2) == 4 );


    const auto expect_slice1 = nn.push_image_rows(rgba, {.x=0, .y=20, .w=100, .h=5});
    assert(expect_slice1.has_value());
    const EigenRGBAMap slice1 = expect_slice1->slice.eval();
    assert( slice1.dimension(0) == 10 );
    assert( slice1.dimension(1) == 20 );  // == dst_size.width
    assert( slice1.dimension(2) == 4 );
    
    const Eigen::Tensor<double, 0, Eigen::RowMajor> sum  
        = slice1.cast<double>().sum();
    const double mean = sum(0) / slice1.size();
    assert(mean == 255);


    const auto expect_slice2 = nn.push_image_rows(rgba, {.x=26, .y=0, .w=100, .h=5});
    assert(expect_slice2.has_value());
    const EigenRGBAMap slice2 = expect_slice2->slice.eval();
    assert( slice2.dimension(0) == 0 );
    assert( slice2.dimension(1) == 8 );  // ((20+10) - 26) * 2
    assert( slice2.dimension(2) == 4 );
    

    return 0;
}


int test_nn_interpolation_streaming_only() {
    const auto expect_nn = NearestNeighborStreamingInterpolator::create(
        /*crop_box=*/ {.x=20, .y=10, .w=10, .h=30},
        /*dst_size=*/ {.width=20, .height=60}, 
        /*full=    */ false
    );
    assert(expect_nn.has_value());
    auto nn = expect_nn.value();

    assert(nn.output_buffer.size() == 0);


    EigenRGBAMap rgba(5, 100, 4);
    rgba.setZero();
    Eigen::array<Eigen::Index, 3> offsets = {0, 20, 0}; // y,x,c
    Eigen::array<Eigen::Index, 3> extents = {5, 10, 4}; // w,h,c
    rgba.slice(offsets, extents) = EigenRGBAMap(extents).setConstant(255);

    const auto expect_slice0 = nn.push_image_rows(rgba, {.x=0, .y=0, .w=100, .h=5});
    assert(expect_slice0.has_value());
    const EigenRGBAMap slice0 = expect_slice0->slice.eval();
    assert( slice0.dimension(0) == 0 );
    assert( slice0.dimension(1) == 20 );  // == dst_size.width
    assert( slice0.dimension(2) == 4 );


    const auto expect_slice1 = nn.push_image_rows(rgba, {.x=0, .y=20, .w=100, .h=5});
    assert(expect_slice1.has_value());
    const EigenRGBAMap slice1 = expect_slice1->slice.eval();
    assert( slice1.dimension(0) == 10 );
    assert( slice1.dimension(1) == 20 );  // == dst_size.width
    assert( slice1.dimension(2) == 4 );
    assert(nn.output_buffer.size() == 10*20*4);
    
    const Eigen::Tensor<double, 0, Eigen::RowMajor> sum  
        = slice1.cast<double>().sum();
    const double mean = sum(0) / slice1.size();
    assert(mean == 255);


    return 0;
}


// bug
int test_nn_interpolation_single_row_upscaling() {
    const uint32_t h = 100;
    const uint32_t w = 50;
    EigenRGBAMap rgba(h, w, 4);
    //rgba.setConstant(111);
    rgba.setRandom();

    const uint32_t dst_H = 10771;
    const uint32_t dst_W = 10002;
    const auto expect_nn = NearestNeighborStreamingInterpolator::create(
        /*crop_box=*/ {.x=0, .y=0, .w=w, .h=h},
        /*dst_size=*/ {.width=dst_W, .height=dst_H}, 
        /*full=    */ false
    );
    assert(expect_nn.has_value());
    auto nn = expect_nn.value();

    int last_y = -1;
    for(int y = 0; y < h; y++){
        const auto expect_row = row_slice(rgba, y);
        assert(expect_row.has_value());
        const auto row = expect_row.value();

        const auto expect_slice0 = nn.push_image_rows(row, {.x=0, .y=y, .w=w, .h=1});
        assert(expect_slice0.has_value());
        const EigenRGBAMap slice0 = expect_slice0->slice.eval();
        const BoxXYWH coordinates = expect_slice0->coordinates;
        
        assert( slice0.dimension(0) == coordinates.h );
        assert( slice0.dimension(1) == coordinates.w );
        assert( slice0.dimension(2) == 4 );

        assert(coordinates.x == 0);
        assert(coordinates.w == dst_W);
        assert(coordinates.y == last_y+1);
        last_y += coordinates.h;

        assert( slice0(0, 0, 0) == rgba(y, 0, 0) );
        assert( slice0(0, 0, 1) == rgba(y, 0, 1) );
        assert( slice0(0, 0, 2) == rgba(y, 0, 2) );
        assert( slice0(0, 0, 3) == rgba(y, 0, 3) );

        assert( slice0(slice0.dimension(0)-1, 0, 0) == rgba(y, 0, 0) );
        assert( slice0(slice0.dimension(0)-1, 0, 1) == rgba(y, 0, 1) );
        assert( slice0(slice0.dimension(0)-1, 0, 2) == rgba(y, 0, 2) );
        assert( slice0(slice0.dimension(0)-1, 0, 3) == rgba(y, 0, 3) );
    }
    assert(last_y+1 == dst_H);

    return 0;
}
