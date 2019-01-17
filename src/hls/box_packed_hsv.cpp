#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define WIDTH 1280
#define HEIGHT 720
#define WIDTH_LQ (WIDTH / 4)   //320
#define HEIGHT_LQ (HEIGHT / 4) //180
#define FBUF_SIZE (WIDTH_LQ * HEIGHT_LQ)

#define GR(v) ((v)&0xFF)
#define GG(v) (((v)&0xFF00) >> 8)
#define GB(v) (((v)&0xFF0000) >> 16)
#define GA(v) (((v)&0xFF000000) >> 24)
#define SR(v) ((v)&0xFF)
#define SG(v) (((v)&0xFF) << 8)
#define SB(v) (((v)&0xFF) << 16)
#define SA(v) (((v)&0xFF) << 24)

typedef ap_uint<24> pixel_color;
typedef ap_uint<104> rect_vector;
typedef ap_axiu<32, 1, 1, 1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

typedef struct
{
    uint32_t color;
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t s;
} rectangle;

uint32_t ah2argb(ap_uint<16> ah)
{
    uint32_t argb = SA((uint8_t)ah(15,8));
    ap_int<8> h = ah(7,0);
    unsigned char h2, f, p, q, t, h2;
    uint8_t v = 255;
    uint8_t s = 255;

    region = (h * 3) >> 7;
    remainder = (h - (region * 43)) * 6; 

    p = 0;//(v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (h2)
    {
    case 0:
        argb = argb | SR(v) | SG(t) | SB(p);
        break;
    case 1:
        argb = argb | SR(q) | SG(v) | SB(p);
        break;
    case 2:
        argb = argb | SR(p) | SG(v) | SB(t);
        break;
    case 3:
        argb = argb | SR(p) | SG(q) | SB(v);
        break;
    case 4:
        argb = argb | SR(t) | SG(p) | SB(v);
        break;
    default:
        argb = argb | SR(v) | SG(p) | SB(q);
        break;
    }

    return argb;
}

uint8_t mix_color_R(uint32_t color_top, uint32_t color_bottom, uint8_t alpha)
{
    return (uint8_t)(((uint16_t)(255 - alpha) * GR(color_bottom) + (uint16_t)(alpha)*GR(color_top)) >> 8);
}

uint8_t mix_color_G(uint32_t color_top, uint32_t color_bottom, uint8_t alpha)
{
    return (uint8_t)(((uint16_t)(255 - alpha) * GG(color_bottom) + (uint16_t)(alpha)*GG(color_top)) >> 8);
}

uint8_t mix_color_B(uint32_t color_top, uint32_t color_bottom, uint8_t alpha)
{
    return (uint8_t)(((uint16_t)(255 - alpha) * GB(color_bottom) + (uint16_t)(alpha)*GB(color_top)) >> 8);
}

uint32_t combine_color(uint8_t R, uint8_t G, uint8_t B)
{
    return SR(R) | SB(B) | SG(G);
}

bool is_on_rect(rectangle rect, uint16_t x, uint16_t y)
{
    return (x >= rect.x0 - rect.s && x < rect.x0 && y >= rect.y0 - rect.s && y <= rect.y1 + rect.s) ||
           (x > rect.x1 && x <= rect.x1 + rect.s && y >= rect.y0 - rect.s && y <= rect.y1 + rect.s) ||
           (x >= rect.x0 && x <= rect.x1 && y > rect.y1 && y <= rect.y1 + rect.s) ||
           (x >= rect.x0 && x <= rect.x1 && y >= rect.y0 - rect.s && y < rect.y0);
}

uint32_t composite_pixel(uint32_t color, uint32_t pixel_in)
{
    uint8_t alpha = GA(color);
    uint8_t R = mix_color_R(color, pixel_in, alpha);
    uint8_t G = mix_color_G(color, pixel_in, alpha);
    uint8_t B = mix_color_B(color, pixel_in, alpha);
    return combine_color(R, G, B);
}

void set_rect_values(rectangle& rect, rect_vector& rect_in){
    rect1.color = ah2argb(rect_in(95,80));
    rect1.x0 = rect_in(79,64);
    rect1.y0 = rect_in(63,48);
    rect1.x1 = rect_in(47,32);
    rect1.y1 = rect_in(31,16);
    rect1.s = rect_in(15,0);
}

