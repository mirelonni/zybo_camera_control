#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
extern "C" {
#include <uiotools/uiotools.h>
}
#include <math.h>

#include <iostream>
#include <vector>
#include <numeric>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <typeinfo>
#include <csignal>
#include <string>



#include "PN532_rfid.h"
#include "cardQueue.h"
#include "cards.h"
#include "acl.h"
#include "config.hpp"

double full_map(double input, double in_min, double in_max, double out_min, double out_max) {

	double slope = (double) (out_max - out_min) / (double) (in_max - in_min);
	double output = (double) out_min + slope * (double) (input - in_min);
	return output;
}

double average_not_zero(int a, int b) {

	if (a != 0 && b != 0) {
		return (double) (a + b) / 2;
	} else if (a != 0) {
		return a;
	} else if (b != 0) {
		return b;
	} else {
		return -1;
	}
}

void one_poly(cv::Mat img) {
	int w = img.cols;
	int h = img.rows;

	int x1, x2, y1, y2;
	x1 = (int) w / 2;
	y1 = 0;
	x2 = w;
	y2 = h;

	int lineType = 8;

	cv::Point pts[1][4];
	pts[0][0] = cv::Point(x1, y1);
	pts[0][1] = cv::Point(x1, y2);
	pts[0][2] = cv::Point(x2, y2);
	pts[0][3] = cv::Point(x2, y1);

	const cv::Point* ppt[1] = { pts[0] };
	int npt[] = { 4 };
	cv::fillPoly(img, ppt, npt, 1, cv::Scalar(255, 255, 255), lineType);
}

cv::Mat crop(cv::Mat frame, int x1, int y1, int x2, int y2) {

	cv::Rect roi;
	roi.x = x1;
	roi.y = y1;
	roi.width = x2 - x1;
	roi.height = y2 - y1;

	cv::Mat crop = frame(roi);
	return crop;
}
