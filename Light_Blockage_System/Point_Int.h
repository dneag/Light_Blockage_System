#pragma once

#include <iostream>
#include <sstream>
#include <math.h>

#include <maya/MStreamUtils.h>
#include <maya/MPoint.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>

struct Point_Int {

	int x = 0;
	int y = 0;
	int z = 0;

	Point_Int() {}

	Point_Int(int X, int Y, int Z) : x(X), y(Y), z(Z) {}

	void operator=(const Point_Int& rhs) {

		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
	}

	bool operator==(const Point_Int& rhs) const {

		return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
	}

	bool operator!=(const Point_Int& rhs) const {

		return (x != rhs.x) || (y != rhs.y) || (z != rhs.z);
	}

	Point_Int& operator+=(const Point_Int& rhs) {

		x += rhs.x;
		y += rhs.y;
		z += rhs.z;

		return *this;
	}

	friend Point_Int operator+(const Point_Int& lhs, const Point_Int& rhs) {

		return Point_Int(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	}

	friend Point_Int operator-(const Point_Int& lhs, const Point_Int& rhs) {

		return Point_Int(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}

	friend std::ostream& operator<<(std::ostream& os, const Point_Int& p) {

		os << "[ " << p.x << ", " << p.y << ", " << p.z << " ]";
		return os;
	}

	MString toMString() const {

		std::stringstream ss;
		ss << "(" << x << ", " << y << ", " << z << ")";
		return ss.str().c_str();
	}

	MVector toMVector() const {

		return MVector(x, y, z);
	}

	std::string toString() const {

		std::stringstream ss;
		ss << "(" << x << ", " << y << ", " << z << ")";
		std::string s = ss.str();
		return s;
	}

	struct HashFunction {
		size_t operator()(const Point_Int& point) const {
			size_t xHash = std::hash<int>()(point.x);
			size_t yHash = std::hash<int>()(point.y) << 1;
			size_t zHash = std::hash<int>()(point.z) << 2;
			return xHash ^ yHash ^ zHash;
		}
	};
};
