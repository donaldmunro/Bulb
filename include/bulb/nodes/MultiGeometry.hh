#ifndef BULB_MULTIGEOMETRY_HH_
#define BULB_MULTIGEOMETRY_HH_

#include <vector>
#include <memory>

#include "bulb/nodes/Drawable.hh"
#include "bulb/nodes/Materializable.hh"
#include "bulb/nodes/Material.hh"

#include "filament/Material.h"
#include "utils/NameComponentManager.h"
#include "gltfio/AssetLoader.h"
#include "gltfio/ResourceLoader.h"

namespace bulb
{
   /**
    * A Geometry which supports child entities as per the filament Scene support for renderable entities with transforms
    * relative to a parent entity. All transforms in the scene graph path to the root will be applied to the root entity
    * while transformation specified in the child entities will be relative to the root.
    */
   class MultiGeometry : public Drawable, public Materializable
      //=====================================================
   {
   public:
      explicit MultiGeometry(const char* name = nullptr, filament::Material* defaultMaterial = nullptr,
                             Transform* internalTransform = nullptr) :
            Drawable(name, internalTransform), defaultRootMaterial(defaultMaterial) { }

      bool open_gltf(const char* gltfPath, bool normalized = true, bool bestShaders =false);

      void pre_render(std::vector<utils::Entity>& renderables) override;

      filament::Material* get_material() override { return defaultRootMaterial; }

      void set_material(filament::Material* mat) override { defaultRootMaterial = mat; }

      filament::Material* get_material_at(size_t i) override;

      void set_material_at(size_t i, filament::Material* material) override;

      virtual utils::Entity& get_root() { return renderedEntity; }

      virtual utils::Entity* get_root_ptr() { return &renderedEntity; }

      size_t no_children() { return children.size(); }

      utils::Entity& add_child(filament::math::mat4f* T = nullptr);

      utils::Entity* get_child_at(size_t i);

   protected:
      filament::Material* defaultRootMaterial = nullptr;
      std::vector<utils::Entity> children;
      std::vector<filament::Material*> childrenMaterials;
      gltfio::AssetLoader* gltfLoader = nullptr;
      gltfio::FilamentAsset* gltfAsset = nullptr;
      filament::math::mat4f S{1.0f};
   };
};

#endif //_MULTIGEOMETRY_HH_
