#include <filamat/MaterialBuilder.h>

#include "bulb/nodes/Geometry.hh"
#include "bulb/AssetReader.hh"
#include "bulb/ut.hh"

namespace bulb
{
   bool Geometry::open_filamesh(const char* assetname, filament::Material* defaultMat)
   //----------------------------------------------------------------------------------
   {
      AssetReader& reader = AssetReader::instance();
      if (! reader.exists(assetname))
         return false;
      if (defaultMat == nullptr)
      {
         if (material != nullptr)
            defaultMat = material;
         else
         {
            std::vector<unsigned char> data;
            if ( (reader.read_asset_vector("assets/bakedColor.cmat", data)) && (! data.empty()) )
            {
               filament::Engine* engine = Managers::instance().engine.get();
               if (engine == nullptr) return false;
               defaultMat = filament::Material::Builder().package(data.data(), data.size()).build(*engine);
            }
         }
      }
      std::vector<unsigned char> data;
      if ( (reader.read_asset_vector(assetname, data)) && (! data.empty()) )
      {
         filament::MaterialInstance* materialInst = defaultMat->createInstance();
         filamesh::MeshReader::Mesh mesh;
         mesh.vertexBuffer = nullptr; mesh.indexBuffer = nullptr;
         mesh = filamesh::MeshReader::loadMeshFromBuffer(Managers::instance().engine.get(), data.data(), nullptr,
                                                         nullptr, materialInst);
         if (mesh.vertexBuffer == nullptr)
            return false;
         Managers::instance().entityManager.destroy(renderedEntity);
         renderedEntity = mesh.renderable;
         meshVertexBuffer = mesh.vertexBuffer;
         meshIndexBuffer = mesh.indexBuffer;
         filament::RenderableManager& rm = Managers::instance().renderManager;
         utils::EntityInstance<filament::RenderableManager> ei = rm.getInstance(renderedEntity);
         for (size_t i = 0; i < rm.getPrimitiveCount(ei); i++)
            rm.setMaterialInstanceAt(ei, i, materialInst);
         return true;
      }
      return false;
   }

   void Geometry::pre_render(std::vector<utils::Entity>& renderables)
   //-------------------------
   {
      Drawable::pre_render(renderables);
      if (material != nullptr)
      {
         filament::MaterialInstance *instance = material->getDefaultInstance();
         filament::RenderableManager& rm = Managers::instance().renderManager;
         utils::EntityInstance<filament::RenderableManager> ei = rm.getInstance(renderedEntity);
         if (ei)
         {
            for (size_t i = 0; i < rm.getPrimitiveCount(ei); i++)
               rm.setMaterialInstanceAt(ei, i, instance);
         }
      }
      renderables.push_back(renderedEntity);
   }

   Geometry::~Geometry()
   //------------------
   {
      filament::Engine* engine = Managers::instance().engine.get();
      if (engine != nullptr)
      {
         if (meshVertexBuffer != nullptr)
            engine->destroy(meshVertexBuffer);
         if (meshIndexBuffer != nullptr)
            engine->destroy(meshIndexBuffer);
         engine->destroy(renderedEntity);
//         if (material != nullptr) engine->destroy(material);
      }
      Managers::instance().entityManager.destroy(renderedEntity);
   }
}