#include "bulb/nodes/MultiGeometry.hh"

#include <gltfio/FilamentAsset.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/SimpleViewer.h>
#include <bulb/AssetReader.hh>
#include "bulb/Log.hh"
#include "bulb/ut.hh"

namespace bulb
{

   void bulb::MultiGeometry::pre_render(std::vector<utils::Entity>& renderables)
//------------------------------------
   {
      filament::TransformManager& tfm = Managers::instance().transformManager;
      utils::EntityInstance<filament::TransformManager> rootInstance = tfm.getInstance(renderedEntity);
      tfm.setTransform(rootInstance, M * S);
/*      std::cout << name << std::endl << M << std::endl << S << std::endl;
      if (gltfAsset)
      {
         const filament::Aabb& aabb = gltfAsset->getBoundingBox();
         debug_bounds(aabb, (M * S));
      }
      std::cout << "============================\n";
*/
      if ((defaultRootMaterial != nullptr) || (!childrenMaterials.empty()))
      {
#if !defined(NDEBUG)
         if (gltfAsset != nullptr)
         {
            Log logger("MultiGeometry::pre_render");
            logger.warn("Overriding gltf materials");
         }
#endif
         filament::RenderableManager& rm = Managers::instance().renderManager;
         if (defaultRootMaterial != nullptr)
         {
            filament::MaterialInstance* instance = defaultRootMaterial->getDefaultInstance();
            utils::EntityInstance<filament::RenderableManager> ei = rm.getInstance(renderedEntity);
            if (ei)
            {
               for (size_t i = 0; i < rm.getPrimitiveCount(ei); i++)
                  rm.setMaterialInstanceAt(ei, i, instance);
            }
         }
         if (!childrenMaterials.empty())
         {
            for (size_t i = 0; i < children.size(); i++)
            {
               utils::Entity& childEntity = children[i];
               filament::Material* material = childrenMaterials[i];
               filament::MaterialInstance* instance = material->getDefaultInstance();
               utils::EntityInstance<filament::RenderableManager> ei = rm.getInstance(childEntity);
               if (ei)
               {
                  for (size_t j = 0; j < rm.getPrimitiveCount(ei); j++)
                     rm.setMaterialInstanceAt(ei, j, instance);
               }
            }
         }
      }
      renderables.push_back(renderedEntity);
      for (utils::Entity child : children)
         renderables.push_back(child);
   }

   utils::Entity& bulb::MultiGeometry::add_child(filament::math::mat4f* T)
//---------------------------------------------
   {
      children.emplace_back(Managers::instance().entityManager.create());
      utils::Entity& entity = children.back();
      if (T != nullptr)
      {
         filament::TransformManager& tcm = Managers::instance().transformManager;
         tcm.create(renderedEntity, filament::TransformManager::Instance {}, filament::math::mat4f());
         tcm.create(entity, tcm.getInstance(renderedEntity), *T);
      }
      return children.back();
   }

   utils::Entity* bulb::MultiGeometry::get_child_at(size_t i)
//--------------------------------------------------------
   {
      if (i >= children.size())
         return nullptr;
      return &children[i];
   }

   filament::Material* bulb::MultiGeometry::get_material_at(size_t i)
//-----------------------------------------------------------------
   {
      if (i >= childrenMaterials.size())
         return nullptr;
      return childrenMaterials[i];
   }

   void bulb::MultiGeometry::set_material_at(size_t i, filament::Material* material)
//------------------------------------------------------------------------------
   {
      if (i >= children.size())
         return;
      childrenMaterials.resize(children.size());
      childrenMaterials[i] = material;
   }

