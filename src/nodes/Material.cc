#include <bulb/nodes/Material.hh>
#include "bulb/AssetReader.hh"

namespace bulb
{
   Material::Material(const void* data, size_t datasize, const char* name)  : Composite(true, name)
   //---------------------------------------------------------------------
   {
      filament::Engine* engine = Managers::instance().engine.get();
      material = filament::Material::Builder().package(data, datasize).build(*engine);
   }

   bool Material::open(const char* filename, const char* name)
//----------------------------------------------------------------------------------------
   {
      bulb::AssetReader& reader = bulb::AssetReader::instance();
      std::vector<char> materialData;
      reader.read_asset_vector(filename, materialData);
      if (materialData.empty()) return false;
      filament::Engine* engine = Managers::instance().engine.get();
      if (engine == nullptr) return false;
      material = filament::Material::Builder().package(materialData.data(), materialData.size()).build(*engine);
      return material != nullptr;
   }

   bulb::Material& Material::operator()(const char* paramName, filament::Texture* texture,
                                        filament::TextureSampler::MinFilter minFilter,
                                        filament::TextureSampler::MagFilter magFilter,
                                        filament::TextureSampler::WrapMode wrapModeS,
                                        filament::TextureSampler::WrapMode wrapModeT,
                                        filament::TextureSampler::WrapMode wrapModeR,
                                        filament::TextureSampler::CompareMode compareMode,
                                        filament::TextureSampler::CompareFunc compareFunc)
   //-------------------------------------------------------------------------------------
   {
      std::string paramname(paramName);
      textures[paramname] = texture;
      samplers.emplace(std::make_pair(paramname, filament::TextureSampler(minFilter, magFilter, wrapModeS, wrapModeT,
                                                                          wrapModeR)));
      samplers[paramname].setCompareMode(compareMode, compareFunc);
      material->setDefaultParameter(paramName, texture, samplers[paramname]);
      return *this;
   }
}