#include "exr.h"
#include "error.h"
#include <openexr.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

CHIZEN_RESULT write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count)
{
	CHIZEN_RESULT result = CHIZEN_RESULT_SUCCESS;

	char* channel_names[] = { ".R", ".G", ".B", ".A" };
	exr_context_initializer_t ctx_init = EXR_DEFAULT_CONTEXT_INITIALIZER;
	exr_context_t out = { 0 };
	exr_chunk_info_t chunk_info = { 0 };
	exr_encode_pipeline_t encoder = { 0 };

	EXR_CHECK("start write", exr_start_write(&out, file_path, EXR_WRITE_FILE_DIRECTLY, &ctx_init), result);
	int part_id = 0;
	EXR_CHECK("get count", exr_get_count(out, &part_id), result);

	int new_id = 0;
	EXR_CHECK("add part", exr_add_part(out, "Render Layers", EXR_STORAGE_SCANLINE, &new_id), result);
	EXR_CHECK("init attr", exr_initialize_required_attr_simple(out, new_id, (int32_t)render_width, (int32_t)render_height, EXR_COMPRESSION_DWAA), result);

	for (size_t p = 0; p < passes_count; ++p)
	{
		for (size_t c = 0; c < _countof(channel_names); ++c)
		{
			char layer_channel_name[64];
			memset(layer_channel_name, 0, 64);
			strncpy(layer_channel_name, passes[p].layer.name, strlen(passes[p].layer.name));
			strcat(layer_channel_name, channel_names[c]);
			EXR_CHECK("add channel", exr_add_channel(out, part_id, layer_channel_name, EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1), result);
		}
	}

	EXR_CHECK("write header", exr_write_header(out), result);

	int32_t chunk_count = 0;
	EXR_CHECK("get chunk count", exr_get_chunk_count(out, part_id, &chunk_count), result);

	int32_t scanlines_per_chunk = 0;
	EXR_CHECK("get scanlines per chunk", exr_get_scanlines_per_chunk(out, part_id, &scanlines_per_chunk), result);

	bool first = true;

	for (int32_t chunk_id = 0; chunk_id < render_height; chunk_id += scanlines_per_chunk)
	{
		EXR_CHECK("write scanline chunk info", exr_write_scanline_chunk_info(out, part_id, chunk_id, &chunk_info), result);

		if (first)
		{
			EXR_CHECK("init encoding", exr_encoding_initialize(out, part_id, &chunk_info, &encoder), result);
		}
		else
		{
			EXR_CHECK("update encoding", exr_encoding_update(out, part_id, &chunk_info, &encoder), result);
		}

		for (int16_t c = 0; c < encoder.channel_count; ++c)
		{
			char encoder_channel_name[64];
			strcpy(encoder_channel_name, (char*)encoder.channels[c].channel_name);

			char* exr_layer_name = strtok(encoder_channel_name, ".");

			for (size_t p = 0; p < passes_count; ++p)
			{
				if (strcmp(passes[p].layer.name, exr_layer_name) == 0)
				{
					char* exr_layer_channel_name = strtok(NULL, ".");
					if (strcmp(exr_layer_channel_name, "R") == 0)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4)]);
					}
					else if (strcmp(exr_layer_channel_name, "G") == 0)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 1)]);
					}
					else if (strcmp(exr_layer_channel_name, "B") == 0)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 2)]);
					}
					else if (strcmp(exr_layer_channel_name, "A") == 0)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 3)]);
					}
				}
			}

			encoder.channels[c].user_data_type = EXR_PIXEL_FLOAT;
			encoder.channels[c].user_pixel_stride = (int32_t)(sizeof(float) * 4);
			encoder.channels[c].user_line_stride = (int32_t)(sizeof(float) * 4 * render_width);
		}

		if (first)
			EXR_CHECK("choose default encoding routines", exr_encoding_choose_default_routines(out, part_id, &encoder), result);

		EXR_CHECK("run encoding", exr_encoding_run(out, part_id, &encoder), result);
		first = false;
	}

cpu_error:
	EXR_CHECK("destroy encoder", exr_encoding_destroy(out, &encoder), result);
	EXR_CHECK("finish", exr_finish(&out), result);

	return result;
}
