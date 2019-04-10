#include "bulb/nodes/PositionalLight.hh"
#include "bulb/nodes/Visitor.hh"
#include "bulb/nodes/Transform.hh"
#include "bulb/nodes/Drawable.hh"
#include "bulb/nodes/Material.hh"
#include "bulb/nodes/Materializable.hh"

namespace bulb
{
   void bulb::RenderVisitor::visit(bulb::Transform* transform) { matrixStack.push_back(transform->matrix()); }

   void bulb::RenderVisitor::visit(bulb::Drawable* draw)
   //------------------------------------------------------------
   {
// #if !defined(NDEBUG)
//      assert(draw->get_renderable());
// #endif
      filament::math::mat4 T(1.0f);
      for (const filament::math::mat4& M : matrixStack)
         T = T*M;
      if (draw->internalTransform)
         T = T*draw->internalTransform->matrix();
      filament::math::mat4f Tf(T);
      draw->set_final_transform(Tf);
      if (currentMaterial != nullptr)
      {
         Materializable* m = dynamic_cast<Materializable*>(draw);
         if (m != nullptr)
            m->set_material(currentMaterial);
      }
      draw->pre_render(renderables);
   }

   void RenderVisitor::on_post_traverse(bulb::Node* node)
   //-------------------------------------------------------------
   {
      if ( (dynamic_cast<bulb::Transform*>(node) != nullptr) ||
           (dynamic_cast<bulb::PositionalLight*>(node) != nullptr) )
         matrixStack.pop_back();
      else if (dynamic_cast<bulb::Material*>(node) != nullptr)
         currentMaterial = nullptr;
   }

   void RenderVisitor::visit(Material* material) { currentMaterial = material->get_material(); }
}