#pragma once

#include <math.h>
#include <time.h>
#include <QVector>
#include <QMap>

namespace AnkaDepthLib
{
	class LidarPoint
	{
	public:
		double X;
		double Y;
		double Z;
		time_t Time;
		int Intensity;
		double R;
		double Theta;
		double Phi;

		LidarPoint(double _x = 0, double _y = 0, double _z = 0, time_t _time = 0, int _intensity = 0, double _r = 0, double _theta = 0, double _phi = 0);

		void faceTo(const LidarPoint & _other, double _heading = 0);

		inline int get2D_X(double _width) { return (int)(_width * Theta / 360.0); }

		inline int get2D_Y(double _height) { return (int)(_height * Phi / 180.0); }

		inline bool equals(const LidarPoint & _other) { return _other.X == X && _other.Y == Y && _other.Z == Z; }
	};

	typedef QVector<LidarPoint> LidarPointVector;
	typedef QMap<int, LidarPointVector> LidarPointDistSliceMap;
}