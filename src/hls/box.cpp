#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

//#include <inc.h>

#define WIDTH 1280
#define HEIGHT 720

#define GR(v) ((v)&0xFF)
#define GG(v) (((v)&0xFF00)>>8)
#define GB(v) (((v)&0xFF0000)>>16)
#define SR(v) ((v)&0xFF)
#define SG(v) (((v)&0xFF)<<8)
#define SB(v) (((v)&0xFF)<<16)

typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void stream(pixel_stream &src, pixel_stream &dst, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t s, uint32_t color)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE s_axilite port=x0
#pragma HLS INTERFACE s_axilite port=y0
#pragma HLS INTERFACE s_axilite port=x1
#pragma HLS INTERFACE s_axilite port=y1
#pragma HLS INTERFACE s_axilite port=s
#pragma HLS INTERFACE s_axilite port=color
#pragma HLS PIPELINE II=1

	// Data to be stored across 'function calls'
	static uint16_t x = 0;
	static uint16_t y = 0;

	pixel_data p_in;

	// Load input data from source
	src >> p_in;

	// Keep track of X and Y
	if(p_in.user)
		x = y = 0;

	////////////////////////////////

	// Pixel data to be stored across 'function calls'
	static pixel_data p_out = p_in;
	static uint32_t dl = 0;
	static uint32_t dc = 0;

	// Current (incoming) pixel data
	uint32_t pixel = p_in.data;

	if( (x >= x0 - s && x < x0 && y >= y0 - s && y <= y1 + s) ||
		(x > x1 && x <= x1 + s  && y >= y0 - s && y <= y1 + s) ||
		(x >= x0 && x <= x1 && y > y1 && y <= y1 + s) ||
		(x >= x0 && x <= x1 && y >= y0 - s && y < y0)){
		pixel = color;
	}

	if( x == x0 && y == y0)
		pixel = 0x000000FF;

	if( x == x1 && y == y1)
		pixel = 0x0000FF00;

	p_out.data = pixel;

	////////////////////////////////

	// Write pixel to destination
	dst << p_out;

	// Copy previous pixel data to next output pixel
	p_out = p_in;

	// Increment X and Y counters
	if(p_in.last) {
		x = 0;
		y++;
	} else {
		x++;
	}
}
