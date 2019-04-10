#include "filament/LightManager.h"
#include "utils/EntityInstance.h"

#include "bulb/Managers.hh"
#include "bulb/nodes/PositionalLight.hh"
#include "bulb/ut.hh"

namespace bulb
{
   bulb::PositionalLight::PositionalLight(const char* name, filament::LinearColor color,
                                          filament::math::float3 initialPosition, filament::math::float3 direction,
                                          filament::math::float2 cone, float intensity, float efficiency, float fade,
                                          bool hasShadows) : Drawable(name),
                                          initialPosition({initialPosition[0], initialPosition[1], initialPosition[2], 1})
   //---------------------------------------------------------------------------------------------------------------
   {
      filament::LightManager::Builder(filament::LightManager::Type::FOCUSED_SPOT)
                .color(color).intensity(intensity, efficiency).position(initialPosition)
                .direction(direction).spotLightCone(cone[0], cone[1]).falloff(fade)
                .build(*Managers::instance().engine, renderedEntity);
   }

   PositionalLight::PositionalLight(const char* name, filament::LinearColor color,
                                    filament::math::float3 initialPosition, filament::math::float3 direction,
                                    float intensity, float efficiency, float fade, bool hasShadows) :
                                    Drawable(name),
                                    initialPosition({initialPosition[0], initialPosition[1], initialPosition[2], 1})
   //----------------------------------------------------------------------------------------------
   {
      filament::LightManager::Builder(filament::LightManager::Type::POINT)
            .color(color).intensity(intensity, efficiency).position(initialPosition)
            .falloff(fade).build(*Managers::instance().engine, renderedEntity);
   }

   void PositionalLight::pre_render(std::vector<utils::Entity>& renderables)
   //------------------------------------------------------------------------
   {
      filament::LightManager& manager = Managers::instance().lightManager;
      utils::EntityInstance<filament::LightManager> inst = manager.getInstance(renderedEntity);
      filament::math::float4 pos = M*initialPosition;
      if (! near_zero(pos[3], 0.0000001f))
      {
         filament::math::float3 position(pos[0]/pos[3], pos[1]/pos[3], pos[2]/pos[3]);
         manager.setPosition(inst, position);
         // std::cout << name << std::endl << M << std::endl << position << std::endl;
      }
      renderables.push_back(renderedEntity);
   }
}