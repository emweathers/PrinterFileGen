#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
using std::fstream;
using std::vector;



#define VERSION 516

#define PX_SIZE_UM 17
#define EXPOSURE_TIME_SEC 10
#define WIDTH 9024
#define HEIGHT 5120

#define GREY_COUNT 16


using std::string;
using std::fstream;

//typedef struct bmp {
//	int width, height;
//} bmp;



typedef struct RGB565 {
	unsigned short R : 5;
	unsigned short G : 6;
	unsigned short B : 5;
} RGB565;

typedef struct layer {
	unsigned int data_addr; // Calculated
	unsigned int data_size; // Calculated
	float lift_height;
	float lift_speed;
	float exposure_time; // Parameter
	float layer_height;
	unsigned int nonzero_pixel_count; // Calculated; does it matter?
	const unsigned int padding = 0;
} layer;

typedef struct Mono4RLE {
	unsigned short length : 12;
	unsigned short color : 4;
} Mono4RLE;

#include "BMP.h"








// Little-Endian system for numbers
struct addr_header {
	const char brand_text[12] = "ANYCUBIC";
	const unsigned int version = VERSION;
	const unsigned int table_quantity = 8;
	const unsigned int header_table_addr = 0x34;
	const unsigned int software_table_addr = 0x0;
	const unsigned int preview_table_addr = 0x98;
	unsigned int layer_image_color_table_addr; // Calculated
	unsigned int layer_definition_addr; // Calculated
	unsigned int extra_table_addr; // Calculated
	unsigned int machine_table_addr; // Calculated
	unsigned int layer_start_addr; // Calculated
} addr_header;

struct header {
	const char header_text[12] = "HEADER";
	const unsigned int header_table_size = 84;
	float pixel_size = PX_SIZE_UM; // μm
	float layer_thickness = 1; // mm;																Requires Testing for other params; Must be greater than 0, leq to 1
	float exposure_time = EXPOSURE_TIME_SEC; // Seconds; Parameter
	const float off_time = -7; // Seconds; -7 is as best as it can get
	float bottom_exposure_time = 0; // Seconds; Parameter
	const float bottom_layer_count = 0; // Number of layers to expose for the bottom exposure time
	const float lift_distance = 0; // mm;																Requires Testing
	const float lift_speed = 100; // mm/s;																Requires Testing
	float retract_speed = 100; // mm/s;																	May Require Testing
	float volume = 0; // mL
	const unsigned int anti_aliasing_setting = 1; // 1, 2, 4, 8, or 16
	unsigned int resolution_x = WIDTH; // Pixels; Parameter
	unsigned int resolution_y = HEIGHT; // Pixels; Parameter
	const float weight = 0; // Grams
	const float price = -1;
	const char32_t currency_symbol = '$';
	unsigned int per_layer_override = 1; // 4-Byte Boolean;												Requires Testing
	const unsigned int estimated_time = -1; // Seconds
	const unsigned int transition_layer_count = 0;
	const unsigned int transition_layer_type = 0;
	const unsigned int advanced_mode = 1; // 4-Byte Aligned Boolean; Set to Basic Mode;					Requires Testing for other params (in EXTRA)
} header;

struct preview {
	const char preview_text[12] = "PREVIEW";
	unsigned int preview_table_size; // Calculated
	unsigned int preview_width; // Dependent on Provided Image; 224
	char32_t multiply_char = 'x';
	unsigned int preview_height; // Dependent on Provided Image; 168
	vector<RGB565> image; // Dependent on Provided Image
	// 75264/2
} preview;

struct grey_table {
	const unsigned int use_greyscale = 0; // 4-Byte Boolean
	unsigned int grey_max_count = GREY_COUNT; // Between 0 and 16; Maximum number of different grey colors; capped at 16
	unsigned char greys[GREY_COUNT];
	const unsigned int unknown = 0;
} grey_table;

struct layerdef {
	const char layer_definition_text[12] = "LAYERDEF";
	unsigned int layer_definition_table_size; // Calculated
	unsigned int layer_count; // Dependent on Provided Mask
	vector<layer> layer_defs;
} layerdef;

struct extra {
	const char extra_text[12] = "EXTRA";
	const unsigned int extra_table_size = 24; // Size per Lift Count Section
	const unsigned int bottom_lift_count = 2;
	float bottom_lift_height1 = 0;
	float bottom_lift_speed1 = 1;
	float bottom_retract_speed1 = 1;
	float bottom_lift_height2 = 0;
	float bottom_lift_speed2 = 1;
	float bottom_retract_speed2 = 1;
	const unsigned int normal_lift_count = 2;
	float lift_height1 = 1;
	float lift_speed1 = 8;
	float retract_speed1 = 8;
	float lift_height2 = 3;
	float lift_speed2 = 24;
	float retract_speed2 = 24;
} extra;

struct machine {
	const char machine_text[12] = "MACHINE";
	unsigned int machine_table_size;
	const char machine_name[96] = "Custom";
	const char file_format[16] = "pw0Img";
	const unsigned int max_anti_aliasing_level = 0x10;
	unsigned int property_fields;
	float display_width_mm = 80;
	float display_height_mm = 130;
	float machine_height_mm = 165;
	const unsigned int max_version = VERSION;
	const unsigned int machine_background = 0x634701;
} machine;

