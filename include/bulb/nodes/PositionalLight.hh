#ifndef BULB_POSITIONALLIGHT_H_
#define BULB_POSITIONALLIGHT_H_

#include "filament/LightManager.h"
#include "filament/Color.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/mat4.h"

#include "bulb/nodes/Drawable.hh"
#include "bulb/nodes/Visitor.hh"
#include "bulb/ut.hh"

namespace bulb
{
   class PositionalLight: public Drawable
   //================================
   {
   public:
      PositionalLight(const char* name, filament::LinearColor color, filament::math::float3 initialPosition,
                      filament::math::float3 direction,
                      filament::math::float2 cone ={bulb::pi<float> / 8, (bulb::pi<float> / 8) * 1.1 },
                      float intensity =5000.0f, float efficiency =filament::LightManager::EFFICIENCY_HALOGEN,
                      float fade =2.0f, bool hasShadows =false);

      PositionalLight(const char* name, filament::LinearColor color, filament::math::float3 initialPosition,
                      filament::math::float3 direction, float intensity =5000.0f,
                      float efficiency =filament::LightManager::EFFICIENCY_HALOGEN,
                      float fade =2.0f, bool hasShadows =false);

      void pre_render(std::vector<utils::Entity>& renderables) override;

   protected:
      filament::math::float4 initialPosition;
   };
}

#endif //_POSITIONALLIGHT_H_
