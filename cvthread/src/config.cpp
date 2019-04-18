#include "config.hpp"

prop_codes string_to_prop(std::string const& inString) {

	if (inString == "resize_factor")
		return E_resize_factor;
	if (inString == "y_1")
		return E_y_1;
	if (inString == "y_2")
		return E_y_2;
	if (inString == "y_3")
		return E_y_3;
	if (inString == "line_dist_1_out")
		return E_line_dist_1_out;
	if (inString == "line_dist_1_in")
		return E_line_dist_1_in;
	if (inString == "line_dist_1")
		return E_line_dist_1;
	if (inString == "line_dist_2")
		return E_line_dist_2;
	if (inString == "line_dist_3")
		return E_line_dist_3;
	if (inString == "servo_fine")
		return E_servo_fine;
	if (inString == "min_speed")
		return E_min_speed;
	if (inString == "max_speed")
		return E_max_speed;
	if (inString == "min_adj_servo")
		return E_min_adj_servo;
	if (inString == "max_adj_servo")
		return E_max_adj_servo;
	if (inString == "sign_min")
		return E_sign_min;
	if (inString == "sign_max")
		return E_sign_max;
	if (inString == "fps")
		return E_fps;
	if (inString == "left_mean_1")
		return E_left_mean_1;
	if (inString == "left_mean_2")
		return E_left_mean_2;
	if (inString == "left_mean_3")
		return E_left_mean_3;
	if (inString == "right_mean_1")
		return E_right_mean_1;
	if (inString == "right_mean_2")
		return E_right_mean_2;
	if (inString == "right_mean_3")
		return E_right_mean_3;
	if (inString == "$")
		return E_calib;

	return E_invalid;

}

struct properties configure(int param, std::string filename, std::vector<double> mean) {

	int seek_calib = 0;

	struct properties configuration;