vector<vector<Mono4RLE>> layers;

std::string project_name;
vector<float> exposure_times;


int fileWrite() {
	//std::ofstream output_file("D:/output.pm4n", std::ios::binary);
	std::string output_file_name = project_name + ".pm4n";
	std::ofstream output_file(output_file_name, std::ios::binary);
	layerdef.layer_count = layers.size();

	preview.preview_width = 224;
	preview.preview_height = 168;
	preview.preview_table_size = 12 + 4 * 4 + 2 * (preview.preview_width * preview.preview_height); // Text size + 4 * 4-Byte Values (this value, value for w, multiplication symbol, value for h) + Number of bytes per pixel * Total pixel count
	layerdef.layer_definition_table_size = 4 + layerdef.layer_count * (8 * 4); // 4: Layer count; layer_count * (8 * 4): Layer control data
	machine.machine_table_size = 12 + 4 + 96 + 16 + 7 * 4; // Text size + size def size + name size + file format name size + 7 * other entry size

	addr_header.layer_image_color_table_addr = addr_header.preview_table_addr + preview.preview_table_size;
	addr_header.layer_definition_addr = addr_header.layer_image_color_table_addr + 2 * 4 + GREY_COUNT + 1 * 4; // 1 * 4: use_greyscale; 1 * 4: grey_max_count; 1 * 4: unknown
	addr_header.extra_table_addr = addr_header.layer_definition_addr + 12 + 4 + layerdef.layer_definition_table_size; // Text size + table size definition + established table size
	addr_header.machine_table_addr = 12 + 4 + addr_header.extra_table_addr + extra.extra_table_size * 2 + 2 * 4; // Text size + size def size + lift count section size * number of lift sections + count def size * number of sections
	addr_header.layer_start_addr = addr_header.machine_table_addr + machine.machine_table_size;

	unsigned int current_layer_pos = addr_header.layer_start_addr;


	for (int i = 0; i < layerdef.layer_count; i++) {
		layer temp = {
			current_layer_pos,
			2*layers[i].size(),
			0,
			0,
			exposure_times[i],
			0,
			0, // Non-zero px count
			0 // Padding
		};

		layerdef.layer_defs.push_back(temp);

		current_layer_pos += 2*layers[i].size();
	};




	if (output_file.is_open()) {
		output_file.write(addr_header.brand_text, 12);
		output_file.write(reinterpret_cast<const char*>(&addr_header.version), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.table_quantity), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.header_table_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.software_table_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.preview_table_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.layer_image_color_table_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.layer_definition_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.extra_table_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.machine_table_addr), 4);
		output_file.write(reinterpret_cast<const char*>(&addr_header.layer_start_addr), 4);

		output_file.write(header.header_text, 12);
		output_file.write(reinterpret_cast<const char*>(&header.header_table_size), 4);
		output_file.write(reinterpret_cast<const char*>(&header.pixel_size), 4);
		output_file.write(reinterpret_cast<const char*>(&header.layer_thickness), 4);
		output_file.write(reinterpret_cast<const char*>(&header.exposure_time), 4);
		output_file.write(reinterpret_cast<const char*>(&header.off_time), 4);
		output_file.write(reinterpret_cast<const char*>(&header.bottom_exposure_time), 4);
		output_file.write(reinterpret_cast<const char*>(&header.bottom_layer_count), 4);
		output_file.write(reinterpret_cast<const char*>(&header.lift_distance), 4);
		output_file.write(reinterpret_cast<const char*>(&header.lift_speed), 4);
		output_file.write(reinterpret_cast<const char*>(&header.retract_speed), 4);
		output_file.write(reinterpret_cast<const char*>(&header.volume), 4);
		output_file.write(reinterpret_cast<const char*>(&header.anti_aliasing_setting), 4);
		output_file.write(reinterpret_cast<const char*>(&header.resolution_x), 4);
		output_file.write(reinterpret_cast<const char*>(&header.resolution_y), 4);
		output_file.write(reinterpret_cast<const char*>(&header.weight), 4);
		output_file.write(reinterpret_cast<const char*>(&header.price), 4);
		output_file.write(reinterpret_cast<const char*>(&header.currency_symbol), 4);
		output_file.write(reinterpret_cast<const char*>(&header.per_layer_override), 4);
		output_file.write(reinterpret_cast<const char*>(&header.estimated_time), 4);
		output_file.write(reinterpret_cast<const char*>(&header.transition_layer_count), 4);
		output_file.write(reinterpret_cast<const char*>(&header.transition_layer_type), 4);
		output_file.write(reinterpret_cast<const char*>(&header.advanced_mode), 4);

		output_file.write(preview.preview_text, 12);
		output_file.write(reinterpret_cast<const char*>(&preview.preview_table_size), 4);
		output_file.write(reinterpret_cast<const char*>(&preview.preview_width), 4);
		output_file.write(reinterpret_cast<const char*>(&preview.multiply_char), 4);
		output_file.write(reinterpret_cast<const char*>(&preview.preview_height), 4);
		for (RGB565 pixel : preview.image) {
			output_file.write(reinterpret_cast<const char*>(&pixel), 2);
		}

		output_file.write(reinterpret_cast<const char*>(&grey_table.use_greyscale), 4);
		output_file.write(reinterpret_cast<const char*>(&grey_table.grey_max_count), 4);
		output_file.write(reinterpret_cast<const char*>(&grey_table.greys), GREY_COUNT);
		output_file.write(reinterpret_cast<const char*>(&grey_table.unknown), 4);
		
		output_file.write(layerdef.layer_definition_text, 12);
		output_file.write(reinterpret_cast<const char*>(&layerdef.layer_definition_table_size), 4);
		output_file.write(reinterpret_cast<const char*>(&layerdef.layer_count), 4);
		for (layer l : layerdef.layer_defs) {
			output_file.write(reinterpret_cast<const char*>(&l.data_addr), 4);
			output_file.write(reinterpret_cast<const char*>(&l.data_size), 4);
			output_file.write(reinterpret_cast<const char*>(&l.lift_height), 4);
			output_file.write(reinterpret_cast<const char*>(&l.lift_speed), 4);
			output_file.write(reinterpret_cast<const char*>(&l.exposure_time), 4);
			output_file.write(reinterpret_cast<const char*>(&l.layer_height), 4);
			output_file.write(reinterpret_cast<const char*>(&l.nonzero_pixel_count), 4);
			output_file.write(reinterpret_cast<const char*>(&l.padding), 4);
		}

		output_file.write(extra.extra_text, 12);
		output_file.write(reinterpret_cast<const char*>(&extra.extra_table_size), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_lift_count), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_lift_height1), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_lift_speed1), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_retract_speed1), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_lift_height2), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_lift_speed2), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.bottom_retract_speed2), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.normal_lift_count), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.lift_height1), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.lift_speed1), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.retract_speed1), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.lift_height2), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.lift_speed2), 4);
		output_file.write(reinterpret_cast<const char*>(&extra.retract_speed2), 4);

		output_file.write(machine.machine_text, 12);
		output_file.write(reinterpret_cast<const char*>(&machine.machine_table_size), 4);
		output_file.write(machine.machine_name, 96);
		output_file.write(machine.file_format, 16);
		output_file.write(reinterpret_cast<const char*>(&machine.max_anti_aliasing_level), 4);
		output_file.write(reinterpret_cast<const char*>(&machine.property_fields), 4);
		output_file.write(reinterpret_cast<const char*>(&machine.display_width_mm), 4);
		output_file.write(reinterpret_cast<const char*>(&machine.display_height_mm), 4);
		output_file.write(reinterpret_cast<const char*>(&machine.machine_height_mm), 4);
		output_file.write(reinterpret_cast<const char*>(&machine.max_version), 4);
		output_file.write(reinterpret_cast<const char*>(&machine.machine_background), 4);

		for (vector<Mono4RLE> layer : layers) {
			for (Mono4RLE pixel : layer) {
				unsigned short* s = reinterpret_cast<unsigned short*>(&pixel);
				unsigned short s_new = ((*s & 0x00FF) << 8) | ((*s & 0xFF00) >> 8);
				output_file.write(reinterpret_cast<const char*>(&s_new), 2);
			}
		}

		output_file.close();
	}
	else {
		std::cout << "\33[31mError: Could not open output file\33[0m\n";
		return -1;
	}


	return 0;
}





