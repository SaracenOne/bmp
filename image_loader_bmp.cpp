#include "image_loader_bmp.h"

Error ImageLoaderBMP::convert_to_image(Image *p_image, const uint8_t *p_buffer, const uint8_t *p_color_buffer, const bmp_header_s &p_header) {
	Error err = OK;

	if (p_buffer == NULL)
		err = FAILED;

	if (err == OK) {
		size_t index = 0;
		size_t width = static_cast<size_t>(p_header.bmp_info_header.bmp_width < 0 ? -p_header.bmp_info_header.bmp_width : p_header.bmp_info_header.bmp_width);
		size_t height = static_cast<size_t>(p_header.bmp_info_header.bmp_height < 0 ? -p_header.bmp_info_header.bmp_height : p_header.bmp_info_header.bmp_height);
		size_t bits_per_pixel = static_cast<size_t>(p_header.bmp_info_header.bmp_bit_count);

		uint32_t line_width = ((p_header.bmp_info_header.bmp_width * p_header.bmp_info_header.bmp_bit_count / 8) + 3) & ~3;

		DVector<uint8_t> image_data;
		err = image_data.resize(width * height * 4);

		DVector<uint8_t>::Write image_data_w = image_data.write();
		uint8_t *write_buffer = image_data_w.ptr();

		if (p_header.bmp_info_header.bmp_compression == 0) {
			const uint8_t *line = p_buffer + (line_width * (height - 1));
			for (unsigned int i = 0; i < height; i++) {
				const uint8_t *line_ptr = line;
				for (unsigned int j = 0; j < width; j++) {
					if (bits_per_pixel == 24) {
						uint32_t color = *((uint32_t*)line_ptr);

						write_buffer[index + 2] = color & 0xff;
						write_buffer[index + 1] = (color >> 8) & 0xff;
						write_buffer[index + 0] = (color >> 16) & 0xff;
						write_buffer[index + 3] = 0xff;
						index += 4;
						line_ptr += 3;
					} else if (bits_per_pixel == 32) {
						uint32_t color = *((uint32_t*)line_ptr);
						write_buffer[index + 2] = color & 0xff;
						write_buffer[index + 1] = (color >> 8) & 0xff;
						write_buffer[index + 0] = (color >> 16) & 0xff;
						write_buffer[index + 3] = color >> 24;
						index += 4;
						line_ptr += 4;
					} else {
						err = FAILED;
						return err;
					}
				}
				line -= line_width;
			}
			p_image->create(width, height, 0, Image::FORMAT_RGBA, image_data);
		} else {
			err = FAILED;
		}
	}
	return err;
}

Error ImageLoaderBMP::load_image(Image *p_image, FileAccess *f) {

	bmp_header_s bmp_header;
	Error err = ERR_INVALID_DATA;

	if (f->get_len() > sizeof(bmp_header)) {
		// File Header
		bmp_header.bmp_file_header.bmp_signature = f->get_16();
		if (bmp_header.bmp_file_header.bmp_signature == BITMAP_SIGNATURE) {
			bmp_header.bmp_file_header.bmp_file_size = f->get_32();
			bmp_header.bmp_file_header.bmp_file_padding = f->get_32();
			bmp_header.bmp_file_header.bmp_file_offset = f->get_32();

			// Info Header
			bmp_header.bmp_info_header.bmp_header_size = f->get_32();
			bmp_header.bmp_info_header.bmp_width = f->get_32();
			bmp_header.bmp_info_header.bmp_height = f->get_32();
			bmp_header.bmp_info_header.bmp_planes = f->get_16();
			bmp_header.bmp_info_header.bmp_bit_count = f->get_16();
			bmp_header.bmp_info_header.bmp_compression = f->get_32();
			bmp_header.bmp_info_header.bmp_size_image = f->get_32();
			bmp_header.bmp_info_header.bmp_pixels_per_meter_x = f->get_32();
			bmp_header.bmp_info_header.bmp_pixels_per_meter_y = f->get_32();
			bmp_header.bmp_info_header.bmp_colors_used = f->get_32();
			bmp_header.bmp_info_header.bmp_important_colors = f->get_32();

			bmp_header.bmp_info_header.bmp_red_mask = f->get_32();
			bmp_header.bmp_info_header.bmp_green_mask = f->get_32();
			bmp_header.bmp_info_header.bmp_blue_mask = f->get_32();
			bmp_header.bmp_info_header.bmp_alpha_mask = f->get_32();
			bmp_header.bmp_info_header.bmp_cs_type = f->get_32();
			for (int i = 0; i < 9; i++)
				bmp_header.bmp_info_header.bmp_endpoints[i] = f->get_32();

			bmp_header.bmp_info_header.bmp_gamma_red = f->get_32();
			bmp_header.bmp_info_header.bmp_gamma_green = f->get_32();
			bmp_header.bmp_info_header.bmp_gamma_blue = f->get_32();

			f->seek(sizeof(bmp_header.bmp_file_header) + bmp_header.bmp_info_header.bmp_header_size);

			uint32_t color_table_size = 0;
			if (bmp_header.bmp_info_header.bmp_bit_count == 1)
				color_table_size = 2;
			else if (bmp_header.bmp_info_header.bmp_bit_count == 4)
				color_table_size = 16;
			else if (bmp_header.bmp_info_header.bmp_bit_count == 8)
				color_table_size = 256;

			DVector<uint8_t> bmp_color_table;
			if (color_table_size > 0) {
				err = bmp_color_table.resize(color_table_size * 4);
				DVector<uint8_t>::Write bmp_color_table_w = bmp_color_table.write();
				f->get_buffer(bmp_color_table_w.ptr(), bmp_header.bmp_info_header.bmp_colors_used * 4);
			}

			f->seek(bmp_header.bmp_file_header.bmp_file_offset);

			uint32_t bmp_buffer_size = (bmp_header.bmp_file_header.bmp_file_size - bmp_header.bmp_file_header.bmp_file_offset);

			DVector<uint8_t> bmp_buffer;
			err = bmp_buffer.resize(bmp_buffer_size);
			if (err == OK) {
				DVector<uint8_t>::Write bmp_buffer_w = bmp_buffer.write();
				f->get_buffer(bmp_buffer_w.ptr(), bmp_buffer_size);


				DVector<uint8_t>::Read bmp_buffer_r = bmp_buffer.read();
				DVector<uint8_t>::Read bmp_color_table_r = bmp_color_table.read();
				err = convert_to_image(p_image, bmp_buffer_r.ptr(), bmp_color_table_r.ptr(), bmp_header);
			}
			f->close();
		}
	}

	return err;
}

void ImageLoaderBMP::get_recognized_extensions(List<String> *p_extensions) const {
	
	p_extensions->push_back("bmp");
}

ImageLoaderBMP::ImageLoaderBMP() {
}


