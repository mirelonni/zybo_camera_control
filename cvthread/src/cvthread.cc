#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>
#include <chrono>

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

#include <signal.h>

#include <thread>
#include <mutex>

#define CHARVIDEO_IOC_MAGIC  '8'
#define MOTION_IOC_MAGIC  '9'

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
#define SERVO_FINE 15

#define MOTION_IOCTSETENABLE    _IO(MOTION_IOC_MAGIC, 0)
#define MOTION_IOCTSETDIR	_IO(MOTION_IOC_MAGIC, 1)

#define RESIZE_FACTOR 0.5

#define Y_1 (560 * RESIZE_FACTOR)

#define LINE_DISTANCE_1 (120 * RESIZE_FACTOR)
#define LINE_DISTANCE_1_OUT (100 * RESIZE_FACTOR)
#define LINE_DISTANCE_1_IN (280 * RESIZE_FACTOR)

#define LEFT_MEAN_1 (172 * RESIZE_FACTOR)
#define LEFT_X_1_1 ((LEFT_MEAN_1 - LINE_DISTANCE_1_OUT))
#define LEFT_X_2_1 ((LEFT_MEAN_1 + LINE_DISTANCE_1_IN))

#define RIGHT_MEAN_1 (1100 * RESIZE_FACTOR)
#define RIGHT_X_1_1 ((RIGHT_MEAN_1 - LINE_DISTANCE_1_IN))
#define RIGHT_X_2_1 ((RIGHT_MEAN_1 + LINE_DISTANCE_1_OUT))

#define Y_2 (440 * RESIZE_FACTOR)

#define LINE_DISTANCE_2 (25 * RESIZE_FACTOR)

#define LEFT_MEAN_2 (304 * RESIZE_FACTOR)
#define LEFT_X_1_2 ((LEFT_MEAN_2 - LINE_DISTANCE_2))
#define LEFT_X_2_2 ((LEFT_MEAN_2 + LINE_DISTANCE_2))

#define RIGHT_MEAN_2 (963 * RESIZE_FACTOR)
#define RIGHT_X_1_2 ((RIGHT_MEAN_2 - LINE_DISTANCE_2))
#define RIGHT_X_2_2 ((RIGHT_MEAN_2 + LINE_DISTANCE_2))

#define Y_3 (300 * RESIZE_FACTOR)

#define LINE_DISTANCE_3 (25 * RESIZE_FACTOR)

#define LEFT_MEAN_3 (481 * RESIZE_FACTOR)
#define LEFT_X_1_3 ((LEFT_MEAN_3 - LINE_DISTANCE_3))
#define LEFT_X_2_3 ((LEFT_MEAN_3 + LINE_DISTANCE_3))

#define RIGHT_MEAN_3 (794 * RESIZE_FACTOR)
#define RIGHT_X_1_3 ((RIGHT_MEAN_3 - LINE_DISTANCE_3))
#define RIGHT_X_2_3 ((RIGHT_MEAN_3 + LINE_DISTANCE_3))

#define SIGN_MIN 55
#define SIGN_MAX 75

#define CLK_FREQ 50000000.0f // FCLK0 frequency not found in xparameters.h
const double clk_to_cm = (((1000000.0f / CLK_FREQ) * 2.54f) / 147.0f);

#define FPS 5
#define LOOP_TIME (1000000 / FPS)

int f_motors;
int f_servo;

using namespace cv;
using namespace std;

CascadeClassifier stop_cascade;
String stop_cascade_name = "stop.xml";

std::mutex IMAGE_mutex;
std::mutex COUT_mutex;
std::mutex SPEED_mutex;
std::mutex STOP_mutex;

int full_map(double input, double in_min, double in_max, double out_min, double out_max) {

	double slope = (double) (out_max - out_min) / (double) (in_max - in_min);
	double output = (double) out_min + slope * (double) (input - in_min);
	return (int) output;
}

