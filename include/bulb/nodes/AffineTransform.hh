#ifndef a1dcd91598b83d9350365c51dea7c499
#define a1dcd91598b83d9350365c51dea7c499 1

#include "bulb/nodes/Transform.hh"
#include "bulb/ut.hh"

#include "math/mat3.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace bulb
{
   enum MatrixTransformOrder { TRS, RTS, RST, SRT, STR, TSR };

   class AffineTransform : public Transform
   //=======================================
   {
   public:
      constexpr static const double EPSILON = 0.00000001;
      constexpr static const float F_EPSILON = 0.00000001f;

      AffineTransform(const char *name, const filament::math::quat& q, const filament::math::double3& t,
                      const filament::math::double3& s = filament::math::double3(1, 1, 1),
                      void* animationParams =nullptr,
                      MatrixTransformOrder transformOrder = MatrixTransformOrder::TRS):
                      Transform(name, animationParams), M(q), R(q), S(filament::math::double4(s[0], s[1], s[2], 1)),
                      T(), Mi(), Ri(transpose(R)),
                      Si(filament::math::double4(near_zero(s[0], EPSILON) ? 0 : 1/s[0],
                                                 near_zero(s[1], EPSILON) ? 0 : 1/s[1],
                                                 near_zero(s[2], EPSILON) ? 0 : 1/s[2], 1)), Ti(),
                      order(transformOrder)
      {
//         M = bulb::MatrixTransform::quaternion_to_matrix(q);
         T[3][0] = t[0]; T[3][1] = t[1]; T[3][2] = t[2];
         Ti[3][0] = -t[0]; Ti[3][1] = -t[1]; Ti[3][2] = -t[2];
         mul();
      }
      AffineTransform(const char *name, const filament::math::mat3& R_, const filament::math::double3& t,
                      const filament::math::double3& s = filament::math::double3(1, 1, 1),
                      void* animationParams =nullptr,
                      MatrixTransformOrder transformOrder = MatrixTransformOrder::TRS) :
                      Transform(name, animationParams), M(R_), R(R_), S(filament::math::double4(s[0], s[1], s[2], 1)),
                      T(), Mi(), Ri(transpose(R)),
                      Si(filament::math::double4(near_zero(s[0], EPSILON) ? 0 : 1/s[0],
                                                near_zero(s[1], EPSILON) ? 0 : 1/s[1],
                                                near_zero(s[2], EPSILON) ? 0 : 1/s[2], 1)), Ti(),
                      order(transformOrder)
      //--------------------------------------------------------------------------------------------

      {
         T[3][0] = t[0]; T[3][1] = t[1]; T[3][2] = t[2];
         Ti[3][0] = -t[0]; Ti[3][1] = -t[1]; Ti[3][2] = -t[2];
         mul();
      }

      AffineTransform& rotation(filament::math::mat3& R_)
      //-------------------------------------
      {
         R = filament::math::mat4(R_);
         Ri = transpose(R);
         mul();
         return *this;
      }

      AffineTransform& rotation(filament::math::quat& q)
      //-------------------------------------
      {
         R = filament::math::mat4(q);
         Ri = transpose(R);
         mul();
         return *this;
      }

      const filament::math::mat4& get_rotation() const { return R; }

      AffineTransform& translation(filament::math::double3& t)
      //------------------------------------------
      {
         T[3][0] = t[0]; T[3][1] = t[1]; T[3][2] = t[2];
         Ti[3][0] = -t[0]; Ti[3][1] = -t[1]; Ti[3][2] = -t[2];
         mul();
         return *this;
      }

      AffineTransform& translation(const double x, const double y, const double z)
      //------------------------------------------
      {
         T[3][0] = x; T[3][1] = y; T[3][2] = z;
         Ti[3][0] = -x; Ti[3][1] = -y; Ti[3][2] = -z;
         mul();
         return *this;
      }

      const filament::math::mat4& get_translation() const { return T; }

      AffineTransform& scaling(const filament::math::double3& s)
      //-------------------------------------
      {
         S = filament::math::mat4(filament::math::double4(s[0], s[1], s[2], 1));
         Si = filament::math::mat4(filament::math::double4(near_zero(s[0], EPSILON) ? 0 : 1/s[0],
                                                          near_zero(s[1], EPSILON) ? 0 : 1/s[1],
                                                          near_zero(s[2], EPSILON) ? 0 : 1/s[2], 1));
         mul();
         return *this;
      }

      AffineTransform& scaling(const double x, const double y, const double z)
      //-------------------------------------
      {
         S = filament::math::mat4(filament::math::double4(x, y, z, 1));
         Si = filament::math::mat4(filament::math::double4(near_zero(x, EPSILON) ? 0 : 1/x,
                                                          near_zero(y, EPSILON) ? 0 : 1/y,
                                                          near_zero(z, EPSILON) ? 0 : 1/z, 1));
         mul();
         return *this;
      }

      AffineTransform& scaling(const double s)
      //-------------------------------------
      {
         S = filament::math::mat4(filament::math::double4(s, s, s, 1));
         const double rs = 1/s;
         Si = filament::math::mat4(filament::math::double4(near_zero(s, EPSILON) ? 0 : rs,
                                                           near_zero(s, EPSILON) ? 0 : rs,
                                                           near_zero(s, EPSILON) ? 0 : rs, 1));
         mul();
         return *this;
      }

      const filament::math::mat4& get_scaling() const { return S; }

      filament::math::mat4 matrix() override { return filament::math::mat4(M); }

      filament::math::mat4f matrixf() override { return filament::math::mat4f(M); }

   protected:
      filament::math::mat4 M, R, S, T, Mi, Ri, Si, Ti;
      MatrixTransformOrder order;

      void mul()
      //--------
      {
         switch (order)
         {
            case MatrixTransformOrder::TRS: M = T * R * S; Mi = Ti * Ri * Si; break;
            case MatrixTransformOrder::RTS: M = R * T * S; Mi = Ri * Ti * Si; break;
            case MatrixTransformOrder::RST: M = R * S * T; Mi = Ri * Si * Ti; break;
            case MatrixTransformOrder::SRT: M = S * R * T; Mi = Si * Ri * Ti; break;
            case MatrixTransformOrder::STR: M = S * T * R; Mi = Si * Ti * Ri; break;
            case MatrixTransformOrder::TSR: M = T * S * R; Mi = Ti * Si * Ri; break;
         }
      }
   };
/*
   filament::math::mat4 MatrixTransform::quaternion_to_matrix(filament::math::quatf q)
   //----------------------------------------------------------------------------------
   {
      const float w = q.w, x = q.x, y = q.y, z = q.z;
      float norm = w * w + x * x + y * y + z * z;
      float s = (norm > 0.0f) ? 2.0f / norm : 0.0f;
      float xs = x * s;
      float ys = y * s;
      float zs = z * s;
      float xx = x * xs;
      float xy = x * ys;
      float xz = x * zs;
      float xw = w * xs;
      float yy = y * ys;
      float yz = y * zs;
      float yw = w * ys;
      float zz = z * zs;
      float zw = w * zs;
      filament::math::mat4 M(1 - (yy + zz), (xy - zw),     (xz + yw),     0,
                              (xy + zw),     1 - (xx + zz), (yz - xw),     0,
                              (xz - yw),    (yz + xw),      1 - (xx + yy), 0,
                              0,            0,              0,             1);
      return M;

   }
   */
}
#endif