#ifndef a663b046ba06c62e8f480d45a121b57e
#define a663b046ba06c62e8f480d45a121b57e 1

#include <iostream>

#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <iterator>
#include <thread>
#include <atomic>
#include <mutex>

namespace bulb
{
   class Composite;
   class NodeVisitor;
   class SceneGraph;

   class Node
   //========
   {
   public:
      enum CallbackOps { Add, Change, Delete };

      explicit Node(bool dirty =false, const char* name =nullptr) : name(name == nullptr ? "" : name), isDirty(dirty) {}

      virtual ~Node() = default;

      virtual void accept(NodeVisitor* visitor) =0;

      virtual void traverse(NodeVisitor* visitor) { accept(visitor); }

      void set_name(const char* newname) { name = newname; }

      template <typename T> T* as() { return dynamic_cast<T>(this); }

      void add_parent(Composite* parent);

      bool remove_parent(Composite* parent);

      Composite* set_parent(size_t i, Composite* parent);

      Composite*& operator[](size_t i) { return parents[i]; }

      const Composite* parent_at(size_t index) const { return parents.at(index); }

      size_t parents_size() { return parents.size(); }

      std::vector<Composite*> get_parents();

      bool is_dirty() { return isDirty; }

      const std::string get_name() const { return name; }

   protected:
      std::string name;
      std::vector<Composite*> parents;
      bool isDirty = false;

      friend class SceneGraph;
   };
}
#endif
