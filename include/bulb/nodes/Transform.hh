#ifndef BULB_TRANSFORM_HH_
#define BULB_TRANSFORM_HH_

#include <unordered_map>

#include "math/mat3.h"
#include "math/mat4.h"

#include "bulb/nodes/Node.hh"
#include "bulb/nodes/Composite.hh"
#include "bulb/nodes/Visitor.hh"

namespace bulb
{
class Transform : public bulb::Composite
   //===================================================
   {
   public:
      explicit Transform(const char* name =nullptr, void* animationParams =nullptr)
         : Composite(true, name), animationParameters(animationParams) {}

      void accept(NodeVisitor* visitor) override {  visitor->visit(this); }

      void set_animation_parameters(void* animationParams) { animationParameters = animationParams; }

      /**
       * @tparam F A callable @see(https://en.cppreference.com/w/cpp/named_req/Callable) ie a class overriding
       * operator()(Transform *, void *), a lambda with the same arguments or a function pointer.
       * @param f The callable (see above)
       * @param animationParameters Animation specific parameters
       */
      template <typename F>
      void animate(F f) { f(this, animationParameters); }

      virtual filament::math::mat4 matrix() =0;
      virtual filament::math::mat4f matrixf() =0;

   protected:
      void* animationParameters = nullptr;
   };
}
#endif //_TRANSFORM_HH_
