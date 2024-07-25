
#include "MathHelper.h"

bool almostEqual(double a, double b, double epsilon) {
	return std::fabs(a - b) < epsilon;
}

SphAngles findVectorAngles(const MVector& v)
{
	if (v.length() == 0.)
		MGlobal::displayError("DISTANCE IS ZERO, THATS A PROBLEM (findVectorAngles(const CVect &v))");

	SphAngles angles;

	if ((v.x > 0.) && (v.z > 0.))
	{
		angles.pol = std::atan(v.z / v.x);
	}
	else if ((v.x < 0.) && (v.z > 0.))
	{
		angles.pol = MH::PI - atan(v.z / fabs(v.x));
	}
	else if ((v.x < 0.) && (v.z < 0.))
	{
		angles.pol = MH::PI + std::atan(std::fabs(v.z) / std::fabs(v.x));
	}
	else if ((v.x > 0.) && (v.z < 0.))
	{
		angles.pol = (MH::PI * 2) - std::atan(std::fabs(v.z) / v.x);
	}
	else if ((v.z == 0.) && (v.x > 0.))
	{
		angles.pol = 0.;
	}
	else if ((v.z == 0.) && (v.x < 0.))
	{
		angles.pol = MH::PI;
	}
	else if ((v.x == 0.) && (v.z > 0.))
	{
		angles.pol = MH::PI * .5;
	}
	else if ((v.x == 0.) && (v.z < 0.))
	{
		angles.pol = MH::PI * 1.5;
	}
	else
	{
		angles.pol = 0.;
	}

	angles.azi = std::acos(v.y / v.length());

	if (angles.azi == 0. && v.y < 0.)
		angles.azi = MH::PI;

	if (std::isnan(angles.azi) || std::isnan(angles.pol)) {

		MGlobal::displayError(MString() + "Found NaN value when computing vector spherical angles. azi: " + angles.azi + ", pol: " + angles.pol +
			"\nvector was: " + v.x + ", " + v.y + ", " + v.z + ", mag: " + v.length());
	}

	return angles;
}

Space::Space(SphAngles angles) {

	angles.azi *= -1.;

	double cosA = std::cos(angles.azi);
	double mCos = 1 - cosA;
	double sinA = std::sin(angles.azi);
	double unitVectorPolar = angles.pol + (MH::PI * .5);
	double uX = std::cos(unitVectorPolar);
	//float uY = 0.; to simplify the matrix equations, anything multiplied by uY is removed because uY is always 0
	double uZ = std::sin(unitVectorPolar);
	double uXTimesMCos = uX * mCos;
	double uXTimesSinA = uX * sinA;
	double uZTimesSinA = uZ * sinA;
	double uXTimesUZTimesMCos = uXTimesMCos * uZ;

	aziMatrix[0][0] = uX * uXTimesMCos + cosA;
	aziMatrix[0][1] = -1 * uZTimesSinA;
	aziMatrix[0][2] = uXTimesUZTimesMCos;
	aziMatrix[1][0] = uZTimesSinA;
	aziMatrix[1][1] = cosA;
	aziMatrix[1][2] = -1 * uXTimesSinA;
	aziMatrix[2][0] = uXTimesUZTimesMCos;
	aziMatrix[2][1] = uXTimesSinA;
	aziMatrix[2][2] = uZ * uZ * mCos + cosA;

	polarOrientation = angles.pol;
	aziOrientation = angles.azi;

	u[0] = uX;
	u[1] = 0.;
	u[2] = uZ;
}

MVector Space::makeVector(double polar, double azimuth, double distance) const
{
	double vector[3] = { 0.,0.,0. };
	double addedPolar = polar + polarOrientation;
	double distTimesSinAzi = distance * std::sin(azimuth);
	double vectorToRotate[3] = {
		distTimesSinAzi * std::cos(addedPolar),
		distance * std::cos(azimuth),
		distTimesSinAzi * std::sin(addedPolar),
	};

	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			vector[r] += aziMatrix[r][c] * vectorToRotate[c];
		}
	}

	return MVector(vector[0], vector[1], vector[2]);
}