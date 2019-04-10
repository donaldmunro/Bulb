#ifndef _VISITOR_HH_
#define _VISITOR_HH_

#include <vector>
#include <list>

#include "math/mat4.h"
#include "utils/Entity.h"
#include "filament/Material.h"

namespace bulb
{
   template <typename ... Types> class Visitor;

   template <typename T>
   class Visitor<T>
   {
   public:
      virtual void visit(T* t) = 0;
   };

   template <typename T, typename ... Types>
   class Visitor<T, Types ...> : public Visitor<Types ...>
   {
   public:
      using Visitor<Types ...>::visit;
      virtual void visit(T* t) = 0;
   };

   class Transform;
   class Drawable;
   class Material;
   class Node;

   class NodeVisitor : public Visitor<Transform, Drawable, Material>
   //================================================================================
   {
   public:
      virtual ~NodeVisitor() = default;

      void visit(Transform* transform) override {  }
      void visit(Drawable* draw) override {  }
      void visit(Material* material) override {  }
      virtual void on_post_traverse(bulb::Node* node) {}
   };

   class RenderVisitor : public NodeVisitor
   //===============================================
   {
   public:
      void visit(Transform* transform) override;
      void visit(Drawable* draw) override;
      void visit(Material* material) override;
      void on_post_traverse(bulb::Node* node) override;

      std::list<filament::math::mat4> matrixStack;
      std::vector<utils::Entity> renderables;
      filament::Material* currentMaterial = nullptr;
   };
}
#endif //_VISITOR_HH_
