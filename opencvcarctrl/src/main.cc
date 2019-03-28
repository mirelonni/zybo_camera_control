#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/fcntl.h>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <numeric>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define CHARVIDEO_IOC_MAGIC  '8'

#define CHARVIDEO_IOCHALT    _IO(CHARVIDEO_IOC_MAGIC, 0)
#define CHARVIDEO_IOCSTART    _IO(CHARVIDEO_IOC_MAGIC, 1)
#define CHARVIDEO_IOCSTATUS    _IO(CHARVIDEO_IOC_MAGIC, 2)

#define CHARVIDEO_IOCQHEIGHT _IOR(CHARVIDEO_IOC_MAGIC,  3, int)
#define CHARVIDEO_IOCQWIDTH _IOR(CHARVIDEO_IOC_MAGIC,  4, int)
#define CHARVIDEO_IOCQPIXELLEN _IOR(CHARVIDEO_IOC_MAGIC,  5, int)
#define CHARVIDEO_IOCQBUFSIZE _IOR(CHARVIDEO_IOC_MAGIC,  6, int)

#define SERVO_LEFT 220
#define SERVO_RIGHT 380
#define SERVO_CENTER (SERVO_LEFT + (SERVO_RIGHT-SERVO_LEFT)/2)

#define RIGHT_Y 560
#define LEFT_Y 560

#define LINE_DISTANCE 70

#define LEFT_MEAN 96
#define LEFT_X_1 LEFT_MEAN - LINE_DISTANCE
#define LEFT_X_2 LEFT_MEAN + LINE_DISTANCE

#define RIGHT_MEAN 1106
#define RIGHT_X_1 RIGHT_MEAN - LINE_DISTANCE
#define RIGHT_X_2 RIGHT_MEAN + LINE_DISTANCE

using namespace cv;
using namespace std;

int servo_comand_map(double m_r, double m_l, double in_min, double in_max) {

	double sum = m_l + m_r;
	double input;

	if (fabs(m_r) < fabs(m_l)) {
		input = (1) * fabs(sum);
	} else {
		input = (-1) * fabs(sum);
	}

	double slope = (double) (SERVO_RIGHT - SERVO_LEFT) / (double) (in_max - in_min);
	double output = (double) SERVO_LEFT + slope * (double) (input - in_min);
	return (int) output;

}

