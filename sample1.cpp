#include <string.h> // memcpy
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "JPGPusher.h"

cv::Mat cv_frame;

uchar* frame_provider_function(unsigned *size)
{
	std::vector<unsigned char> buffer;
	std::vector<int> jpgparams;
	unsigned char* ret;
	jpgparams.push_back(CV_IMWRITE_JPEG_QUALITY);
	jpgparams.push_back(50);//sets jpg quality.
	cv::imencode(".jpg", cv_frame, buffer, jpgparams);
	size[0] = buffer.size();
	ret = (unsigned char*) malloc(size[0]);
	memcpy(ret, buffer.data(), size[0]);
	buffer.clear();
	jpgparams.clear();
	cv::waitKey(67);//FPS 15
	return ret; //JPGPusher will free after.
}

int main(void)
{
	JPGPusher server_jpg(frame_provider_function);
	bool run = true;
	char c;
	cv::VideoCapture cap(0);

	server_jpg.start(12340);

	while (run)
	{
		cap >> cv_frame;
		cv::imshow("cv",cv_frame);
		c = cv::waitKey(1);
		run = c != 27;//Stop if ESC pressed
	}
	server_jpg.finish();
	return 0;
}
