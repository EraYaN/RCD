/* Streamulator test platform
 * Original by Michiel van der Vlag, adapted by Matti Dreef
 */

#ifndef INC_H
#define INC_H


#include <stdint.h>
#include <hls_stream.h>
#include <hls_video.h>
#include <hls_opencv.h>
#include <ap_axi_sdata.h>


// Image dimensions
#define WIDTH 1280
#define HEIGHT 720

typedef struct
{
    bool is_on;
    uint32_t color;
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t s;
} rectangle;

// Pixel and stream types

typedef ap_uint<112> rect_vector;
typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;


// Stream processing function
void stream(pixel_stream &src, /*pixel_stream &dst,*/ pixel_stream &dst_hq, uint16_t idx, rect_vector rect_in);



// Streamulator image paths
#define INPUT_IMG  "parrot.jpg"
#define OUTPUT_IMG_HQ "C:\\Users\\Erwin\\Documents\\PYNQ-master\\boards\\hls\\Streamulator\\image_hq.bmp"
//#define OUTPUT_IMG_LQ "C:\\Users\\Erwin\\Documents\\PYNQ-master\\boards\\hls\\Streamulator\\image_lq.bmp"


#endif // INC_H
