#pragma once

#include <math.h>
#include <opencv2/opencv.hpp>
#include <QString>
#include "lidarpoint.h"

namespace AnkaDepthLib
{
#define PIXEL_MULTIPLIER			1000.0f
#define W							4096.0
#define H							2048.0
#define VISION_CURVE				16.0		// opt = 16.0
#define GROUND_ASSERTION_MIN_ANGLE	115.0		// opt = 115.0
#define NEAR_DISTANCE_THRESHOLD		7.5f		// opt = 7.5f
#define MAX_DISTANCE				40.0f
#define DISTANCE_SLICE				0.25f		// opt = 0.25f
#define DISTANCED_SIZE_FACTOR		2.0
#define DEFAULT_CAM_OFFSET			2.35

#pragma region Inline Functions
	inline double deg2rad(double deg)
	{
		return deg * CV_PI / 180.0;
	}

	inline double rad2deg(double rad)
	{
		return rad * 180.0 / CV_PI;
	}

	inline float pix2dist(const cv::Vec3b & _pixel)
	{
		uint d = (_pixel.val[2] << 16) | (_pixel.val[0] << 8) | (_pixel.val[1]);
		return ((float)d) / PIXEL_MULTIPLIER;
	}

	inline cv::Vec3b dist2pix(const float _distance)
	{
		uint d = _distance * PIXEL_MULTIPLIER;
		return cv::Vec3b(
			(d >> 8) & 255,
			(d) & 255,
			(d >> 16) & 255
		);
	}

	inline float averageDistance(cv::Mat & _image, int _row, int _col, int _size, bool _exceptMid = false, int * _count = nullptr)
	{
		int x = 0, y = 0, count = 0;
		double d = 0, sum = 0;
		for (int i = (_row - _size / 2); i <= (_row + _size / 2); i++)
		{
			x = i >= 0 ? i : _image.rows - i;
			for (int j = (_col - _size / 2); j <= (_col + _size / 2); j++)
			{
				y = j >= 0 ? j : _image.cols - j;

				if (!_exceptMid || (_exceptMid && (x != _row && y != _col)))
				{
					d = _image.at<float>(x % _image.rows, y % _image.cols);

					if (d > 0)
					{
						sum += d;
						++count;
					}
				}
			}
		}

		if (_count)
			(*_count) = count;

		return count > 0 ? (sum / (double)count) : 0;
	}

	inline double minDistance(cv::Mat & _image, int _row, int _col, int _size, bool _exceptMid = false, int * _count = nullptr)
	{
		int x = 0, y = 0, count = 0;
		double d = 0, min = 0;
		for (int i = (_row - _size / 2); i <= (_row + _size / 2); i++)
		{
			x = i >= 0 ? i : _image.rows - i;
			for (int j = (_col - _size / 2); j <= (_col + _size / 2); j++)
			{
				y = j >= 0 ? j : _image.cols - j;

				if (!_exceptMid || (_exceptMid && (x != _row && y != _col)))
				{
					d = _image.at<float>(x % _image.rows, y % _image.cols);

					if (d > 0)
					{
						if (min == 0 || min > d)
							min = d;
						++count;
					}
				}
			}
		}

		if (_count)
			(*_count) = count;

		return count > 0 ? min : 0;
	}

	inline float distance(int x, int y, int i, int j)
	{
		return float(sqrt(pow(x - i, 2) + pow(y - j, 2)));
	}

	inline double gaussian(float x, double sigma)
	{
		return exp(-(pow(x, 2)) / (2 * pow(sigma, 2))) / (2 * CV_PI * pow(sigma, 2));
	}

	inline cv::Mat rotationMatrix(double _yaw, double _pitch, double _roll)
	{
		_yaw = deg2rad(_yaw);
		_pitch = deg2rad(_pitch);
		_roll = deg2rad(_roll);

		// x
		cv::Mat rotX = (cv::Mat_<double>(3, 3) <<
			1, 0, 0,
			0, cos(_pitch), -sin(_pitch),
			0, sin(_pitch), cos(_pitch)
			);

		// y
		cv::Mat rotY = (cv::Mat_<double>(3, 3) <<
			cos(_roll), 0, sin(_roll),
			0, 1, 0,
			-sin(_roll), 0, cos(_roll)
			);

		// z
		cv::Mat rotZ = (cv::Mat_<double>(3, 3) <<
			cos(_yaw), -sin(_yaw), 0,
			sin(_yaw), cos(_yaw), 0,
			0, 0, 1);

		return rotY * (rotX * rotZ);
	}
#pragma endregion

#pragma region Enums
	enum DepthTaskParameterType
	{
		DTPT_SYS_CALL,
		DTPT_TASK_CONFIG,
		DTPT_TASK_EXECUTE,
		DTPT_TASK_RESULT
	};

	enum DepthTaskWorkerStatus
	{
		DTWS_ERROR_STATE = -1,
		DTWS_IDLE = 0,
		DTWS_POOLED = 1,
		DTWS_RUNNING = 2,
		DTWS_COMPLETED = 3
	};
#pragma endregion

	class AnkaDepthLibGlobals
	{
	public:
		static constexpr uchar VersionMajor = 1;
		static constexpr uchar VersionMinor = 0;
		static constexpr uchar VersionPatch = 0;
		static constexpr uchar VersionBuild = 0;

	private:
		AnkaDepthLibGlobals();
	};
}