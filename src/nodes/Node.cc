#include "bulb/nodes/Node.hh"

namespace bulb
{

   void Node::add_parent(Composite* parent)
   //--------------------------------
   {
      auto it = std::find(parents.begin(), parents.end(), parent);
      if (it == std::end(parents))
         parents.push_back(parent);
   }

   bool Node::remove_parent(Composite* parent)
   //-----------------------------------
   {
      auto it = std::find(parents.begin(), parents.end(), parent);
      if (it != std::end(parents))
      {
         parents.erase(it);
         return true;
      }
      return false;
   }

   Composite* Node::set_parent(size_t i, Composite* parent)
   //-----------------------------------
   {
      Composite* prevParent = nullptr;
      if (i >= parents.size())
         add_parent(parent);
      else
      {
         prevParent = parents[i];
         parents[i] = parent;
      }
      return prevParent;
   }

   std::vector<Composite*> Node::get_parents()
   //------------------------------------
   {
      std::vector<Composite*> result;
      std::transform(parents.begin(), parents.end(), std::back_inserter(result),
                     [](Composite* parent) -> Composite* { return parent; });
      return result;
   }
}