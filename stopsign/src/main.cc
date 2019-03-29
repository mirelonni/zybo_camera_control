#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>

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
#define MOTION_IOC_MAGIC  '9'

#define CHARVIDEO_IOCHALT    _IO(CHARVIDEO_IOC_MAGIC, 0)
#define CHARVIDEO_IOCSTART    _IO(CHARVIDEO_IOC_MAGIC, 1)
#define CHARVIDEO_IOCSTATUS    _IO(CHARVIDEO_IOC_MAGIC, 2)

#define CHARVIDEO_IOCQHEIGHT _IOR(CHARVIDEO_IOC_MAGIC,  3, int)
#define CHARVIDEO_IOCQWIDTH _IOR(CHARVIDEO_IOC_MAGIC,  4, int)
#define CHARVIDEO_IOCQPIXELLEN _IOR(CHARVIDEO_IOC_MAGIC,  5, int)
#define CHARVIDEO_IOCQBUFSIZE _IOR(CHARVIDEO_IOC_MAGIC,  6, int)

using namespace cv;
using namespace std;

/* Function Headers */
void detectAndDisplay(Mat frame);
/* Global variables */
String stop_cascade_name = "stop.xml";
CascadeClassifier stop_cascade;

void detectAndDisplay(Mat frame) {

	std::vector<Rect> stop_signs;
	Mat frame_gray;
	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);
	//-- Detect stop_signs
	stop_cascade.detectMultiScale(frame_gray, stop_signs, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30));

	cout << "stop signs = " << stop_signs.size() << endl;


	for (size_t i = 0; i < stop_signs.size(); i++) {
		Point center(stop_signs[i].x + stop_signs[i].width / 2, stop_signs[i].y + stop_signs[i].height / 2);
		ellipse(frame, center, Size(stop_signs[i].width / 2, stop_signs[i].height / 2), 0, 0, 360, Scalar(255, 0, 255), 4, 8, 0);
	}

}

int main(int argc, char** argv) {

	cout << "OpenCV version : " << CV_VERSION << endl;

	if (argc < 3) {
		cerr << "./opencvcarctrl.elf <debug param> <number of iterations>" << endl << "0 - nothing" << endl << "1 - just text" << endl << "2 - one *relevant* image" << endl
				<< "3 - more *relevant* images" << endl;
		return -1;
	}

	FILE* camera = fopen("/dev/video", "rb");
	if (camera < 0) {
		cerr << "Failed to open camera." << endl;
		return -1;
	}

	int iterations = atoi(argv[2]);
	if (iterations < 0) {
		cerr << "Bad number of iterations." << endl;
		fclose(camera);
		return -1;
	}

	if (!stop_cascade.load(stop_cascade_name)) {
		printf("--(!)Error loading stop cascade\n");
		return -1;
	};

	unsigned char* pixels;
	int h, w, l;
	h = ioctl(camera->_fileno, CHARVIDEO_IOCQHEIGHT);
	w = ioctl(camera->_fileno, CHARVIDEO_IOCQWIDTH);
	l = ioctl(camera->_fileno, CHARVIDEO_IOCQPIXELLEN);
	cout << h << endl << w << endl << l << endl;
	pixels = (unsigned char *) malloc(h * w * l * sizeof(char));

	for (int loop = 0; loop < iterations; loop++) {
		if (atoi(argv[1]) > 0) {
			cout << "loop=====================" << loop << endl;
		}

		//clock_t begin = clock();

		fread(pixels, 1, h * w * l, camera);

		Mat image(h, w, CV_8UC3, &pixels[0]); //in case of BGR image use CV_8UC3

		detectAndDisplay(image);

		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(9);
		char name[20];
		sprintf(name, "img%d.png", loop);
		try {
			if (atoi(argv[1]) > 1) {
				imwrite(name, image, compression_params);
			}

		} catch (runtime_error& ex) {
			cerr << "Exception converting image to PNG format: " << ex.what() << endl;
			return 1;
		}

	}

	fclose(camera);

	return 0;
}
