#pragma once

#include <cmath>

#include <maya/MVector.h>
#include <maya/MGlobal.h>

namespace MH {

	const double PI = 3.14159265358979;
}

// For comparing doubles
bool almostEqual(double a, double b, double epsilon = 1e-8);

struct SphAngles
{
	double pol = 0.;
	double azi = 0.;

	SphAngles() {}

	SphAngles(double POL, double AZI) : pol(POL), azi(AZI) {}

	void operator=(const SphAngles& rhs) {

		pol = rhs.pol;
		azi = rhs.azi;
	}
};

// Gets the polar and azimuth angles of the given vector
SphAngles findVectorAngles(const MVector& v);

class Space
{
	double aziMatrix[3][3];
	double iAziMatrix[3][3];
	double matrixFromVectors[3][3];
	double polarOrientation = 0.;
	double aziOrientation = 0.;
	double u[3];

public:

	Space() {}

	// Create a space oriented to the angles given
	Space(SphAngles angles);

	MVector makeVector(double polar, double azimuth, double distance) const;

	/*void rotateSpace(SphAngles angles);

	void rotateSpaceAndInvertMatrix(double pRot, double aRot);

	void createMatrixFromVectors(const CVect& v1, const CVect& v2);

	void createMatrixFromUnitVectors(const CVect& v1, const CVect& v2);

	MVector makeVector(double polar, double azimuth, double distance) const;

	CVect makeCVect(double polar, double azimuth, double distance) const;

	CVect getVectorFromVectorMatrix(const CVect& v);

	double getRelativeAzimuth(const CVect& vect);

	double getRelativeAzimuth(double vect[3]);

	double getRelativePolar(const CVect& vect);*/
};
