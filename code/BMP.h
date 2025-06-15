#pragma once

class BMP {
private:
	typedef struct color {
		uint32_t B : 8;
		uint32_t G : 8;
		uint32_t R : 8;
	};

#pragma pack(push,1)
	typedef struct BMPHeader {
		uint8_t id[2];
		uint32_t file_size;
		uint16_t reserved0;
		uint16_t reserved1;
		uint32_t addr_start;

		uint32_t header_size;
		uint32_t bmp_width;
		uint32_t bmp_height;
		uint16_t color_plane_count;
		uint16_t bits_per_pixel;
		uint32_t compression_method;
		uint32_t image_size;
		int32_t pixel_per_meter_w;
		int32_t pixel_per_meter_h;
		uint32_t colors_in_palette;
		uint32_t important_color_count;

	} BMPHeader;
#pragma pack(pop)
#define BMP_HEADER_SIZE 54 // General BMP header size

public:
	string file_name;
	BMPHeader header;
	vector<color> pixels;

	vector<RGB565> toRGB565() {
		std::cout << "Generating color table of file \"" << this->file_name << "\" in RGB565 format...\n";
		vector<RGB565> out(pixels.size());

		for (int i = 0; i < pixels.size(); i++) {
			out[i].R = (pixels[i].R)/8;
			out[i].G = (pixels[i].G)/4;
			out[i].B = (pixels[i].B)/8;
		}
		std::cout << "Table generation complete!\n\n";

		return out;
	}

	vector<Mono4RLE> toMono4RLE() {
		std::cout << "Encoding file \"" << this->file_name << "\" to 4-bit color RLE format...\n";
		vector<Mono4RLE> out;

		unsigned short count = 1;
		for (int i = 0; i < pixels.size(); i++) {
			color px = pixels[i];
			int coloravg = (((px.R >> 4) & 0xF) + ((px.R) & 0xF) + ((px.G >> 4) & 0xF) + ((px.G) & 0xF) + ((px.B >> 4) & 0xF) + ((px.B) & 0xF)) / 6;

			if (((i + 1) < pixels.size() && (pixels[i + 1].R != pixels[i].R || pixels[i + 1].G != pixels[i].G || pixels[i + 1].B != pixels[i].B)) || (i + 1) >= pixels.size() || (count + 1) > 0x0FFF) {
				Mono4RLE temp = { count,coloravg };
				out.push_back(temp);
				count = 0;
			}
			count++;
		}
		std::cout << "Encoding complete!\n\n";

		return out;
	}

