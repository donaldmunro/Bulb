#include "bulb/nodes/Drawable.hh"

#include <iostream>

#include "filament/Material.h"
#include "utils/EntityInstance.h"

namespace bulb
{
   void Drawable::pre_render(std::vector<utils::Entity>& renderables)
   //-------------------------
   {
      filament::TransformManager& transformManager = Managers::instance().transformManager;
      utils::EntityInstance<filament::TransformManager> transform = transformManager.getInstance(renderedEntity);
      transformManager.setTransform(transform, M);

      // std::cout << name << std::endl << M << std::endl;
      // std::cout << "============================\n";
   }

   Drawable::~Drawable()
   //------------------
   {
      Node::~Node();
      if (internalTransform) internalTransform.reset();
      Managers::instance().entityManager.destroy(renderedEntity);
   }
}