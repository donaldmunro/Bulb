#ifndef _DRAWABLE_H_
#define _DRAWABLE_H_

#include <iostream>
#include <memory>

#include "math/mat4.h"
#include "filament/TransformManager.h"
#include "filament/Box.h"
#include "filament/Material.h"
#include "utils/Entity.h"

#include "bulb/Managers.hh"
#include "bulb/nodes/Node.hh"
#include "bulb/nodes/Transform.hh"
#include "bulb/nodes/Visitor.hh"

namespace bulb
{
   class Drawable : public Node
   //==========================
   {
   public:
      explicit Drawable(const char* name, Transform* internalTransform = nullptr) : Node(true, name),
            renderedEntity(Managers::instance().entityManager.create()), internalTransform(internalTransform) {}

      ~Drawable() override;

      void accept(NodeVisitor* visitor) override { visitor->visit(this); }

      virtual void pre_render(std::vector<utils::Entity>& renderables);

      virtual utils::Entity& get_renderable() { return renderedEntity; }

      virtual utils::Entity* get_renderable_ptr() { return &renderedEntity; }

      void set_transform(Transform* T) { internalTransform.reset(T); }

      Transform* get_transform() { return internalTransform.get(); }

   protected:
      utils::Entity renderedEntity;
      filament::math::mat4f M{1.0f};
      filament::Box BB;
      std::unique_ptr<Transform> internalTransform;

      // This is the final transform used to position the Drawable in the world composed of possibly multiple transforms
      // + the internal transform and is called by RenderVisitor.
      void set_final_transform(filament::math::mat4f& T) { M = T; }

      friend class RenderVisitor;
   };
};

#endif //_DRAWABLE_H_
