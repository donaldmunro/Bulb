#include <filamat/MaterialBuilder.h>

#include "bulb/nodes/Geometry.hh"
#include "bulb/AssetReader.hh"
#include "bulb/ut.hh"
#include "bulb/Log.hh"

namespace bulb
{
   bool Geometry::open_filamesh(const char* assetname, filament::Material* defaultMat)
   //----------------------------------------------------------------------------------
   {
      Log logger("Geometry::open_filamesh");
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
            if ( (reader.read_asset_vector("assets/bakedColor", data)) && (! data.empty()) )
            {
               filament::Engine* engine = Managers::instance().engine.get();
               if (engine == nullptr) return false;
               defaultMat = filament::Material::Builder().package(data.data(), data.size()).build(*engine);
            }
            else
               logger.error("Error loading assets/bakedColor as default material." );
         }
      }
      if (defaultMat == nullptr)
      {
         logger.error("Error loading {0}: Material not specified or error loading material.");
         Managers::instance().entityManager.destroy(renderedEntity);
         return false;
      }
      size_t nBytes;
      filameshData = reader.read_asset_buffer(assetname, nBytes);
      if (filameshData != nullptr)
      {
         filament::MaterialInstance* materialInst = defaultMat->getDefaultInstance(); //->createInstance();
         filamesh::MeshReader::Mesh mesh;
         mesh.vertexBuffer = nullptr; mesh.indexBuffer = nullptr;
         mesh = filamesh::MeshReader::loadMeshFromBuffer(Managers::instance().engine.get(), filameshData,
                                                         Geometry::delete_filamesh,
                                                         this, materialInst);
         if (mesh.vertexBuffer == nullptr)
            return false;
         Managers::instance().entityManager.destroy(renderedEntity);
         renderedEntity = mesh.renderable;
         meshVertexBuffer = mesh.vertexBuffer;
         meshIndexBuffer = mesh.indexBuffer;
//         filament::RenderableManager& rm = Managers::instance().renderManager;
//         utils::EntityInstance<filament::RenderableManager> ei = rm.getInstance(renderedEntity);
//         for (size_t i = 0; i < rm.getPrimitiveCount(ei); i++)
//            rm.setMaterialInstanceAt(ei, i, materialInst);
         material = defaultMat;
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
#if !defined(NDEBUG)
      else
      {
         Log logger("Geometry::pre_render");
         logger.error("{0} has null material.", get_name());
      }
#endif
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

   void Geometry::delete_filamesh(void* p, size_t size, void* user)
   //--------------------------------------------------------------
   {
      if (user == nullptr) return;
      Geometry* me = static_cast<Geometry*>(user);
      if (me->filameshData != nullptr) // (p) They call with an offset from the original pointer
      {
//       char* pch = static_cast<char*>(p);
         delete[] me->filameshData;
         me->filameshData = nullptr;
      }
   }
}