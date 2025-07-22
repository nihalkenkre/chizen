#include "misc.hpp"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfHeader.h>
#include <ImfFrameBuffer.h>

void write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count)
{
	Imf_3_4::Header header((int)render_width, (int)render_height);
	Imf_3_4::FrameBuffer framebuffer;

	for (size_t p = 0; p < passes_count; ++p)
	{
		if (passes[p].layer == EXR_LAYER_NORMAL)
		{
			header.channels().insert("Normal.R", Imf_3_4::Channel(Imf_3_4::FLOAT));
			header.channels().insert("Normal.G", Imf_3_4::Channel(Imf_3_4::FLOAT));
			header.channels().insert("Normal.B", Imf_3_4::Channel(Imf_3_4::FLOAT));
			header.channels().insert("Normal.A", Imf_3_4::Channel(Imf_3_4::FLOAT));
			framebuffer.insert("Normal.R", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(passes[p].pixels), sizeof(float) * 4, sizeof(float) * 4 * render_width));
			framebuffer.insert("Normal.G", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(&passes[p].pixels[1]), sizeof(float) * 4, sizeof(float) * 4 * render_width));
			framebuffer.insert("Normal.B", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(&passes[p].pixels[2]), sizeof(float) * 4, sizeof(float) * 4 * render_width));
			framebuffer.insert("Normal.A", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(&passes[p].pixels[3]), sizeof(float) * 4, sizeof(float) * 4 * render_width));
		}
		else if (passes[p].layer == EXR_LAYER_UV)
		{
			header.channels().insert("UV.R", Imf_3_4::Channel(Imf_3_4::FLOAT));
			header.channels().insert("UV.G", Imf_3_4::Channel(Imf_3_4::FLOAT));
			header.channels().insert("UV.B", Imf_3_4::Channel(Imf_3_4::FLOAT));
			header.channels().insert("UV.A", Imf_3_4::Channel(Imf_3_4::FLOAT));

			framebuffer.insert("UV.R", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(passes[p].pixels), sizeof(float) * 4, sizeof(float) * 4 * render_width));
			framebuffer.insert("UV.G", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(&passes[p].pixels[1]), sizeof(float) * 4, sizeof(float) * 4 * render_width));
			framebuffer.insert("UV.B", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(&passes[p].pixels[2]), sizeof(float) * 4, sizeof(float) * 4 * render_width));
			framebuffer.insert("UV.A", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(&passes[p].pixels[3]), sizeof(float) * 4, sizeof(float) * 4 * render_width));
		}
	}

	Imf_3_4::OutputFile output_file(file_path, header);
	output_file.setFrameBuffer(framebuffer);
	output_file.writePixels((int)render_height);
}