   bool bulb::MultiGeometry::open_gltf(const char* gltfPath, bool normalized, bool bestShaders)
//---------------------------------------------------------------------------------------------
   {
      Log logger("MultiGeometry::open_gltf");
      if ((gltfAsset) || (gltfLoader))
      {
         if ( (gltfLoader) && (gltfAsset) )
            gltfLoader->destroyAsset(gltfAsset);
         if (gltfLoader)
            gltfio::AssetLoader::destroy(&gltfLoader);
         gltfLoader = nullptr;
         gltfAsset = nullptr;
         logger.info("Destroyed previous gltf loader/asset instances.");
      }
      AssetReader& reader = AssetReader::instance();
      if (!reader.exists(gltfPath))
      {
         logger.error("{0} does not exist.", gltfPath);
         return false;
      }
      if (reader.is_asset_dir(gltfPath))
      {  //TODO: Search dir for gltf file.
         logger.error("{0} is a directory but a gltf, glb or zip file was expected.", gltfPath);
         return false;
      }

      std::string filepath(gltfPath),  assetsDir, assetName, ext, nameNoExt;
      auto p = filepath.rfind('/');
#if defined (WIN32)
      if (p == std::string::npos)
            p = filepath.rfind('\\');
#endif
      if (p != std::string::npos)
      {
         assetsDir = filepath.substr(0, p);
         assetName = filepath.substr(p+1);
      }
      else
      {
#if defined(__ANDROID__)
         assetsDir = "/";
#else
         assetsDir = ".";
#endif
         assetName = filepath;
      }
      p = assetName.rfind('.');
      if (p != std::string::npos)
      {
         ext = assetName.substr(p);
         nameNoExt = assetName.substr(0, p);
      }
      else
      {
         ext = "";
         nameNoExt = assetName;
      }
      utils::Path copyToPath(gltfPath);
      bool isTmpDir = false;
      if (ext == ".zip")
      {  //TODO: Handle zips in Android assets even though assets are already zipped ?
#if defined(HAVE_LIBZIP) && !defined(__ANDROID__)
         std::string tmpdir = temp_dir() + "/" + nameNoExt;
         if (is_dir(tmpdir.c_str()))
            deldirs(tmpdir.c_str());
         if (unzip(gltfPath, tmpdir))
         {
            copyToPath = tmpdir;
            assetsDir = tmpdir;
            isTmpDir = true;
         }
         else
         {
            logger.error("Error decompressing {0}", gltfPath);
            return false;
         }
#else
         logger.error("Cannot handle {0} due to not being compiled with libzip support.", gltfPath);
         return false;
#endif
      }
#if defined(__ANDROID__)
      else
      {
         std::string tmpdir = temp_dir() + "/" + nameNoExt;
         if (is_dir(tmpdir.c_str()))
            deldirs(tmpdir.c_str());
         utils::Path destination(tmpdir);
         if (  (destination.mkdirRecursive()) &&
               (reader.copy_assets_to(assetsDir.c_str(), tmpdir.c_str()))
            )
         {
            copyToPath = destination;
            assetsDir = tmpdir;
            isTmpDir = true;
         }
         else
         {
            logger.error("Could not copy Android assets in directory {0} to sdcard {1}", assetsDir.c_str(),
                         tmpdir.c_str());
            return false;
         }
      }
#endif

      Managers& managers = Managers::instance();
      filament::Engine* engine = managers.engine.get();
      auto materials = gltfio::createMaterialGenerator(engine);
      gltfLoader = gltfio::AssetLoader::create({engine, materials});
      std::vector<unsigned char> data;
      utils::Path assetPath(assetsDir + "/" + assetName);
      if ((reader.read_asset_vector(assetPath.c_str(), data)) && (!data.empty()))
      {
         auto loadStart = std::chrono::high_resolution_clock::now();
         if (ext == ".glb")
            gltfAsset = gltfLoader->createAssetFromBinary(data.data(), data.size());
         else
            gltfAsset = gltfLoader->createAssetFromJson(data.data(), data.size());
         auto loadEnd = std::chrono::high_resolution_clock::now();
         logger.info("GLTF load for {0} took {1}ms", assetName,
                     std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count());
         data.clear();
         data.shrink_to_fit();
         if ((!gltfAsset) || (gltfAsset->getEntityCount() == 0))
         {
            if (gltfAsset)
               gltfLoader->destroyAsset(gltfAsset);
            gltfio::AssetLoader::destroy(&gltfLoader);
            gltfLoader = nullptr;
            gltfAsset = nullptr;
            logger.error("Error loading asset(s) from {0}", gltfPath);
            return false;
         }

         loadStart = std::chrono::high_resolution_clock::now();
         if (! gltfio::ResourceLoader({Managers::instance().engine.get(), assetPath.getParent(), true, false}).loadResources(gltfAsset))
            logger.error("Error loading resources from {0}", assetPath.c_str());
         else
         {
            loadEnd = std::chrono::high_resolution_clock::now();
            logger.info("GLTF Resources load for {0} took {1}ms", assetName,
                        std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count());
         }
         if (isTmpDir)
            deldirs(assetsDir.c_str());
         gltfAsset->releaseSourceData();
         renderedEntity = gltfAsset->getRoot();
         const utils::Entity* entities = gltfAsset->getEntities();
         for (size_t i = 0; i < gltfAsset->getEntityCount(); i++)
            children.push_back(entities[i]);
         if (normalized)
            S = scale_to_unitcube(gltfAsset->getBoundingBox());
         return true;
      }
      else
      {
         gltfio::AssetLoader::destroy(&gltfLoader);
         gltfLoader = nullptr;
         logger.error("Error loading data from asset file {0}", gltfPath);
      }
      return false;
   }
}