int map_servo(double input, double in_min, double in_max) {

	return full_map(input, in_min, in_max, SERVO_LEFT, SERVO_RIGHT);

}

int map_servo_fine(double input, double in_min, double in_max, double mean) {

	if (input < mean) {
		return full_map(input, in_min, mean, SERVO_LEFT, SERVO_CENTER);
	} else {
		return full_map(input, mean, in_max, SERVO_CENTER, SERVO_RIGHT);
	}

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

int choose_servo(int left, int right, int mean) {
	if (left != 0 && right != 0) {
		if (abs(right - left) < SERVO_FINE) {
			return average_not_zero(left, right);
		} else {
			if (left < mean && right < mean) {
				return min(left, right);
			} else if (left > mean && right > mean) {
				return max(left, right);
			} else {
				return average_not_zero(left, right);
			}
		}
	} else {
		return average_not_zero(left, right);
	}
}

std::vector<int> choose_advanced_servo(int left_1, int right_1, int left_2, int right_2, int left_3, int right_3, int mean, unsigned short usr_speed) {

	std::vector<int> ret;

	if (left_2 != 0 && right_2 != 0) {
		if (left_2 != 0 && right_2 != 0) {
			ret.push_back(usr_speed * 1.5f);
			ret.push_back(mean);
		} else {
			ret.push_back(usr_speed * 1.25f);
			ret.push_back(mean);
		}
	} else {
		ret.push_back(usr_speed);
		ret.push_back(choose_servo(left_1, right_1, mean));
	}
	return ret;

}

double find_avg_point_on_line(Mat frame_pixels, Mat frame_image, int line_y, int line_start, int line_stop, int param) {

	std::vector<int> v;
	double ret = -1;
	for (int i = 0; i < line_stop - line_start + 3; i += 1) {
		if (frame_pixels.at<uchar>(Point(i + line_start, line_y)) == 255) {
			if (param > 1 || param == -1) {
				COUT_mutex.lock();
				cout << "px " << i + line_start << endl;
				COUT_mutex.unlock();
			}
			if (param > 1 || param == -1) {
				line(frame_image, Point(i + line_start, line_y - 7), Point(i + line_start, line_y + 7), Scalar(0, 0, 255), 4, CV_AA);
			}
			v.push_back(i + line_start);
		}
	}

	if (v.size() > 0) {
		ret = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
		line(frame_image, Point(ret, line_y - 10), Point(ret, line_y + 10), Scalar(255, 0, 255), 3, CV_AA);
	}
	return ret;
}

std::vector<int> servo_comand_line(Mat frame_pixels, Mat frame_image, int param, unsigned char usr_speed) {

	double left_avg_1, right_avg_1, left_avg_2, right_avg_2, left_avg_3, right_avg_3;

	if (param > 1 || param == -1) {
		COUT_mutex.lock();
		cout << "left1++" << endl;
		COUT_mutex.unlock();
	}
	left_avg_1 = find_avg_point_on_line(frame_pixels, frame_image, Y_1, LEFT_X_1_1, LEFT_X_2_1, param);

	if (param > 1 || param == -1) {
		COUT_mutex.lock();
		cout << "right1++" << endl;
		COUT_mutex.unlock();
	}
	right_avg_1 = find_avg_point_on_line(frame_pixels, frame_image, Y_1, RIGHT_X_1_1, RIGHT_X_2_1, param);

	if (param > 1 || param == -1) {
		COUT_mutex.lock();
		cout << "left2++" << endl;
		COUT_mutex.unlock();
	}
	left_avg_2 = find_avg_point_on_line(frame_pixels, frame_image, Y_2, LEFT_X_1_2, LEFT_X_2_2, param);

	if (param > 1 || param == -1) {
		COUT_mutex.lock();
		cout << "right2++" << endl;
		COUT_mutex.unlock();
	}
	right_avg_2 = find_avg_point_on_line(frame_pixels, frame_image, Y_2, RIGHT_X_1_2, RIGHT_X_2_2, param);

	if (param > 1 || param == -1) {
		COUT_mutex.lock();
		cout << "left3++" << endl;
		COUT_mutex.unlock();
	}
	left_avg_3 = find_avg_point_on_line(frame_pixels, frame_image, Y_3, LEFT_X_1_3, LEFT_X_2_3, param);

	if (param > 1 || param == -1) {
		COUT_mutex.lock();
		cout << "right3++" << endl;
		COUT_mutex.unlock();
	}
	right_avg_3 = find_avg_point_on_line(frame_pixels, frame_image, Y_3, RIGHT_X_1_3, RIGHT_X_2_3, param);

	int servo_left1 = 0, servo_right1 = 0, servo_left2 = 0, servo_right2 = 0, servo_left3 = 0, servo_right3 = 0;

	if (left_avg_1 != -1) {
		servo_left1 = map_servo_fine(left_avg_1, LEFT_X_1_1, LEFT_X_2_1, LEFT_MEAN_1);
		if (left_avg_2 != -1) {
			servo_left2 = map_servo_fine(left_avg_2, LEFT_X_1_2, LEFT_X_2_2, LEFT_MEAN_2);
			if (left_avg_3 != -1) {
				servo_left3 = map_servo_fine(left_avg_3, LEFT_X_1_3, LEFT_X_2_3, LEFT_MEAN_3);
			}
		}
	}

	if (right_avg_1 != -1) {
		servo_right1 = map_servo_fine(right_avg_1, RIGHT_X_1_1, RIGHT_X_2_1, RIGHT_MEAN_1);
		if (right_avg_2 != -1) {
			servo_right2 = map_servo_fine(right_avg_2, RIGHT_X_1_2, RIGHT_X_2_2, RIGHT_MEAN_2);
			if (right_avg_3 != -1) {
				servo_right3 = map_servo_fine(right_avg_3, RIGHT_X_1_3, RIGHT_X_2_3, RIGHT_MEAN_3);
			}
		}
	}

	if (param > 0 || param == -1) {
		COUT_mutex.lock();
		cout << "lft1  = " << servo_left1 << ", rgt1 = " << servo_right1 << endl;
		cout << "lft2  = " << servo_left2 << ", rgt2 = " << servo_right2 << endl;
		cout << "lft3  = " << servo_left3 << ", rgt3 = " << servo_right3 << endl;
		COUT_mutex.unlock();
	}

	return choose_advanced_servo(servo_left1, servo_right1, servo_left2, servo_right2, servo_left3, servo_right3, SERVO_CENTER, usr_speed);
}

void one_poly_sign(Mat img) {
	int w = img.cols;
	int h = img.rows;

	int x1, x2, y1, y2;
	x1 = (int) w / 2;
	y1 = 0;
	x2 = w;
	y2 = h;

	int lineType = 8;
	/* Create some points */
	Point pts[1][4];
	pts[0][0] = Point(x1, y1);
	pts[0][1] = Point(x1, y2);
	pts[0][2] = Point(x2, y2);
	pts[0][3] = Point(x2, y1);

	const Point* ppt[1] = { pts[0] };
	int npt[] = { 4 };
	fillPoly(img, ppt, npt, 1, Scalar(255, 255, 255), lineType);
}

Mat crop(Mat frame, int x1, int y1, int x2, int y2) {

	Rect roi;
	roi.x = x1;
	roi.y = y1;
	roi.width = x2 - x1;
	roi.height = y2 - y1;

	Mat crop = frame(roi);
	return crop;
}

void lines(Mat img) {
// the 1st selection lines
	line(img, Point(RIGHT_X_1_1, Y_1), Point(RIGHT_X_2_1, Y_1), Scalar(255, 255, 255), 3, CV_AA);
	line(img, Point(LEFT_X_1_1, Y_1), Point(LEFT_X_2_1, Y_1), Scalar(255, 255, 255), 3, CV_AA);
// the 2nd selection lines
	line(img, Point(RIGHT_X_1_2, Y_2), Point(RIGHT_X_2_2, Y_2), Scalar(255, 255, 255), 3, CV_AA);
	line(img, Point(LEFT_X_1_2, Y_2), Point(LEFT_X_2_2, Y_2), Scalar(255, 255, 255), 3, CV_AA);
// the 3rd selection lines
	line(img, Point(RIGHT_X_1_3, Y_3), Point(RIGHT_X_2_3, Y_3), Scalar(255, 255, 255), 3, CV_AA);
	line(img, Point(LEFT_X_1_3, Y_3), Point(LEFT_X_2_3, Y_3), Scalar(255, 255, 255), 3, CV_AA);
}

int detect_and_display(Mat frame, int param) {

	int ret = 0;
	std::vector<Rect> stop_signs;
	Mat frame_gray;
	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);
//-- Detect stop_signs
	stop_cascade.detectMultiScale(frame_gray, stop_signs, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30));
	if (param > 0 || param == -1) {
		COUT_mutex.lock();
		cout << "stop signs = " << stop_signs.size() << endl;
		COUT_mutex.unlock();
	}

	for (size_t i = 0; i < stop_signs.size(); i++) {
		if (param > 0 || param == -1) {
			COUT_mutex.lock();
			cout << "stop " << i << " height = " << stop_signs[i].height << endl;
			cout << "stop " << i << " width = " << stop_signs[i].width << endl;
			COUT_mutex.unlock();

			// draw a ellipse around the sign
			if (param > 2 || param == -1) {
				Point center(stop_signs[i].x + stop_signs[i].width / 2, stop_signs[i].y + stop_signs[i].height / 2);
				ellipse(frame, center, Size(stop_signs[i].width / 2, stop_signs[i].height / 2), 0, 0, 360, Scalar(255, 0, 255), 4, 8, 0);
			}
		}
		if (stop_signs[i].height >= SIGN_MIN && stop_signs[i].height <= SIGN_MAX) {
			ret = 1;
		}
	}
	return ret;

}


