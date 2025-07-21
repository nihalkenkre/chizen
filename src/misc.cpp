#include "misc.hpp"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfHeader.h>
#include <ImfFrameBuffer.h>

void write_exr(const char* file_path, const size_t render_width, const size_t render_height, float* pixels)
{
	Imf_3_4::Header header((int)render_width, (int)render_height);
	header.channels().insert("R", Imf_3_4::Channel(Imf_3_4::FLOAT));
	header.channels().insert("G", Imf_3_4::Channel(Imf_3_4::FLOAT));
	header.channels().insert("B", Imf_3_4::Channel(Imf_3_4::FLOAT));
	header.channels().insert("A", Imf_3_4::Channel(Imf_3_4::FLOAT));

	Imf_3_4::FrameBuffer framebuffer;

	framebuffer.insert("R", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(pixels), sizeof(float) * 4, sizeof(float) * 4 * render_width));
	framebuffer.insert("G", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(pixels) + sizeof(float), sizeof(float) * 4, sizeof(float) * 4 * render_width));
	framebuffer.insert("B", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(pixels) + (sizeof(float) * 2), sizeof(float) * 4, sizeof(float) * 4 * render_width));
	framebuffer.insert("A", Imf_3_4::Slice(Imf_3_4::FLOAT, reinterpret_cast<char*>(pixels) + (sizeof(float) * 3), sizeof(float) * 4, sizeof(float) * 4 * render_width));

	Imf_3_4::OutputFile output_file(file_path, header);
	output_file.setFrameBuffer(framebuffer);
	output_file.writePixels((int)render_height);
}
