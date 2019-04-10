#include "bulb/nodes/Composite.hh"

namespace bulb
{

   void Composite::traverse(NodeVisitor* visitor)
   //--------------------------------------------
   {
      accept(visitor);
      for (bulb::Node* node : children)
         node->traverse(visitor);
      visitor->on_post_traverse(this);
   }

   std::vector<Node*> Composite::remove_children(size_t start, size_t no)
   //---------------------------------------------
   {
      size_t count = children.size();
      std::vector<Node*> result;
      if (start > count) return result;
      size_t end = start + no;
      if (end > count) end = children.size();
      if (start >= end) return result;
      std::for_each(children.begin()+start, children.begin()+end,
                    [this,&result](Node* child) -> void
                    {
                       child->remove_parent(this);
                       result.push_back(child);
                       for (std::function<void(const Composite* parent, const Node* child, CallbackOps op)>& callback : child_listeners)
                          callback(this, child, CallbackOps::Delete);
                    });
      children.erase(children.begin()+start, children.begin()+end);
      return result;
   }

   std::vector<Node*> Composite::get_children()
   //------------------------------------
   {
      std::vector<Node*> result;
      std::for_each(children.begin(), children.end(),
                    [&result](bulb::Node* child) -> void { result.push_back(child); });
      return result;
   }
}