//char sign_done_flag sign_speed_flag;
Mat IMAGE;
int h, w, l;
char lane_done = 0;
struct sigaction sigIntHandler;
//unsigned int speed;
char stop_sign;


//not functioning
unsigned int speed_out(int speed) {

	static int old_stop_sgn = 0;
	static int loop = 0;
	STOP_mutex.lock();
	int stop_sgn = stop_sign;
	STOP_mutex.unlock();
	if (stop_sgn == 1 && old_stop_sgn == 0) {
		old_stop_sgn = 1;
	}
	if (old_stop_sgn == 1 && loop == 20) {
		old_stop_sgn = 0;
	}

	if (old_stop_sgn == 1) {
		speed = 0;
		loop ++;
	} else {
		loop = 0;
	}

	return speed;
}

void my_handler(int s) {

	unsigned int speed = 0;
	int servo_out = SERVO_CENTER;
	lane_done = 1;

	write(f_servo, &servo_out, 2);
	write(f_motors, &speed, 4);

	exit(1);
}

void lane_component(int argc, int param, int iterations, FILE* camera, FILE* servo, FILE* motors, FILE* sonar, unsigned short usr_speed, double stop_dist) {

	int servo_out = 300;
	int old_servo_out = servo_out;

	int mtr_write, srv_write;

	int clk_edges;
	int dist = 0;

	unsigned int speed_local = (usr_speed << 16) + usr_speed;
	unsigned int speed;

	unsigned char* pixels;
	pixels = (unsigned char *) malloc(h * w * l * sizeof(char));

	auto start = std::chrono::high_resolution_clock::now();
	auto finish = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);

	unsigned short fast_speed;



	for (int loop = 0; loop < iterations; loop++) {

		start = std::chrono::high_resolution_clock::now();

		COUT_mutex.lock();
		cout << "loop=============" << loop << endl;
		COUT_mutex.unlock();

		if (loop == 0 && param == -1) {
			IMAGE_mutex.lock();
			IMAGE = imread("test.png", CV_LOAD_IMAGE_COLOR);
			IMAGE_mutex.unlock();
		} else {
			fread(pixels, 1, h * w * l, camera);
			IMAGE_mutex.lock();
			IMAGE = Mat(h, w, CV_8UC3, &pixels[0]);
			IMAGE_mutex.unlock();
		}

		//MUTEX
		Mat frame_1, frame;
		IMAGE_mutex.lock();
		IMAGE.copyTo(frame_1);
		IMAGE_mutex.unlock();
		resize(frame_1, frame, cv::Size(), (double) RESIZE_FACTOR, (double) RESIZE_FACTOR);

		Mat gray_image;
		cvtColor(frame, gray_image, CV_RGB2GRAY);

		Mat blurred_image;
		GaussianBlur(gray_image, blurred_image, Size(5, 5), 0, 0);

		Mat canny_image;
		Canny(blurred_image, canny_image, 50, 150);

		// make the selection areas for the images
		Mat poly;
		poly = cv::Mat::zeros(frame.size(), canny_image.type());
		lines(poly);

		// apply selection areas to said images
		Mat region_image_lane;
		bitwise_and(poly, canny_image, region_image_lane);

		vector<int> line_ret = servo_comand_line(region_image_lane, frame, param, usr_speed);

		fast_speed = line_ret[0];
		servo_out = line_ret[1];

		if (argc > 5) {
			read(sonar->_fileno, &clk_edges, 4);
			dist = clk_edges * clk_to_cm;
			if (dist < stop_dist && dist != 0) {
				speed_local = 0;
			} else {
				speed_local = (fast_speed << 16) + fast_speed;
			}
		}

		speed = speed_out(speed_local);

		if (servo_out == -1) {
			servo_out = old_servo_out;
		}

		if (servo_out <= SERVO_LEFT)
			servo_out = SERVO_LEFT;
		if (servo_out >= SERVO_RIGHT)
			servo_out = SERVO_RIGHT;

		old_servo_out = servo_out;

		mtr_write = write(motors->_fileno, &speed, 4);
		srv_write = write(servo->_fileno, &servo_out, 2);

		if (param > 0 || param == -1) {
			COUT_mutex.lock();

			cout << "servo = " << servo_out << endl;
			cout << "speed = " << (speed << 16) << endl;
			cout << "dist  = " << dist << endl;

			if (param > 2) {
				cout << "Servo write: " << srv_write << endl;
				cout << "Motor write: " << mtr_write << endl;

			}
			COUT_mutex.unlock();
		}
		if (param > 1 || param == -1) {
			line(frame, Point(RIGHT_X_1_1, Y_1), Point(RIGHT_X_2_1, Y_1), Scalar(100, 0, 0), 2, CV_AA);
			line(frame, Point(LEFT_X_1_1, Y_1), Point(LEFT_X_2_1, Y_1), Scalar(0, 100, 0), 2, CV_AA);
			line(frame, Point(RIGHT_X_1_2, Y_2), Point(RIGHT_X_2_2, Y_2), Scalar(100, 0, 0), 2, CV_AA);
			line(frame, Point(LEFT_X_1_2, Y_2), Point(LEFT_X_2_2, Y_2), Scalar(0, 100, 0), 2, CV_AA);
			line(frame, Point(RIGHT_X_1_3, Y_3), Point(RIGHT_X_2_3, Y_3), Scalar(100, 0, 0), 2, CV_AA);
			line(frame, Point(LEFT_X_1_3, Y_3), Point(LEFT_X_2_3, Y_3), Scalar(0, 100, 0), 2, CV_AA);

			line(frame, Point(LEFT_MEAN_1, Y_1 - 5), Point(LEFT_MEAN_1, Y_1 + 5), Scalar(0, 255, 255), 2, CV_AA);
			line(frame, Point(RIGHT_MEAN_1, Y_1 - 5), Point(RIGHT_MEAN_1, Y_1 + 5), Scalar(0, 255, 255), 2, CV_AA);
			line(frame, Point(LEFT_MEAN_2, Y_2 - 5), Point(LEFT_MEAN_2, Y_2 + 5), Scalar(0, 255, 255), 2, CV_AA);
			line(frame, Point(RIGHT_MEAN_2, Y_2 - 5), Point(RIGHT_MEAN_2, Y_2 + 5), Scalar(0, 255, 255), 2, CV_AA);
			line(frame, Point(LEFT_MEAN_3, Y_3 - 5), Point(LEFT_MEAN_3, Y_3 + 5), Scalar(0, 255, 255), 2, CV_AA);
			line(frame, Point(RIGHT_MEAN_3, Y_3 - 5), Point(RIGHT_MEAN_3, Y_3 + 5), Scalar(0, 255, 255), 2, CV_AA);

			vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
			compression_params.push_back(9);

			char name[20];
			sprintf(name, "img%d.png", loop);

			char reg_name[65];
			sprintf(reg_name, "img_region_image_lane%d.png", loop);
			try {

				imwrite(name, frame, compression_params);

				if (param > 2 || param == -1) {
					imwrite(reg_name, region_image_lane, compression_params);
				}
				if (param == -1) {
					imwrite("img_poly.png", poly, compression_params);
				}

			} catch (runtime_error& ex) {
				COUT_mutex.lock();
				cerr << "Exception converting image to PNG format: " << ex.what() << endl;
				COUT_mutex.unlock();
			}
		}

		sigaction(SIGINT, &sigIntHandler, NULL);
//		COUT_mutex.lock();
//		cout << "1" << endl;
//		COUT_mutex.unlock();
		//usleep(100000);
		finish = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);

		if (duration.count() < LOOP_TIME) {
			usleep(LOOP_TIME - duration.count());
		}

	}

	lane_done = 1;

}

