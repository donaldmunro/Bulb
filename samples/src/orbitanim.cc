#include "orbitanim.hh"

#include <cmath>

void CircularAnimator::operator()(bulb::Transform* t, void* params)
//----------------------------------------
{
   CircularRotationParams* parameters = reinterpret_cast<CircularRotationParams*>(params);
   const filament::math::double3& axis = parameters->axis;
   if (! parameters->isStarted)
   {
      parameters->isStarted = true;
      parameters->angleChangedTime = parameters->lastTimeCheck = std::chrono::high_resolution_clock::now();
      std::this_thread::yield();
      return;
   }

   bulb::AffineTransform* transform = dynamic_cast<bulb::AffineTransform*>(t);
   const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
   long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - parameters->lastTimeCheck).count();
   if (elapsed > 100)
   {
      long elapsedAngle = std::chrono::duration_cast<std::chrono::milliseconds>(now - parameters->angleChangedTime).count();
//      std::cout << elapsed << " " << elapsedAngle << " " << parameters->angle << " " << parameters->angleSoFar << " "
//                << parameters->angle + parameters->angleSoFar << std::endl;
      if (elapsedAngle > 1000)
      {
         parameters->angle = parameters->angle + parameters->angleIncrement*1000.0;
         parameters->angleSoFar = 0;
         parameters->angleChangedTime = now;
         if (parameters->angle >= bulb::twoPi<double>)
            parameters->angle -= bulb::twoPi<double>;
      }
      else
         parameters->angleSoFar = double(elapsedAngle)*parameters->angleIncrement;

      filament::math::quat rotation = filament::math::quat::fromAxisAngle(normalize(axis),
                                                                          parameters->angle + parameters->angleSoFar);
      transform->rotation(rotation);
      parameters->lastTimeCheck = now;
   }
//   else
//      boost::this_fiber::yield();
}

void EllipticAnimator::operator()(bulb::Transform* t, void* params)
//------------------------------------------------------------------
{
   EllipticRotationParams* parameters = reinterpret_cast<EllipticRotationParams*>(params);
   if (! parameters->isStarted)
   {
      parameters->isStarted = true;
      parameters->angleChangedTime = parameters->lastTimeCheck = std::chrono::high_resolution_clock::now();
      return;
   }

   const filament::math::double3& axis = parameters->axis;
   bulb::AffineTransform* transform = dynamic_cast<bulb::AffineTransform*>(t);
   const std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
   long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - parameters->lastTimeCheck).count();
   if (elapsed > 100)
   {
      long elapsedAngle = std::chrono::duration_cast<std::chrono::milliseconds>(now - parameters->angleChangedTime).count();
//      std::cout << elapsed << " " << elapsedAngle << " " << parameters->angle << " " << parameters->angleSoFar << " "
//                << parameters->angle + parameters->angleSoFar << std::endl;
      if (elapsedAngle > 1000)
      {
         if (std::string(t->get_name()) == "MonolithJupiterOrbit")
         {
            std::cout << t->get_name() << parameters->a << " " << parameters->b <<  " " << parameters->angle << " " << parameters->angleIncrement << std::endl
                      << transform->get_translation() << std::endl;
         }

         parameters->angle = parameters->angle + parameters->angleIncrement*1000.0;
         parameters->angleSoFar = 0;
         parameters->angleChangedTime = now;
         if (parameters->angle >= bulb::twoPi<double>)
            parameters->angle -= bulb::twoPi<double>;
      }
      else
         parameters->angleSoFar = double(elapsedAngle)*parameters->angleIncrement;

      rotate_ellipse(transform, parameters->a, parameters->b, parameters->angle + parameters->angleSoFar, axis);
      parameters->lastTimeCheck = now;
   }
//   else
//      boost::this_fiber::yield();
}

void EllipticAnimator::rotate_ellipse(bulb::AffineTransform* transform, const double a, const double b,
                                      double angle, const filament::math::double3& axis, bool isAntiClockwise)
//----------------------------------------------------------------------------------------------------------------
{
   if (isAntiClockwise)
      angle = -angle;
   const double cos = std::cos(angle);
   const double sin = std::sin(angle);
   const double bcos = b*cos;
   const double asin = a*sin;
   double r = a*b / sqrt(bcos*bcos + asin*asin);
   double x = r*cos, z = r*sin;
   filament::math::quat rotation = filament::math::quat::fromAxisAngle(normalize(axis), angle);
   transform->rotation(rotation);
   transform->translation(x, 0, z);
}