int map_servo (double input, double in_min, double in_max) {

	double slope = (double) (SERVO_RIGHT - SERVO_LEFT) / (double) (in_max - in_min);
	double output = (double) SERVO_LEFT + slope * (double) (input - in_min);
	return (int) output;

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

int servo_comand_line(Mat img) {

	int w = img.cols;
	int h = img.rows;

	std::vector<int> l, r;

	cout << "left++" << endl;

	for (int i = 0; i < 2 * LINE_DISTANCE; i += 1) {
		if (img.at<uchar>(Point(i + LEFT_X_1, LEFT_Y)) == 255) {
			cout << "px " << i + LEFT_X_1 << endl;
			l.push_back(i + LEFT_X_1);
		}
	}

	cout << "right++" << endl;

	for (int i = 0; i < 2 * LINE_DISTANCE; i += 1) {
		if (img.at<uchar>(Point(i + RIGHT_X_1, RIGHT_Y)) == 255) {
			cout << "px " << i + RIGHT_X_1 << endl;
			r.push_back(i + RIGHT_X_1);
		}
	}


	double left_avg, right_avg;

	left_avg = std::accumulate(l.begin(), l.end(), 0.0) / l.size();
	right_avg = std::accumulate(r.begin(), r.end(), 0.0) / r.size();

	int servo_left = 0, servo_right = 0;

	if (l.size() > 0) {
		servo_left = map_servo(left_avg, LEFT_X_1, LEFT_X_2);
	}

	if (r.size() > 0) {
		servo_right = map_servo(right_avg, RIGHT_X_1, RIGHT_X_2);
	}




	return average_not_zero(servo_left, servo_right);
}

void one_poly(Mat img) {
	int w = img.cols;
	int h = img.rows;

	int lineType = 8;
	/* Create some points */
	Point pts[1][6];
	pts[0][0] = Point(0, 350);
	pts[0][1] = Point(0, 430);
	pts[0][2] = Point(157, h);
	pts[0][3] = Point(1085, h);
	pts[0][4] = Point(1230, 430);
	pts[0][5] = Point(1230, 350);

	const Point* ppt[1] = { pts[0] };
	int npt[] = { 6 };
	fillPoly(img, ppt, npt, 1, Scalar(255, 255, 255), lineType);
}

void two_poly(Mat img) {
	int w = img.cols;
	int h = img.rows;

	int lineType = 8;
	/* Create some points */

	Point pts[2][4];
	pts[0][0] = Point(90, 475);
	pts[0][1] = Point(90, 610);
	pts[0][2] = Point(300, 610);
	pts[0][3] = Point(300, 475);
	pts[1][0] = Point(980, 475);
	pts[1][1] = Point(980, 580);
	pts[1][2] = Point(1160, 580);
	pts[1][3] = Point(1160, 475);

	const Point* ppt[1] = { pts[0] };
	int npt[] = { 4 };
	fillPoly(img, ppt, npt, 2, Scalar(255, 255, 255), lineType);

	ppt[1] = {pts[1]};
	fillPoly(img, ppt, npt, 2, Scalar(255, 255, 255), lineType);
}

void two_lines(Mat img) {
	int left_y, right_y, x1l, x2l, x1r, x2r;

	left_y = LEFT_Y;
	right_y = RIGHT_Y;

	x1l = LEFT_X_1;
	x2l = LEFT_X_2;

	x1r = RIGHT_X_1;
	x2r = RIGHT_X_2;

	// the right selection line
	line(img, Point(x1r, right_y), Point(x2r, right_y), Scalar(255, 255, 255), 3, CV_AA);

	// the left selection line
	line(img, Point(x1l, left_y), Point(x2l, left_y), Scalar(255, 255, 255), 3, CV_AA);
}

int main(int argc, char** argv) {

	unsigned char* pixels;

	FILE* in = fopen("/dev/video", "rb");
	FILE* servo = fopen("/dev/servo", "r+b");
	//ofstream servo("/dev/servo", ofstream::binary);
	//int s = open("/dev/servo", O_WRONLY);

	int h, w, l;
	h = ioctl(in->_fileno, CHARVIDEO_IOCQHEIGHT);
	w = ioctl(in->_fileno, CHARVIDEO_IOCQWIDTH);
	l = ioctl(in->_fileno, CHARVIDEO_IOCQPIXELLEN);
	cout << h << endl << w << endl << l << endl;
	pixels = (unsigned char *) malloc(h * w * l * sizeof(char));

	for (size_t loop = 0; loop < 300; loop++) {
		cout << "loop=====================" << loop << endl;

		clock_t begin = clock();

		fread(pixels, 1, h * w * l, in);

		Mat image(h, w, CV_8UC3, &pixels[0]); //in case of BGR image use CV_8UC3

		Mat gray_image;
		cvtColor(image, gray_image, CV_RGB2GRAY);

		Mat blurred_image;
		GaussianBlur(gray_image, blurred_image, Size(5, 5), 0, 0);

		Mat canny_image;
		Canny(blurred_image, canny_image, 50, 150);

		Mat poly, poly2;
		poly = cv::Mat::zeros(canny_image.size(), canny_image.type());
		poly2 = cv::Mat::zeros(canny_image.size(), canny_image.type());
		one_poly(poly);
		two_lines(poly2);
		Mat region_image, region_image2;
		//blurred_image.copyTo(final_image, poly);
		bitwise_and(poly, canny_image, region_image);
		bitwise_and(poly2, canny_image, region_image2);
		//region_image.copyTo(region_image2);

//		vector<Vec4i> lines;
//		HoughLinesP(region_image, lines, 1, CV_PI / 180, 50, 50, 10);
//
////	Mat poly2;
////	poly2 = cv::Mat::zeros(canny_image.size(), canny_image.type());
////	//TwoPoly(poly2);
////	Mat final_image2;
////	blurred_image.copyTo(final_image2, poly2);
//
//		vector<Vec4i> l_l;
//		vector<Vec4i> l_r;
//		for (size_t i = 0; i < lines.size(); i++) {
//			Vec4i l = lines[i];
//			if (min(l[0], l[2]) < 630) {
//				l_l.push_back(l);
//			} else {
//				l_r.push_back(l);
//			}
//
//		}
//
//		std::vector<int> x1, x2, y1, y2;
//
//		double x1r, x2r, y1r, y2r, x1l, x2l, y1l, y2l;
//
//		double m;
//
//		for (size_t i = 0; i < l_l.size(); i++) {
//			Vec4i l = l_l[i];
//			if (l[2] - l[3] != 0) {
//				m = (l[1] - l[3]) / (l[0] - l[2]);
//				if (fabs(m) < 2) {
//					if (l[0] < l[2]) {
//						x1.push_back(l[0]);
//						y1.push_back(l[1]);
//						x2.push_back(l[2]);
//						y2.push_back(l[3]);
//					} else {
//						x1.push_back(l[2]);
//						y1.push_back(l[3]);
//						x2.push_back(l[0]);
//						y2.push_back(l[1]);
//					}
//
//				}
//			}
//		}
//
//		x1l = std::accumulate(x1.begin(), x1.end(), 0.0) / x1.size();
//		y1l = std::accumulate(y1.begin(), y1.end(), 0.0) / y1.size();
//		x2l = std::accumulate(x2.begin(), x2.end(), 0.0) / x2.size();
//		y2l = std::accumulate(y2.begin(), y2.end(), 0.0) / y2.size();
//
//		x1.clear();
//		x2.clear();
//		y1.clear();
//		y2.clear();
//
//		for (size_t i = 0; i < l_r.size(); i++) {
//			Vec4i l = l_r[i];
//			if (l[2] - l[3] != 0) {
//				m = (l[1] - l[3]) / (l[0] - l[2]);
//				if (fabs(m) < 2) {
//					if (l[0] < l[2]) {
//						x1.push_back(l[0]);
//						y1.push_back(l[1]);
//						x2.push_back(l[2]);
//						y2.push_back(l[3]);
//					} else {
//						x1.push_back(l[2]);
//						y1.push_back(l[3]);
//						x2.push_back(l[0]);
//						y2.push_back(l[1]);
//					}
//
//				}
//			}
//
//		}
//
//		x1r = std::accumulate(x1.begin(), x1.end(), 0.0) / x1.size();
//		y1r = std::accumulate(y1.begin(), y1.end(), 0.0) / y1.size();
//		x2r = std::accumulate(x2.begin(), x2.end(), 0.0) / x2.size();
//		y2r = std::accumulate(y2.begin(), y2.end(), 0.0) / y2.size();
//
//		x1.clear();
//		x2.clear();
//		y1.clear();
//		y2.clear();
//
//		double m_r, m_l;
//
//		m_r = (y2r - y1r) / (x2r - x1r);
//		m_l = (y2l - y1l) / (x2l - x1l);
//
//		clock_t end = clock();
//		double elapsed = double(end - begin);
//
//		cout << "lines = " << lines.size() << endl;
//		cout << "m_r = " << m_r << endl;
//		cout << "m_l = " << m_l << endl;
//		cout << "sum of slopes = " << m_r + m_l << endl;
//		unsigned short servo_out = servo_comand_map(m_r, m_l, -0.5, 0.5);


		int servo_out = servo_comand_line(region_image2);

		cout << "servo = " << servo_out << endl;
		//cout << "time = " << elapsed << endl;


		if (servo_out == -1) {
			continue;
		}
		if (servo_out <= 220)
			servo_out = 220;
		if (servo_out >= 380)
			servo_out = 380;

		cout << "Servo write: " << write(servo->_fileno, &servo_out, 2) << endl;

//		for (size_t i = 0; i < lines.size(); i++) {
//			Vec4i l = lines[i];
//			line(image, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 0, 255), 3, CV_AA);
//		}
//		line(image, Point(x1r, y1r), Point(x2r, y2r), Scalar(255, 0, 0), 3, CV_AA);
//		line(image, Point(x1l, y1l), Point(x2l, y2l), Scalar(0, 255, 0), 3, CV_AA);

		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(9);

		char name[20];
		sprintf(name, "img%d.png", loop);
//		try {
//			imwrite("im.png", poly2, compression_params);
//			imwrite(name, region_image2, compression_params);
//
//		} catch (runtime_error& ex) {
//			fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
//			return 1;
//		}

		//image.release();
	}

	fclose(in);
	fclose(servo);

	return 0;
}