void sign_component(int param) {

	auto start = std::chrono::high_resolution_clock::now();
	auto finish = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);

	int loop = 0;

	while (lane_done == 0) {

		start = std::chrono::high_resolution_clock::now();

		Mat frame;
		//MUTEX
		IMAGE_mutex.lock();
		IMAGE.copyTo(frame);
		IMAGE_mutex.unlock();

		if (frame.rows != 0) {

			Mat region_image_sign_1, region_image_sign_2;

			region_image_sign_1 = crop(frame, frame.cols / 2, 0, frame.cols, frame.rows);

			cv::resize(region_image_sign_1, region_image_sign_2, cv::Size(), 0.5, 0.5);

			STOP_mutex.lock();
			stop_sign = detect_and_display(region_image_sign_2, param);
			STOP_mutex.unlock();

			if (param > 1 || param == -1) {

				vector<int> compression_params;
				compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
				compression_params.push_back(9);

				char name[65];
				sprintf(name, "img_region_image_sign%d.png", loop);

				try {

					if (param > 2 || param == -1) {

						imwrite(name, region_image_sign_2, compression_params);
					}

				} catch (runtime_error& ex) {
					cerr << "Exception converting image to PNG format: " << ex.what() << endl;
				}
			}
//			COUT_mutex.lock();
//			cout << "2" << endl;
//			COUT_mutex.unlock();
			//usleep(100000);
			finish = std::chrono::high_resolution_clock::now();
			duration = std::chrono::duration_cast<std::chrono::microseconds>(finish - start);

			if (duration.count() < LOOP_TIME) {
				usleep(LOOP_TIME - duration.count());
			}
			loop++;
		}

	}

}

