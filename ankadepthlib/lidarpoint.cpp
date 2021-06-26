#include "lidarpoint.h"
#include "ankadepthlibglobals.h"

using namespace AnkaDepthLib;

AnkaDepthLib::LidarPoint::LidarPoint(double _x, double _y, double _z, time_t _time, int _intensity, double _r, double _theta, double _phi)
	: X(_x), Y(_y), Z(_z), Time(_time), Intensity(_intensity), R(_r), Theta(_theta), Phi(_phi)
{}

void AnkaDepthLib::LidarPoint::faceTo(const AnkaDepthLib::LidarPoint & _other, double _heading)
{
	double x = X - _other.X;
	double y = Y - _other.Y;
	double z = Z - _other.Z;

	R = sqrt((x * x) + (y * y) + (z * z));

	Theta = (360.0 - rad2deg(atan2(y, x))) + (360.0 - (_heading + 90.0));
	if (Theta < 0.0)
		Theta += 360.0;
	Theta = fmod(Theta, 360.0);

	Phi = rad2deg(atan2(sqrt((x * x) + (y * y)), z));
	if (Phi < 0.0)
		Phi += 180.0;
	Phi = fmod(Phi, 180.0);
}