#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

//#include <inc.h>

#define WIDTH 1280
#define HEIGHT 720

#define GR(v) ((v)&0xFF)
#define GG(v) (((v)&0xFF00)>>8)
#define GB(v) (((v)&0xFF0000)>>16)
#define GA(v) (((v)&0xFF000000)>>24)
#define SR(v) ((v)&0xFF)
#define SG(v) (((v)&0xFF)<<8)
#define SB(v) (((v)&0xFF)<<16)
#define SA(v) (((v)&0xFF)<<24)

typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void stream(pixel_stream &src, pixel_stream &dst, pixel_stream &dst_hq, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t s, uint32_t color)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE axis port=&dst_hq
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

	static uint32_t l0[WIDTH >> 4];

	static uint32_t tmp_pixel = 0;


	int current_line = y >> 2;
	int current_pixel = x >> 2;

	bool is_output_pixel_x = ((x & 0x3) == 0x3);
	bool is_output_pixel_y = ((y & 0x3) == 0x3);

	pixel_data p_in;

	// Load input data from source
	src >> p_in;

	// Keep track of X and Y
	if(p_in.user)
		x = y = 0;

	////////////////////////////////

	// Pixel data to be passed
	pixel_data p_out = p_in;
	pixel_data p_out_resample = p_in;

	// Current (incoming) pixel data
	uint32_t pixel = p_in.data;

	if( (x >= x0 - s && x < x0 && y >= y0 - s && y <= y1 + s) ||
		(x > x1 && x <= x1 + s  && y >= y0 - s && y <= y1 + s) ||
		(x >= x0 && x <= x1 && y > y1 && y <= y1 + s) ||
		(x >= x0 && x <= x1 && y >= y0 - s && y < y0)){
		uint8_t alpha = GA(color);
		uint16_t R = ((uint16_t)(255-alpha)*GR(pixel) + (uint16_t)(alpha) * GR(color)) >> 8;
		uint16_t G = ((uint16_t)(255-alpha)*GG(pixel) + (uint16_t)(alpha) * GG(color)) >> 8;
		uint16_t B = ((uint16_t)(255-alpha)*GB(pixel) + (uint16_t)(alpha) * GB(color)) >> 8;
		pixel = SR(R) | SB(B) | SG(G);
	}

	p_out.data = pixel;

	////////////////////////////////

	// Write pixel to destination
	dst_hq << p_out;

	uint32_t updated_value = tmp_pixel + (SR(GR(pixel) / 4) | SG(GG(pixel) / 4) | SB(GB(pixel) / 4));
	if(is_output_pixel_x && !is_output_pixel_y){
		l0[current_pixel] = updated_value;
		tmp_pixel = l0[current_pixel+1];
		p_out_resample.data = 0;
		dst << p_out_resample;
	} else if(is_output_pixel_x && is_output_pixel_y){
		p_out_resample.data = updated_value;
		l0[current_pixel] = 0;
		tmp_pixel = l0[current_pixel+1];
		dst << p_out_resample;
	}

	// Increment X and Y counters
	if(p_in.last) {
		x = 0;
		y++;
	} else {
		x++;
	}
}
