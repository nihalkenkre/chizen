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

	class BoundingBox;
	class Triangle;

	class Point
	{
	public:
		Point() {}
		Point(glm::vec3 pos, glm::vec3 col) : mPosition(pos), mColor(col)
		{
		}

		void Draw(float* color, float* depth, const size_t width, const size_t height) const
		{
			if (std::lroundf(mPosition.y) >= height || std::lroundf(mPosition.x) >= width)
				return;

			size_t pixel_offset = (std::lroundf(mPosition.y) * width + std::lroundf(mPosition.x)) * 4;
			size_t depth_offset = std::lroundf(mPosition.y) * width + std::lroundf(mPosition.x);

			if (mPosition.z > depth[depth_offset]) {
				color[pixel_offset] = mPosition.z;// mColor.b;
				color[pixel_offset + 1] = mPosition.z;//  mColor.g;
				color[pixel_offset + 2] = mPosition.z;//  mColor.r;
				color[pixel_offset + 3] = 1;
				depth[depth_offset] = mPosition.z;
			}
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

		void Draw(float* color, float* depth, const size_t width, const size_t height) const
		{
			for (const auto& point : mPoints)
			{
				point.Draw(color, depth, width, height);
			}
		}

	private:
		std::vector<Point> mPoints;
	};

	class Triangle;

	class BoundingBox
	{
	public:
		BoundingBox() {}
		BoundingBox(const Point& p0, const Point& p1, const Point& p2)
		{
			mMin.mPosition.x = std::min(std::min(p0.mPosition.x, p1.mPosition.x), p2.mPosition.x);
			mMin.mPosition.y = std::min(std::min(p0.mPosition.y, p1.mPosition.y), p2.mPosition.y);
			mMin.mPosition.z = std::min(std::min(p0.mPosition.z, p1.mPosition.z), p2.mPosition.z);

			mMax.mPosition.x = std::max(std::max(p0.mPosition.x, p1.mPosition.x), p2.mPosition.x);
			mMax.mPosition.y = std::max(std::max(p0.mPosition.y, p1.mPosition.y), p2.mPosition.y);
			mMax.mPosition.z = std::max(std::max(p0.mPosition.z, p1.mPosition.z), p2.mPosition.z);
		}

		void Draw(const Triangle& triangle, float* color, float* depth, const size_t width, const size_t height)
		{
			if (mPointsInTriangle.size() == 0)
			{
				for (float x = mMin.mPosition.x; x < mMax.mPosition.x; ++x)
				{
					for (float y = mMin.mPosition.y; y < mMax.mPosition.y; ++y)
					{
						float alpha = Triangle::SignedArea(glm::vec2(x, y), triangle.mVertices[1].mPosition, triangle.mVertices[2].mPosition) / triangle.mTotalArea;
						float beta = Triangle::SignedArea(glm::vec2(x, y), triangle.mVertices[2].mPosition, triangle.mVertices[0].mPosition) / triangle.mTotalArea;
						float gamma = Triangle::SignedArea(glm::vec2(x, y), triangle.mVertices[0].mPosition, triangle.mVertices[1].mPosition) / triangle.mTotalArea;

						if (alpha < 0 || beta < 0 || gamma < 0) continue;

						mPointsInTriangle.push_back(
							Point(
								(triangle.mVertices[0].mPosition * alpha) + (triangle.mVertices[1].mPosition * beta) + (triangle.mVertices[2].mPosition * gamma),
								(triangle.mVertices[0].mColor * alpha) + (triangle.mVertices[1].mColor * beta) + (triangle.mVertices[2].mColor * gamma)
							)
						);
					}
				}
			}

			for (const auto& p : mPointsInTriangle)
				p.Draw(color, depth, width, height);
		}

	private:
		Point mMin = Point(glm::vec3(FLT_MAX), glm::vec3(0.f));
		Point mMax = Point(glm::vec3(-FLT_MAX), glm::vec3(0.f));
		Line mLines[4];
		std::vector<Point> mPointsInTriangle;
	};

	class Triangle
	{
	public:
		Triangle(const Point _p0, const Point _p1, const Point _p2, const size_t width, const size_t height)
		{
			Point p0(_p0);
			Point p1(_p1);
			Point p2(_p2);

			mEdges[0] = Line(p0, p1);
			mEdges[1] = Line(p2, p1);
			mEdges[2] = Line(p0, p2);

			mVertices[0] = p0;
			mVertices[1] = p1;
			mVertices[2] = p2;

			mBBox = BoundingBox(p0, p1, p2);
			mTotalArea = Triangle::SignedArea(p0.mPosition, p1.mPosition, p2.mPosition);
		}

		void Draw(float* color, float* depth, const size_t width, const size_t height)
		{
			mEdges[0].Draw(color, depth, width, height);
			mEdges[1].Draw(color, depth, width, height);
			mEdges[2].Draw(color, depth, width, height);
			mBBox.Draw(*this, color, depth, width, height);
		}

		static float SignedArea(const glm::vec2 p0, const glm::vec2 p1, const glm::vec2 p2)
		{
			return 0.5f * (
				(p0.y - p1.y) * (p1.x + p0.x) +
				(p1.y - p2.y) * (p2.x + p1.x) +
				(p2.y - p0.y) * (p0.x + p2.x)
				);
		}

		Point mVertices[3];
		float mTotalArea = 0.f;

	private:
		Line mEdges[3];
		BoundingBox mBBox;
	};

	bool ArePointsToBeClipped(const glm::vec4 p0, const glm::vec4 p1, const glm::vec4 p2) const;

private:
	bool mStopRendering = false;
	std::vector<std::vector<Triangle>> mMeshInstanceTriangles;
};
