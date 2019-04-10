#ifndef BULB_MATERIALIZABLE_HH_
#define BULB_MATERIALIZABLE_HH_

#include "filament/Material.h"

namespace bulb
{
   class Materializable
   //===================
   {
   public:
      virtual filament::Material* get_material() =0;
      virtual void set_material(filament::Material* material) =0;
      virtual filament::Material* get_material_at(size_t i) { return nullptr; }
      virtual void set_material_at(size_t i, filament::Material* material) {}
   };
}
#endif //_MATERIALIZABLE_HH_
