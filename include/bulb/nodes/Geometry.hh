

#ifndef _3601667ab5151555050ba341e2e6008f
#define _3601667ab5151555050ba341e2e6008f

#include <map>

#include "bulb/Managers.hh"
#include "bulb/nodes/Drawable.hh"
#include "bulb/nodes/Materializable.hh"
#include "bulb/nodes/Material.hh"

#include "filament/Material.h"
#include <filameshio/MeshReader.h>
#include "utils/Entity.h"

namespace bulb
{
   class Geometry : public Drawable, public Materializable
   //=====================================================
   {
   public:
      explicit Geometry(const char *name = nullptr, filament::Material* defaultMaterial = nullptr,
                        Transform* internalTransform = nullptr) :
                        Drawable(name, internalTransform), material(defaultMaterial) { }

      bool open_filamesh(const char* filemeshPath, filament::Material* defaultMat = nullptr);

      void pre_render(std::vector<utils::Entity>& renderables) override;

      filament::Material* get_material() override { return material; }

      void set_material(filament::Material* mat) override { material = mat; }

      ~Geometry() override;

   protected:
      filament::Material* material = nullptr;
      filament::VertexBuffer* meshVertexBuffer = nullptr;
      filament::IndexBuffer* meshIndexBuffer = nullptr;
   };
}
#endif