int main() {
#ifdef _WIN32 || _WIN64
	std::system("echo %CD% > \"temp\"");
	std::system("cd masks && dir /b /a >> ../\"temp\"");
#elif __linux__ || __FreeBSD__ || __unix || __unix__

#endif
	
	std::ifstream temp("temp");
	std::string line;

	std::getline(temp,line);
	project_name = line.substr(line.find_last_of('\\') + 1, -1);
	std::cout << project_name << std::endl;

	preview.image = BMP::read("preview.bmp").toRGB565();

	while (std::getline(temp, line)) {
		exposure_times.push_back(std::stoi(line.substr(line.find_first_of('.') + 1, line.find_last_of('.') - (line.find_first_of('.') + 1))));
		std::string imgpath = "masks/" + line;
		layers.push_back(BMP::read(imgpath.c_str()).toMono4RLE());
	}

	//layers.push_back(BMP::read("input/masks/input.bmp").toMono4RLE());
	//layers.push_back(BMP::read("input/masks/printer_test_mask.bmp").toMono4RLE());

	fileWrite();

	//printf("%i\n",rletest.header.bits_per_pixel);


	/*for (RGB565 a : preview) {
		unsigned short* c = reinterpret_cast<unsigned short*>(&a);
		printf("%.4x", ((*c & 0xFF00) >> 8) | ((*c & 0x00FF) << 8));
	}*/

	/*for (int i = 0; i < rletest.size(); i++) {
		printf("%.4x", rletest[i]);
	}*/

	return 0;
}