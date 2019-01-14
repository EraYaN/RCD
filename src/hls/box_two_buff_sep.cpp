#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define WIDTH 1280
#define HEIGHT 720
#define WIDTH_LQ (WIDTH/4) //320
#define HEIGHT_LQ (HEIGHT/4) //180
#define FBUF_SIZE (WIDTH_LQ*HEIGHT_LQ)

#define GR(v) ((v)&0xFF)
#define GG(v) (((v)&0xFF00)>>8)
#define GB(v) (((v)&0xFF0000)>>16)
#define GA(v) (((v)&0xFF000000)>>24)
#define SR(v) ((v)&0xFF)
#define SG(v) (((v)&0xFF)<<8)
#define SB(v) (((v)&0xFF)<<16)
#define SA(v) (((v)&0xFF)<<24)

typedef ap_uint<24> pixel_color;
typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

typedef struct {
	uint32_t color;
	uint16_t x0;
	uint16_t y0;
	uint16_t x1;
	uint16_t y1;
	uint16_t s;
} rectangle;


uint8_t mix_color_R(uint32_t color_top, uint32_t color_bottom, uint8_t alpha){
	return (uint8_t)(((uint16_t)(255-alpha) * GR(color_bottom) + (uint16_t)(alpha) * GR(color_top)) >> 8);
}

uint8_t mix_color_G(uint32_t color_top, uint32_t color_bottom, uint8_t alpha){
	return (uint8_t)(((uint16_t)(255-alpha) * GG(color_bottom) + (uint16_t)(alpha) * GG(color_top)) >> 8);
}

uint8_t mix_color_B(uint32_t color_top, uint32_t color_bottom, uint8_t alpha){
	return (uint8_t)(((uint16_t)(255-alpha) * GB(color_bottom) + (uint16_t)(alpha) * GB(color_top)) >> 8);
}

uint32_t combine_color(uint8_t R, uint8_t G, uint8_t B){
	return SR(R) | SB(B) | SG(G);
}

bool is_on_rect(rectangle rect, uint16_t x, uint16_t y){
	return (x >= rect.x0 - rect.s && x < rect.x0 && y >= rect.y0 - rect.s && y <= rect.y1 + rect.s) ||
			(x > rect.x1 && x <= rect.x1 + rect.s  && y >= rect.y0 - rect.s && y <= rect.y1 + rect.s) ||
			(x >= rect.x0 && x <= rect.x1 && y > rect.y1 && y <= rect.y1 + rect.s) ||
			(x >= rect.x0 && x <= rect.x1 && y >= rect.y0 - rect.s && y < rect.y0);
}

uint32_t composite_pixel(uint32_t color, uint32_t pixel_in){
	uint8_t alpha = GA(color);
	uint8_t R = mix_color_R(color, pixel_in, alpha);
	uint8_t G = mix_color_G(color, pixel_in, alpha);
	uint8_t B = mix_color_B(color, pixel_in, alpha);
	return combine_color(R,G,B);
}

void stream(pixel_stream &src, /*pixel_stream &dst,*/ pixel_stream &dst_hq, uint32_t color_arg, uint16_t x0_arg, uint16_t y0_arg, uint16_t x1_arg, uint16_t y1_arg, uint16_t s_arg, uint8_t idx_arg, uint8_t write_rect_arg)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src

//#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE axis port=&dst_hq
#pragma HLS INTERFACE s_axilite port=color_arg
#pragma HLS INTERFACE s_axilite port=x0_arg
#pragma HLS INTERFACE s_axilite port=y0_arg
#pragma HLS INTERFACE s_axilite port=x1_arg
#pragma HLS INTERFACE s_axilite port=y1_arg
#pragma HLS INTERFACE s_axilite port=s_arg
#pragma HLS INTERFACE s_axilite port=idx_arg
#pragma HLS INTERFACE s_axilite port=write_rect_arg
#pragma HLS PIPELINE II=1

	// Input arguments buffered
	static uint32_t color = 0;
	static uint16_t x0 = 0;
	static uint16_t y0 = 0;
	static uint16_t x1 = 0;
	static uint16_t y1 = 0;
	static uint16_t s = 0;
	static uint8_t idx = 0;
	static uint8_t write_rect = 0;

	color = color_arg;
	x0 = x0_arg;
	y0 = y0_arg;
	x1 = x1_arg;
	y1 = y1_arg;
	s = s_arg;
	idx = idx_arg;
	write_rect = write_rect_arg;

	// Data to be stored across 'function calls'
	static uint16_t x = 0;
	static uint16_t y = 0;

	//static uint32_t l0[WIDTH >> 4];

	//static uint32_t tmp_pixel = 0;

	static rectangle rect1 = {0,0,0,0,0};
	static rectangle rect2 = {0,0,0,0,0};

	//static bool is_second_fb = false;
	//static pixel_color frame_buffer[FBUF_SIZE*2];
	//#pragma HLS DEPENDENCE variable=frame_buffer inter false

	if(write_rect != 0){
		rectangle rect_in = {color, x0, y0, x1, y1, s};
		switch(idx){
		case 0:
			rect1 = rect_in;
			break;
		case 1:
			rect2 = rect_in;
			break;
		}
	}

	/*int current_line = y >> 2;
	int current_pixel = x >> 2;

	bool is_output_pixel_x = ((x & 0x3) == 0x3);
	bool is_output_pixel_y = ((y & 0x3) == 0x3);*/

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
	bool is_on_rect1 = is_on_rect(rect1,x,y);
	bool is_on_rect2 = is_on_rect(rect2,x,y);

	uint32_t pixel;

	if(is_on_rect1 && is_on_rect2){
		pixel = composite_pixel(rect1.color,p_in.data);
	} else if (is_on_rect1){
		pixel = composite_pixel(rect1.color,p_in.data);
	} else if (is_on_rect2){
		pixel = composite_pixel(rect2.color,p_in.data);
	} else {
		pixel = p_in.data;
	}

	p_out.data = pixel;

	////////////////////////////////

	// Write pixel to destination
	dst_hq << p_out;

	/*uint32_t updated_value = tmp_pixel + (SR(GR(pixel) / 4) | SG(GG(pixel) / 4) | SB(GB(pixel) / 4));
	if(is_output_pixel_x && !is_output_pixel_y){
		l0[current_pixel] = updated_value;
		tmp_pixel = l0[current_pixel+1];
	} else if(is_output_pixel_x && is_output_pixel_y){
		l0[current_pixel] = 0;
		tmp_pixel = l0[current_pixel+1];
		int idx = WIDTH_LQ*current_line+current_pixel;
		int offset_0 = is_second_fb?FBUF_SIZE:0;
		int offset_1 = !is_second_fb?FBUF_SIZE:0;
		frame_buffer[idx+offset_0] = updated_value & 0xFFFFFF;
		p_out_resample.data = (uint32_t)frame_buffer[idx+offset_1];
		dst << p_out_resample;
	}*/

	// Increment X and Y counters
	if(p_in.last) {
		x = 0;
		y++;
	} else {
		x++;
	}
}