	std::string line;
	std::string prop;
	std::ifstream config_file;
	try {
		config_file.open(filename);

		while (std::getline(config_file, line)) {
			std::istringstream is_line(line);
			std::getline(is_line, prop, '=');
			std::string value;
			std::getline(is_line, value);

			switch (string_to_prop(prop)) {
			case E_resize_factor:
				configuration.resize_factor = std::stof(value);
				break;
			case E_y_1:
				configuration.y_1 = std::stoi(value);
				break;
			case E_y_2:
				configuration.y_2 = std::stoi(value);
				break;
			case E_y_3:
				configuration.y_3 = std::stoi(value);
				break;
			case E_line_dist_1_out:
				configuration.line_distance_1_out = std::stoi(value);
				break;
			case E_line_dist_1_in:
				configuration.line_distance_1_in = std::stoi(value);
				break;
			case E_line_dist_1:
				configuration.line_distance_1 = std::stoi(value);
				break;
			case E_line_dist_2:
				configuration.line_distance_2 = std::stoi(value);
				break;
			case E_line_dist_3:
				configuration.line_distance_3 = std::stoi(value);
				break;
			case E_servo_fine:
				configuration.servo_fine = std::stoi(value);
				break;
			case E_min_speed:
				configuration.min_speed = std::stoi(value);
				break;
			case E_max_speed:
				configuration.max_speed = std::stoi(value);
				break;
			case E_min_adj_servo:
				configuration.min_adj_servo = std::stof(value);
				break;
			case E_max_adj_servo:
				configuration.max_adj_servo = std::stof(value);
				break;
			case E_sign_min:
				configuration.sign_min = std::stoi(value);
				break;
			case E_sign_max:
				configuration.sign_max = std::stoi(value);
				break;
			case E_fps:
				configuration.fps = std::stoi(value);
				break;
			case E_left_mean_1:
				configuration.left_mean_1 = std::stoi(value);
				break;
			case E_left_mean_2:
				configuration.left_mean_2 = std::stoi(value);
				break;
			case E_left_mean_3:
				configuration.left_mean_3 = std::stoi(value);
				break;
			case E_right_mean_1:
				configuration.right_mean_1 = std::stoi(value);
				break;
			case E_right_mean_2:
				configuration.right_mean_2 = std::stoi(value);
				break;
			case E_right_mean_3:
				configuration.right_mean_3 = std::stoi(value);
				break;
			case E_calib:
				seek_calib = config_file.tellg();
				break;
			case E_invalid:
				continue;
			}

		}

		config_file.close();

//		configuration.y_1 *= configuration.resize_factor;
//		configuration.y_2 *= configuration.resize_factor;
//		configuration.y_3 *= configuration.resize_factor;
//		configuration.line_distance_1 *= configuration.resize_factor;
//		configuration.line_distance_1_out *= configuration.resize_factor;
//		configuration.line_distance_1_in *= configuration.resize_factor;
//		configuration.line_distance_2 *= configuration.resize_factor;
//		configuration.line_distance_3 *= configuration.resize_factor;
//		configuration.left_mean_1 *= configuration.resize_factor;
//		configuration.left_mean_2 *= configuration.resize_factor;
//		configuration.left_mean_3 *= configuration.resize_factor;
//		configuration.right_mean_1 *= configuration.resize_factor;
//		configuration.right_mean_2 *= configuration.resize_factor;
//		configuration.right_mean_3 *= configuration.resize_factor;

		configuration.loop_time = (1000000 / configuration.fps);

		if (param == 100) {
			std::ofstream config_file_out;
			config_file_out.open(filename, std::ios::in | std::ios::out);

			configuration.left_x_1_1 = ((configuration.left_mean_1 - configuration.line_distance_1_out));
			configuration.left_x_2_1 = ((configuration.left_mean_1 + configuration.line_distance_1_in));

			configuration.right_x_1_1 = ((configuration.right_mean_1 - configuration.line_distance_1_in));
			configuration.right_x_2_1 = ((configuration.right_mean_1 + configuration.line_distance_1_out));

			configuration.left_x_1_2 = ((configuration.left_mean_2 - configuration.line_distance_2));
			configuration.left_x_2_2 = ((configuration.left_mean_2 + configuration.line_distance_2));

			configuration.right_x_1_2 = ((configuration.right_mean_2 - configuration.line_distance_2));
			configuration.right_x_2_2 = ((configuration.right_mean_2 + configuration.line_distance_2));

			configuration.left_x_1_3 = ((configuration.left_mean_3 - configuration.line_distance_3));
			configuration.left_x_2_3 = ((configuration.left_mean_3 + configuration.line_distance_3));

			configuration.right_x_1_3 = ((configuration.right_mean_3 - configuration.line_distance_3));
			configuration.right_x_2_3 = ((configuration.right_mean_3 + configuration.line_distance_3));



			if (mean.size() == 6) {
				std::cout << seek_calib << "\n";
				std::cout << "MUIE" << "\n";
				config_file_out.seekp(seek_calib, std::ios::beg);
				config_file_out << "left_mean_1=" << (int) mean[0] << "\n";
				std::cout << mean[0] << "\n";
				std::cout << mean[2] << "\n";
				std::cout << mean[4] << "\n";
				std::cout << mean[1] << "\n";
				std::cout << mean[3] << "\n";
				std::cout << mean[5] << "\n";


				config_file_out << "left_mean_2=" << (int) mean[2] << "\n";
				config_file_out << "left_mean_3=" << (int) mean[4] << "\n";
				config_file_out << "right_mean_1=" << (int) mean[1] << "\n";
				config_file_out << "right_mean_2=" << (int) mean[3] << "\n";
				config_file_out << "right_mean_3=" << (int) mean[5] << "\n";
				configuration.left_mean_1 = (int)mean[0];
				configuration.left_mean_2 = (int)mean[2];
				configuration.left_mean_3 = (int)mean[4];
				configuration.right_mean_1 = (int)mean[1];
				configuration.right_mean_2 = (int)mean[3];
				configuration.right_mean_3 = (int)mean[5];
			}
			std::cout << "PULA" << "\n";
			config_file_out.close();

		} else {

			configuration.y_1 *= configuration.resize_factor;
			configuration.y_2 *= configuration.resize_factor;
			configuration.y_3 *= configuration.resize_factor;
			configuration.line_distance_1 *= configuration.resize_factor;
			configuration.line_distance_1_out *= configuration.resize_factor;
			configuration.line_distance_1_in *= configuration.resize_factor;
			configuration.line_distance_2 *= configuration.resize_factor;
			configuration.line_distance_3 *= configuration.resize_factor;
			configuration.left_mean_1 *= configuration.resize_factor;
			configuration.left_mean_2 *= configuration.resize_factor;
			configuration.left_mean_3 *= configuration.resize_factor;
			configuration.right_mean_1 *= configuration.resize_factor;
			configuration.right_mean_2 *= configuration.resize_factor;
			configuration.right_mean_3 *= configuration.resize_factor;

			configuration.left_x_1_1 = ((configuration.left_mean_1 - configuration.line_distance_1_out));
			configuration.left_x_2_1 = ((configuration.left_mean_1 + configuration.line_distance_1_in));

			configuration.right_x_1_1 = ((configuration.right_mean_1 - configuration.line_distance_1_in));
			configuration.right_x_2_1 = ((configuration.right_mean_1 + configuration.line_distance_1_out));

			configuration.left_x_1_2 = ((configuration.left_mean_2 - configuration.line_distance_2));
			configuration.left_x_2_2 = ((configuration.left_mean_2 + configuration.line_distance_2));

			configuration.right_x_1_2 = ((configuration.right_mean_2 - configuration.line_distance_2));
			configuration.right_x_2_2 = ((configuration.right_mean_2 + configuration.line_distance_2));

			configuration.left_x_1_3 = ((configuration.left_mean_3 - configuration.line_distance_3));
			configuration.left_x_2_3 = ((configuration.left_mean_3 + configuration.line_distance_3));

			configuration.right_x_1_3 = ((configuration.right_mean_3 - configuration.line_distance_3));
			configuration.right_x_2_3 = ((configuration.right_mean_3 + configuration.line_distance_3));
		}

	} catch (...) {
		std::cout << "INVALID CONFIGURATION FILE" << '\n';
	}

	return configuration;

}

