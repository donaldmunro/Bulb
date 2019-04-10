#ifndef H93979eb388c712f426ceb96ae5d97234
#define H93979eb388c712f426ceb96ae5d97234

#include <cmath>
#include <fstream>

#include "filament/Box.h"
#include "math/quat.h"
#include "math/mat4.h"

namespace bulb
{

   template<typename T> static inline bool near_zero(T v, T epsilon);

   template<> bool near_zero(long double v, long double epsilon) { return (fabsl(v) <= epsilon); }

   template<> bool near_zero(double v, double epsilon) { return (fabs(v) <= epsilon); }

   template<> bool near_zero(float v, float epsilon) { return (fabsf(v) <= epsilon); }

   template<typename T> constexpr T pi = T(3.1415926535897932385);

   template<typename T> constexpr T twoPi = T(3.1415926535897932385)*T(2.0);

   template<typename T> constexpr T halfPi = T(3.1415926535897932385)*T(0.5);

   template<typename T> constexpr T quarterPi = T(3.1415926535897932385)*T(0.25);

   inline static double radians_to_degrees(double radians) { return (radians * 180.0 / pi<double>); }

   inline static float radians_to_degrees(float radians) { return (radians * 180.0f / pi<float>); }

   inline static double degrees_to_radians(double degrees) { return (degrees * pi<double> / 180.0); }

   inline static float degrees_to_radians(float degrees) { return (degrees * pi<float> / 180.0f); }

   filament::math::mat4f scale_to_unitcube(const filament::Aabb& bounds);

   std::string temp_dir();

   bool is_dir(const char* dir);

   bool mkdirs(const char* dir);

   void deldirs(const char* dir, int maxDepth =0);

#ifdef HAVE_LIBZIP
   bool unzip(const char* zipfile, std::string dir);
#endif

#if !defined(NDEBUG)
   inline void debug_bounds(const filament::Aabb& aabb, filament::math::mat4f M =filament::math::mat4f(1.0f))
   {
      std::cout << M << std::endl << "Centre: " << M * aabb.center() << std::endl << "Corner Vertices:" << std::endl;
      for (filament::math::float3 v : aabb.getCorners().vertices)
         std::cout << M * v << ", ";
      std::cout << std::endl;
   }
#endif
}
#endif
