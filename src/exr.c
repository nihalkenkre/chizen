#include "exr.h"
#include "utils.h"
#include <openexr.h>
#include <string.h>

static inline void EXR_CHECK(const char* action, exr_result_t result)
{
	if (result > EXR_ERR_SUCCESS)
	{
		printf("EXR ERR: %d, %s\nExiting...", result, action);
		exit(result);
	}
}

void write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count)
{
	exr_context_initializer_t ctx_init = EXR_DEFAULT_CONTEXT_INITIALIZER;
	exr_context_t out = { 0 };

	EXR_CHECK("start write", exr_start_write(&out, file_path, EXR_WRITE_FILE_DIRECTLY, &ctx_init));
	int part_id = 0;
	EXR_CHECK("get count", exr_get_count(out, &part_id));

	int new_id = 0;
	EXR_CHECK("add part", exr_add_part(out, "Render Layers", EXR_STORAGE_SCANLINE, &new_id));
	EXR_CHECK("init required attr", exr_initialize_required_attr_simple(out, new_id, (int32_t)render_width, (int32_t)render_height, EXR_COMPRESSION_DWAA));

	for (size_t p = 0; p < passes_count; ++p)
	{
		if (passes[p].layer == EXR_LAYER_NORMAL)
		{
			EXR_CHECK("add normal channel R", exr_add_channel(out, part_id, "Normal.R", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
			EXR_CHECK("add normal channel G", exr_add_channel(out, part_id, "Normal.G", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
			EXR_CHECK("add normal channel B", exr_add_channel(out, part_id, "Normal.B", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
			EXR_CHECK("add normal channel A", exr_add_channel(out, part_id, "Normal.A", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
		}
		else if (passes[p].layer == EXR_LAYER_UV)
		{
			EXR_CHECK("add uv channel R", exr_add_channel(out, part_id, "UV.R", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
			EXR_CHECK("add uv channel G", exr_add_channel(out, part_id, "UV.G", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
			EXR_CHECK("add uv channel B", exr_add_channel(out, part_id, "UV.B", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
			EXR_CHECK("add uv channel A", exr_add_channel(out, part_id, "UV.A", EXR_PIXEL_FLOAT, EXR_PERCEPTUALLY_LINEAR, 1, 1));
		}
	}

	EXR_CHECK("write header", exr_write_header(out));

	int32_t chunk_count = 0;
	EXR_CHECK("get chunk count", exr_get_chunk_count(out, part_id, &chunk_count));

	int32_t scanlines_per_chunk = 0;
	EXR_CHECK("get scanlines per chunk", exr_get_scanlines_per_chunk(out, part_id, &scanlines_per_chunk));

	exr_chunk_info_t chunk_info = { 0 };
	exr_encode_pipeline_t encoder = { 0 };
	bool first = true;

	for (int32_t chunk_id = 0; chunk_id < render_height; chunk_id += scanlines_per_chunk)
	{
		EXR_CHECK("write scanline chunk info", exr_write_scanline_chunk_info(out, part_id, chunk_id, &chunk_info));

		if (first)
		{
			EXR_CHECK("init encoding", exr_encoding_initialize(out, part_id, &chunk_info, &encoder));
		}
		else
		{
			EXR_CHECK("update encoding", exr_encoding_update(out, part_id, &chunk_info, &encoder));
		}

		for (int16_t c = 0; c < encoder.channel_count; ++c)
		{
			if (strcmp(encoder.channels[c].channel_name, "Normal.R") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_NORMAL)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "Normal.G") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_NORMAL)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 1)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "Normal.B") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_NORMAL)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 2)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "Normal.A") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_NORMAL)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 3)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "UV.R") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_UV)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "UV.G") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_UV)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 1)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "UV.B") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_UV)
					{
						encoder.channels[c].encode_from_ptr = (uint8_t*)(&passes[p].pixels[(chunk_info.start_y * render_width * 4) + (chunk_info.start_x * 4 + 2)]);
					}
				}
			}
			else if (strcmp(encoder.channels[c].channel_name, "UV.A") == 0)
			{
				for (size_t p = 0; p < passes_count; ++p)
				{
					if (passes[p].layer == EXR_LAYER_UV)
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
			EXR_CHECK("choose default encoding routines", exr_encoding_choose_default_routines(out, part_id, &encoder));

		EXR_CHECK("encoding run", exr_encoding_run(out, part_id, &encoder));
		first = false;
	}

	EXR_CHECK("encoder destroy", exr_encoding_destroy(out, &encoder));
	EXR_CHECK("finish", exr_finish(&out));
}
