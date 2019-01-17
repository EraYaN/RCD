/* Streamulator test platform
 * Original by Michiel van der Vlag, adapted by Matti Dreef, edited by Erwin de Haan
 */

#include "inc.h"


int main ()
{
	// Streams and data
	ap_uint<32>* pixeldata = (ap_uint<32> *)malloc(sizeof(ap_uint<32>)*HEIGHT*WIDTH);
	//ap_uint<32>* pixeldata_lq = (ap_uint<32> *)malloc(sizeof(ap_uint<32>)*(HEIGHT/4)*(WIDTH/4));
	hls::stream<pixel_data> inputStream;
	//hls::stream<pixel_data> outputStream;
	hls::stream<pixel_data> outputStream_hq;
	pixel_data streamIn;
	pixel_data streamOut;


	// Read input image
	//IplImage* sourceImg = cvLoadImage(INPUT_IMG,1);
	cv::Mat sourceImg = cv::imread(INPUT_IMG);

	if (!sourceImg) //check whether the image is loaded or not
	{
	  std::cout << "Error: Image cannot be loaded..!!" << std::endl;
	  //system("pause"); //wait for a key press
	  return -1;
	}
	// A necessary conversion to obtain the right format...
	cvCvtColor(sourceImg, sourceImg, CV_BGR2RGBA);

	rectangle rect;
	rect.x0 = 100;
	rect.y0 = 100;
	rect.x1 = 200;
	rect.y1 = 200;
	rect.color = 0x660000FF;
	// Write input data
	for (int rows=0; rows < HEIGHT; rows++)
		for (int cols=0; cols < WIDTH; cols++)
		{
			streamIn.data = sourceImg.at<int>(rows,cols);
			streamIn.user = (rows==0 && cols==0) ? 1 : 0;
			streamIn.last = (cols==WIDTH-1) ? 1 : 0;

			inputStream << streamIn;
		}

	// Call stream processing function
	while (!inputStream.empty())
		stream(inputStream, outputStream_hq, rect, 1, 1); // Add extra arguments here


	// Read output data
	/*for (int rows=0; rows < HEIGHT/4; rows++)
		for (int cols=0; cols < WIDTH/4; cols++)
		{
			outputStream.read(streamOut);
			pixeldata_lq[rows][cols] = streamOut.data;
		}*/

	for (int rows=0; rows < HEIGHT; rows++)
			for (int cols=0; cols < WIDTH; cols++)
			{
				outputStream_hq.read(streamOut);
				pixeldata[rows][cols] = streamOut.data;
			}


	// Save image by converting data array to matrix
	// Depth or precision: CV_8UC4: 8 bit unsigned chars x 4 channels = 32 bit per pixel;
	cv::Mat imgCvOut(cv::Size(WIDTH, HEIGHT), CV_8UC4, pixeldata);
	//cv::Mat imgCvOut_lq(cv::Size(WIDTH/4, HEIGHT/4), CV_8UC4, pixeldata_lq);
	cv::imwrite(OUTPUT_IMG_HQ, imgCvOut);
	//cv::imwrite(OUTPUT_IMG_LQ, imgCvOut_lq);

	free(pixeldata);
	//free(pixeldata_lq);
	return 0;
}

