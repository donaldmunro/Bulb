#ifndef BULB_COMPOSITE_HH_
#define BULB_COMPOSITE_HH_ 1

#include <vector>
#include <algorithm>
#include <functional>
#include <iterator>
#include <thread>
#include <mutex>
#include "cassert"

#include "bulb/nodes/Node.hh"
#include "bulb/nodes/Visitor.hh"

namespace bulb
{
   class Composite : public Node
   //===========================
   {
   public:
      explicit Composite(bool dirty = false, const char* name = nullptr) : Node(dirty, name) { }

      void accept(NodeVisitor* visitor) override {  }

      void traverse(NodeVisitor* visitor) override;

      void add_child(Node* child)
      //---------------------------------------------
      {
         auto it = std::find(children.begin(), children.end(), child);
         if (it == std::end(children))
         {
            children.emplace_back(child);
            child->add_parent(this);
            for (std::function<void(const Composite* parent, const Node* child, CallbackOps op)>& callback : child_listeners)
               callback(this, child, CallbackOps::Add);
         }
      }

      Node* set_child(Node* child, size_t index)
      //---------------------------------------------
      {
         if (index >= children.size()) add_child(child);
         Node* current_child = children[index];
         if (child == current_child) return nullptr;
         current_child->remove_parent(this);
         children[index] = child;
         child->add_parent(this);
         for (std::function<void(const Composite* parent, const Node* child, CallbackOps op)>& callback : child_listeners)
            callback(this, child, CallbackOps::Change);
         return current_child;
      }

      bool remove_child(Node* child)
      //------------------------------------------
      {
         auto it = std::find(children.begin(), children.end(), child);
         if (it != std::end(children))
         {
            child->remove_parent(this);
            children.erase(it);
            for (std::function<void(const Composite* parent, const Node* child, CallbackOps op)>& callback : child_listeners)
               callback(this, child, CallbackOps::Delete);
            return true;
         }
         return false;
      }

      std::vector<Node*> remove_children(size_t start, size_t no);

      const Node* child_at(size_t index) const { return children.at(index); }

      size_t children_size() { return children.size(); }

      std::vector<Node*> get_children();

      void add_child_listener(std::function<void(const Composite* parent, const Node* child, CallbackOps op)>& callback)
      {
         child_listeners.emplace_back(callback);
      }

   protected:
      std::vector<bulb::Node*> children;
      std::vector<std::function<void(const Composite* parent, const Node* child, CallbackOps op)>> child_listeners;
   };
}
#endif