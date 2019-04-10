#ifndef _BULB_MATERIAL_H_
#define _BULB_MATERIAL_H_

#include <unordered_map>

#include "filament/Engine.h"
#include "filament/Material.h"

#include "bulb/Managers.hh"
#include "bulb/nodes/Composite.hh"
#include "bulb/nodes/Visitor.hh"

namespace bulb
{
   class Material  : public Composite
   {
   public:
      explicit Material(const char* name =nullptr) : Composite(true, name), material(nullptr) {}

      explicit Material(filament::Material* m, const char* name =nullptr) : Composite(true, name), material(m) {}

      Material(const void* data, size_t datasize, const char* name =nullptr);

      Material(const Material& other) = default;

      bool open(const char* filename, const char* name =nullptr);

      void accept(NodeVisitor* visitor) override {  visitor->visit(this); }

      template <typename T>
      bulb::Material& operator()(const char* param, T value)
      {
         material->setDefaultParameter(param, value);
         return *this;
      }

      bulb::Material& operator()(const char* paramName, filament::Texture* texture,
                                 filament::TextureSampler::MinFilter minFilter = filament::TextureSampler::MinFilter::NEAREST,
                                 filament::TextureSampler::MagFilter magFilter = filament::TextureSampler::MagFilter::NEAREST,
                                 filament::TextureSampler::WrapMode wrapModeS = filament::TextureSampler::WrapMode::CLAMP_TO_EDGE,
                                 filament::TextureSampler::WrapMode wrapModeT = filament::TextureSampler::WrapMode::CLAMP_TO_EDGE,
                                 filament::TextureSampler::WrapMode wrapModeR = filament::TextureSampler::WrapMode::CLAMP_TO_EDGE,
                                 filament::TextureSampler::CompareMode compareMode = filament::TextureSampler::CompareMode::NONE,
                                 filament::TextureSampler::CompareFunc compareFunc = filament::TextureSampler::CompareFunc::LE);

      filament::Material* get_material() { return material; }

   protected:
      filament::Material* material;
      std::unordered_map<std::string, filament::Texture*> textures;
      std::unordered_map<std::string, filament::TextureSampler> samplers;

   };
}

#endif
