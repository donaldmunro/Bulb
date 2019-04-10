#ifndef ORBIT_ORBITANIM_HH_
#define ORBIT_ORBITANIM_HH_
#include <chrono>

#include "bulb/nodes/AffineTransform.hh"

struct CircularRotationParams
{
   explicit CircularRotationParams(double degreesPerSecond,
                                   filament::math::double3 rotationAxis =filament::math::double3(0, 1, 0)) :
         axis(rotationAxis), radiansPerSecond(bulb::degrees_to_radians(degreesPerSecond)),
         angleIncrement(radiansPerSecond/1000.0) {}
   filament::math::double3 axis;
   double radiansPerSecond = 0;
   double angle = 0;
   double angleSoFar =0;
   double angleIncrement;
   std::chrono::high_resolution_clock::time_point angleChangedTime, lastTimeCheck;
   bool isStarted = false;
};

struct EllipticRotationParams : public CircularRotationParams
{
   explicit EllipticRotationParams(double degreesPerSecond, double majorRadius, double minorRadius,
                                   bool isAntiClockwise =true,
                                   filament::math::double3 rotationAxis =filament::math::double3(0, 1, 0)) :
         CircularRotationParams(degreesPerSecond, rotationAxis), a(majorRadius), b(minorRadius),
         isAntiClockwise(isAntiClockwise) {}
   double a, b;
   bool isAntiClockwise;
};

struct CircularAnimator
{
   void operator()(bulb::Transform *t, void *params);
};

struct EllipticAnimator
{
   void operator()(bulb::Transform *t, void *params);
   static void rotate_ellipse(bulb::AffineTransform* transform, const double majorAxis, const double minorAxis,
                              const double angle, const filament::math::double3& axis, bool isAntiClockwise =true);
};


#endif