	static BMP read(const char* file_name) {
		std::ifstream input_file(file_name, std::ios::binary); // Open file as binary

		BMPHeader header;
		input_file.read(reinterpret_cast<char*>(&header), BMP_HEADER_SIZE); // Read up to then end of the header

		vector<color> color_table(sizeof(uint32_t) * header.important_color_count); // Initialize the color table vector with the size of the table
		for (int i = 0; i < header.important_color_count; i++) {
			color temp = { input_file.get(), input_file.get(), input_file.get()}; // Little-Endian
			input_file.get(); // Skip the "reserved" byte, as it has no use
			color_table[i] = temp; // Add new color to color table
		}

		int position_count = header.file_size - header.addr_start; // Number of bytes in pixel map
		// printf("%i\n", position_count);
		// printf("%i\n", (int)ceil(((float)header.bmp_width) / 2));
		// printf("%i\n", 4 - (header.bmp_width / 2) % 4);


		// Generate Offsets:
		// Bitmaps try to keep each row to have the width as a multiple of 4, so it adds a padding, so an offset is necessary to counteract the padding
		uint8_t bpp1_offset = 4 - ((int)ceil(((float)header.bmp_width) / 8)) % 4;
		bpp1_offset = bpp1_offset == 4 ? 0 : bpp1_offset;
		
		uint8_t bpp4_offset = 4 - ((int)ceil(((float)header.bmp_width) / 2)) % 4;
		bpp4_offset = bpp4_offset == 4 ? 0 : bpp4_offset;

		uint8_t bpp8_offset = 4 - header.bmp_width % 4;
		bpp8_offset = bpp8_offset == 4 ? 0 : bpp8_offset;

		uint8_t bppx_offset = 4 - 3*header.bmp_width % 4;
		bppx_offset = bppx_offset == 4 ? 0 : bppx_offset;

		// TODO: Allocate vectors while taking into account the offset value
		std::cout << "Allocating memory to store image data for image \"" << file_name << "\"...\n";
		vector<color> image_data;
		if (header.bits_per_pixel == 1) image_data = vector<color>(position_count * 8); // Each byte in the pixel map contains 2 pixels
		else if (header.bits_per_pixel == 4) image_data = vector<color>(position_count * 2); // Each byte in the pixel map contains 2 pixels
		else if (header.bits_per_pixel == 8) image_data = vector<color>(position_count); // Each byte in the pixel map contains 1 pixel
		else image_data = vector<color>((position_count) / 3); // Each pixel takes up 3 bytes
		std::cout << "Allocation complete!\n\n";


		if (header.important_color_count > 0) { // If the color table exists
			if (header.bits_per_pixel == 1) {
				for (int i = 0; i < header.bmp_height; i++) {
					input_file.seekg(header.file_size - ((i + 1) * ((header.bmp_width / 8) + bpp1_offset))); // Start at the top of the image and work down
					for (int j = 0; j < header.bmp_width; j += 8) { // The image width is half the number of bytes, so increment twice
						unsigned int pixel = input_file.get();
						image_data[i * header.bmp_width + j] = color_table[(pixel & 0x80) >> 7];
						if (j + 1 <= header.bmp_width) image_data[i * header.bmp_width + j + 1] = color_table[(pixel & 0x40) >> 6];
						if (j + 2 <= header.bmp_width) image_data[i * header.bmp_width + j + 2] = color_table[(pixel & 0x20) >> 5];
						if (j + 3 <= header.bmp_width) image_data[i * header.bmp_width + j + 3] = color_table[(pixel & 0x10) >> 4];
						if (j + 4 <= header.bmp_width) image_data[i * header.bmp_width + j + 4] = color_table[(pixel & 0x08) >> 3];
						if (j + 5 <= header.bmp_width) image_data[i * header.bmp_width + j + 5] = color_table[(pixel & 0x04) >> 2];
						if (j + 6 <= header.bmp_width) image_data[i * header.bmp_width + j + 6] = color_table[(pixel & 0x02) >> 1];
						if (j + 7 <= header.bmp_width) image_data[i * header.bmp_width + j + 7] = color_table[(pixel & 0x01)];
					}
				}
			}
			else if (header.bits_per_pixel == 4) {
				for (int i = 0; i < header.bmp_height; i++) {
					input_file.seekg(header.file_size - ((i + 1) * ((header.bmp_width / 2) + bpp4_offset))); // Start at the top of the image and work down
					for (int j = 0; j < header.bmp_width; j += 2) { // The image width is half the number of bytes, so increment twice
						unsigned int pixel = input_file.get();
						image_data[i * header.bmp_width + j] = color_table[(pixel & 0xF0) >> 4]; // On even numbered pixels, apply bitmask, shift, and read pixel value
						if (j + 1 <= header.bmp_width) image_data[i * header.bmp_width + j + 1] = color_table[pixel & 0x0F]; // Same as above for odd, but no shift, and potential for it to not exist
					}
				}
			}
			else if (header.bits_per_pixel == 8) {
				for (int i = 0; i < header.bmp_height; i++) {
					input_file.seekg(header.file_size - ((i + 1) * (header.bmp_width + bpp8_offset))); // Start at the top of the image and work down
					for (int j = 0; j < header.bmp_width; j++) {
						unsigned int pixel = input_file.get();
						image_data[i * header.bmp_width + j] = color_table[pixel]; // Read the pixel referencing its position in the color table as the key for the color
					}
				}
			}
		}
		else {
			for (int i = 0; i < header.bmp_height; i++) {
				input_file.seekg(header.file_size - ((i + 1) * ((header.bmp_width * 3) + bppx_offset))); // Start at the top of the image and work down
				for (int j = 0; j < header.bmp_width; j++) {
					unsigned int pixel0 = input_file.get(); // Byte for B
					unsigned int pixel1 = input_file.get(); // Byte for G
					unsigned int pixel2 = input_file.get(); // Byte for R
					image_data[i * header.bmp_width + j] = { pixel0, pixel1, pixel2 }; // Paste in B, G, R; Little-Endian
				}
			}
		}

		// int idx = 1512*9024 + 2142;
		// printf("%.2x%.2x%.2x\n", image_data[idx].R, image_data[idx].G, image_data[idx].B);

		input_file.close(); // Close the file
		return { file_name, header, image_data };
	}


};
