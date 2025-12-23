#pragma once

class SWRasterizerScene;

class SWRasterizer
{
public:
	SWRasterizer() {}

	SWRasterizer(const SWRasterizer& other) = delete;
	SWRasterizer& operator=(const SWRasterizer& other) = delete;

	void Start(const SWRasterizerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, const uint32_t cam_index, float* pixels);
	void Stop();

	~SWRasterizer() noexcept;

	class Point
	{
	public:
		Point(glm::vec3 pos, glm::vec3 col) : mPosition(pos), mColor(col)
		{

		}

		void Draw(float* pixels, const size_t width, const size_t height) const
		{
			size_t pixel_offset = (std::lroundf(mPosition.y) * width + std::lroundf(mPosition.x)) * 4;

			pixels[pixel_offset] = static_cast<uint8_t>(mColor.b);
			pixels[pixel_offset + 1] = static_cast<uint8_t>(mColor.g);
			pixels[pixel_offset + 2] = static_cast<uint8_t>(mColor.r);
			pixels[pixel_offset + 3] = 1;// static_cast<uint8_t>(mColor.r);
		}

		glm::vec3 mPosition = glm::vec3(0.f);
		glm::vec3 mColor = glm::vec3(1.f);
	};

	class Line
	{
	public:
		Line() {}
		Line(Point p0, Point p1)
		{
			{
				if (abs(p1.mPosition.x - p0.mPosition.x) >= abs(p1.mPosition.y - p0.mPosition.y))
				{
					if (p1.mPosition.x < p0.mPosition.x)
					{
						Point tmp = p1;

						p1 = p0;
						p0 = tmp;
					}

					mPoints.reserve(std::lroundf(p1.mPosition.x) - std::lroundf(p0.mPosition.x));

					for (float x = p0.mPosition.x; x < p1.mPosition.x; ++x)
					{
						float t = (x - p0.mPosition.x) / (p1.mPosition.x - p0.mPosition.x);

						mPoints.push_back(
							Point(
								p0.mPosition + (t * (p1.mPosition - p0.mPosition)),
								p0.mColor + (t * (p1.mColor - p0.mColor))
							)
						);
					}
				}
				else
				{
					if (p1.mPosition.y < p0.mPosition.y)
					{
						Point tmp = p1;

						p1 = p0;
						p0 = tmp;
					}

					mPoints.reserve(std::lroundf(p1.mPosition.y) - std::lroundf(p0.mPosition.y));

					for (float y = p0.mPosition.y; y < p1.mPosition.y; ++y)
					{
						float t = (y - p0.mPosition.y) / (p1.mPosition.y - p0.mPosition.y);

						mPoints.push_back(
							Point(
								p0.mPosition + (t * (p1.mPosition - p0.mPosition)),
								p0.mColor + (t * (p1.mColor - p0.mColor))
							)
						);
					}
				}
			}
		}
		void Draw(float* pixels, const size_t width, const size_t height) const
		{
			for (const auto& point : mPoints)
			{
				point.Draw(pixels, width, height);
			}
		}
		std::vector<Point> mPoints;
	};

	class Triangle
	{
	public:
		Triangle(const Point _p0, const Point _p1, const Point _p2, const size_t width, const size_t height)
		{
			Point p0(glm::vec3((_p0.mPosition.x + 1.f * 0.5f) * width, (_p0.mPosition.y + 1.f * 0.5f) * height, (_p0.mPosition.z + 1.f * 0.5f)), glm::vec3(1.f));
			Point p1(glm::vec3((_p1.mPosition.x + 1.f * 0.5f) * width, (_p1.mPosition.y + 1.f * 0.5f) * height, (_p1.mPosition.z + 1.f * 0.5f)), glm::vec3(1.f));
			Point p2(glm::vec3((_p2.mPosition.x + 1.f * 0.5f) * width, (_p2.mPosition.y + 1.f * 0.5f) * height, (_p2.mPosition.z + 1.f * 0.5f)), glm::vec3(1.f));

			mLines[0] = Line(p0, p1);
			mLines[1] = Line(p2, p1);
			mLines[2] = Line(p0, p2);
		}

		void Draw(float* pixels, const size_t width, const size_t height) const
		{
			mLines[0].Draw(pixels, width, height);
			mLines[1].Draw(pixels, width, height);
			mLines[2].Draw(pixels, width, height);
		}

		Line mLines[3];
	};

private:
	bool mStopRendering = false;
	std::vector<std::vector<Triangle>> mTriangles;
};