int main(int argc, char** argv) {

	cout << "OpenCV version : " << CV_VERSION << endl;

	int param = 0;

	if (argc < 5) {
		cerr << argv[0] << " <debug param> <number of iterations> <speed> <direction> <stop distance>" << endl << "0 - nothing" << endl << "1 - just text" << endl
				<< "2 - one *relevant* image" << endl << "3 - more *relevant* images" << endl << "-1 - full debug mode test.png used" << endl;
		return -1;
	}

	param = atoi(argv[1]);

	FILE* camera = fopen("/dev/video", "rb");
	if (camera < 0) {
		cerr << "Failed to open camera." << endl;
		return -1;
	}

	FILE* servo = fopen("/dev/servo", "r+b");
	if (servo < 0) {
		cerr << "Failed to open servo." << endl;
		fclose(camera);
		return -1;
	}

	FILE* motors = fopen("/dev/motors", "r+b");
	if (motors < 0) {
		cerr << "Failed to open motors." << endl;
		fclose(camera);
		fclose(servo);
		return -1;
	}

	FILE* sonar = fopen("/dev/sonar", "rb");
	if (argc > 5) {
		if (sonar < 0) {
			cerr << "Failed to open sonar." << endl;
			fclose(camera);
			fclose(servo);
			fclose(motors);
			return -1;
		}
	}

	f_motors = motors->_fileno;
	f_servo = servo->_fileno;

	int iterations = atoi(argv[2]);
	if (iterations < 0) {
		cerr << "Bad number of iterations." << endl;
		fclose(camera);
		fclose(servo);
		fclose(motors);
		fclose(sonar);
		return -1;
	} else if (iterations == 42) {
		iterations = 10000;
	}
	if (param == -1) {
		iterations = 1;
	}

	unsigned short usr_speed = atoi(argv[3]);

	unsigned int left_dir = atoi(argv[4]);
	if (left_dir < 0 && left_dir > 1) {
		cerr << "Bad direction." << endl;
		fclose(camera);
		fclose(servo);
		fclose(motors);
		fclose(sonar);
		return -1;
	}
	unsigned int right_dir = left_dir;
	ioctl(motors->_fileno, MOTION_IOCTSETDIR, ((left_dir & 1) << 1) + (right_dir & 1));

	unsigned int enable = 1;
	ioctl(motors->_fileno, MOTION_IOCTSETENABLE, enable);

	int stock_servo_out = 300;
	write(servo->_fileno, &stock_servo_out, 2);

	int stock_speed = 0;
	write(motors->_fileno, &stock_speed, 4);

	double stop_dist = 0;
	if (argc > 5) {
		stop_dist = atoi(argv[5]);
	}

	if (!stop_cascade.load(stop_cascade_name)) {
		cerr << "--(!)Error loading stop cascade" << endl;
		fclose(camera);
		fclose(servo);
		fclose(motors);
		if (argc > 5) {
			fclose(sonar);
		}
		return -1;
	};

	memset(&sigIntHandler, 0, sizeof(sigIntHandler));

	sigIntHandler.sa_flags = SA_RESETHAND;
	sigIntHandler.sa_handler = my_handler;

	sigaction(SIGINT, &sigIntHandler, NULL);

	h = ioctl(camera->_fileno, CHARVIDEO_IOCQHEIGHT);
	w = ioctl(camera->_fileno, CHARVIDEO_IOCQWIDTH);
	l = ioctl(camera->_fileno, CHARVIDEO_IOCQPIXELLEN);
	cout << h << endl << w << endl << l << endl;

	std::thread t1(lane_component, argc, param, iterations, camera, servo, motors, sonar, usr_speed, stop_dist);
	std::thread t2(sign_component, param);

	t1.join();
	t2.join();

	stock_servo_out = SERVO_CENTER;
	write(servo->_fileno, &stock_servo_out, 2);
	stock_speed = 0;
	write(motors->_fileno, &stock_speed, 4);

	fclose(camera);
	fclose(servo);
	fclose(motors);
	if (argc > 5) {
		fclose(sonar);
	}

	return 0;
}