ap_int<96> rect_in;

void stream(pixel_stream &src, pixel_stream &dst, pixel_stream &dst_hq, rect_vector rect_in)
{
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INTERFACE axis port = &src

#pragma HLS INTERFACE axis port = &dst
#pragma HLS INTERFACE axis port = &dst_hq
#pragma HLS INTERFACE s_axilite port = rect_in
#pragma HLS PIPELINE II = 1

    // Data to be stored across 'function calls'
    static uint16_t x = 0;
    static uint16_t y = 0;

    ap_uint<8> idx = rect_in(103,96);

    //static uint32_t l0[WIDTH >> 4];

    //static uint32_t tmp_pixel = 0;

    static rectangle rect1 = {0, 0, 0, 0, 0};
    static rectangle rect2 = {0, 0, 0, 0, 0};
    static rectangle rect3 = {0, 0, 0, 0, 0};
    static rectangle rect4 = {0, 0, 0, 0, 0};
    static rectangle rect5 = {0, 0, 0, 0, 0};
    static rectangle rect6 = {0, 0, 0, 0, 0};
    static rectangle rect7 = {0, 0, 0, 0, 0};
    static rectangle rect8 = {0, 0, 0, 0, 0};

    //static bool is_second_fb = false;
    //static pixel_color frame_buffer[FBUF_SIZE*2];
    //#pragma HLS DEPENDENCE variable=frame_buffer inter false

    if (write_rect != 0)
    {
        switch (idx)
        {
        case 0:
            set_rect_values(rect1,rect_in);
            break;
        case 1:
            set_rect_values(rect2,rect_in);
            break;
        case 2:
            set_rect_values(rect3,rect_in);
            break;
        case 3:
            set_rect_values(rect4,rect_in);
            break;
        case 4:
            set_rect_values(rect5,rect_in);
            break;
        case 5:
            set_rect_values(rect6,rect_in);
            break;
        case 6:
            set_rect_values(rect7,rect_in);
            break;
        case 7:
            set_rect_values(rect8,rect_in);
            break;
        }
    }

    int current_line = y >> 2;
    int current_pixel = x >> 2;

    bool is_output_pixel_x = ((x & 0x3) == 0x3);
    bool is_output_pixel_y = ((y & 0x3) == 0x3);

    pixel_data p_in;

    // Load input data from source
    src >> p_in;

    // Keep track of X and Y
    if (p_in.user)
        x = y = 0;

    ////////////////////////////////

    // Pixel data to be passed
    pixel_data p_out = p_in;
    pixel_data p_out_resample = p_in;

    // Current (incoming) pixel data
    bool is_on_rect1 = is_on_rect(rect1, x, y);
    bool is_on_rect2 = is_on_rect(rect2, x, y);
    bool is_on_rect3 = is_on_rect(rect3, x, y);
    bool is_on_rect4 = is_on_rect(rect4, x, y);
    bool is_on_rect5 = is_on_rect(rect5, x, y);
    bool is_on_rect6 = is_on_rect(rect6, x, y);
    bool is_on_rect7 = is_on_rect(rect7, x, y);
    bool is_on_rect8 = is_on_rect(rect8, x, y);
    uint32_t pixel;

    if (is_on_rect1)
    {
        pixel = composite_pixel(rect1.color, p_in.data);
    }
    else if (is_on_rect2)
    {
        pixel = composite_pixel(rect2.color, p_in.data);
    }
    else if (is_on_rect3)
    {
        pixel = composite_pixel(rect3.color, p_in.data);
    }
    else if (is_on_rect4)
    {
        pixel = composite_pixel(rect4.color, p_in.data);
    }
    else if (is_on_rect5)
    {
        pixel = composite_pixel(rect5.color, p_in.data);
    }
    else if (is_on_rect6)
    {
        pixel = composite_pixel(rect6.color, p_in.data);
    }
    else if (is_on_rect7)
    {
        pixel = composite_pixel(rect7.color, p_in.data);
    }
    else if (is_on_rect8)
    {
        pixel = composite_pixel(rect8.color, p_in.data);
    }
    else
    {
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
    if (p_in.last)
    {
        x = 0;
        y++;
    }
    else
    {
        x++;
    }
}
