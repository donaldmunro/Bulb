#ifndef BULB_CUSTOM_TRANSFORM_HH_
#define BULB_CUSTOM_TRANSFORM_HH_ 1

#include "bulb/nodes/Transform.hh"

#include "math/mat3.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace bulb
{
   class CustomTransform : public Transform
   //======================================
   {
   public:
      CustomTransform(const char* name, const filament::math::mat4& m, void* animationParams =nullptr):
            Transform(name, animationParams), M(m), Mi(inverse(M)) { }

      void set(const filament::math::mat4& m)
      {
         M = m;
         Mi = inverse(m);
      }

      filament::math::mat4 matrix() override { return filament::math::mat4(M); }

      filament::math::mat4f matrixf() override { return filament::math::mat4f(M); }

   protected:
      filament::math::mat4 M, Mi;
   };
}
#endif