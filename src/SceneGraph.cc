#include "backend/Platform.h"

#include "bulb/Managers.hh"
#include "bulb/SceneGraph.hh"
#include "bulb/AssetReader.hh"
#include "Log.hh"

namespace bulb
{

   bool bulb::SceneGraph::render(PostRenderCallback postRender, void* postRenderParams)
   //-------------------------------------------------------------------------------------------
   {
      if (isUpdating.load())
         return false;
      std::unique_ptr<RenderVisitor> v(new RenderVisitor);
      std::vector<utils::Entity>& renderables = v->renderables;
      if (dirty)
      {
         if (scenePtr->getRenderableCount() > 0)
         {
            view->setScene(nullptr);
            scenePtr.reset(engine->createScene(), SceneDeleter(engine));
         }
         if (!sun.isNull())
            renderables.push_back(sun);
         if (!directional_lights.empty())
         {
            std::for_each(directional_lights.begin(), directional_lights.end(),
                          [&renderables](std::pair<std::string, utils::Entity> pp) -> void
                          {
                             renderables.push_back(pp.second);
                          });
         }
         root->traverse(v.get());
         scenePtr->addEntities(renderables.data(), renderables.size());
         view->setScene(scenePtr.get());
         view->setCamera(foregroundCamera);
         for (std::weak_ptr<SceneCallback> listener : scene_listeners)
         {
            auto splistener = listener.lock();
            if (splistener) splistener->operator()(scenePtr.get(), view);
         }
         dirty = false;
      }
      if (renderer->beginFrame(swapchain))
      {
         if (backgroundView != nullptr)
         {
            renderer->render(backgroundView);
            backgroundDirty = false;
         }
         // /src/Phd/c++/Bulb/filament/filament/src/Renderer.cpp RenderPass.cpp filament/backend/src/opengl/OpenGLDriver.cpp beginRenderPass
         renderer->render(view);
         if (postRender)
            postRender(engine.get(), view, renderer, scenePtr.get(), postRenderParams);
         renderer->endFrame();
      }
      else
      {
         std::cerr << "Skipping frame\n";
         return false;
      }
      return true;
   }

   bool bulb::SceneGraph::start_updating()
//-------------------------------------
   {
      bool canUpdate = false;
      return isUpdating.compare_exchange_strong(canUpdate, true);
   }

   void bulb::SceneGraph::end_updating(bool setDirty)
   {
      dirty = setDirty;
      isUpdating.store(false);
   }

   bulb::Composite* SceneGraph::make_root(const char* name, bool isReplace)
   //-------------------------------------------------------------------------
   {
      if (root)
      {
         if (isReplace)
         {
            nodes.clear();
            root.reset();
         }
         else
            return root.get();
      }
      root = std::make_unique<bulb::Composite>(name);
      dirty = true;
      if (! root->get_name().empty())
         nodesByName[root->get_name()] = root.get();
      return root.get();
   }

   bulb::AffineTransform*
   SceneGraph::make_affine_transform(const char* name, const filament::math::quat& q, const filament::math::double3& t,
                                     const filament::math::double3& s, void* animationParams,
                                     MatrixTransformOrder transformOrder)
   //--------------------------------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Node> node = std::make_unique<bulb::AffineTransform>(name, q, t, s, animationParams, transformOrder);
      nodes.emplace_back(std::move(node));
      auto transform = dynamic_cast<AffineTransform*>(nodes.back().get());
      if (transform == nullptr)
         return nullptr;
      dirty = true;
      if (animationParams)
         animationTransforms[transform->name] = transform;
      if (! transform->get_name().empty())
         nodesByName[transform->get_name()] = transform;
      return transform;
   }

   bulb::AffineTransform*
   SceneGraph::make_affine_transform(const char* name, const filament::math::mat3& R_, const filament::math::double3& t,
                                     const filament::math::double3& s, void* animationParams,
                                     MatrixTransformOrder transformOrder)
   //-----------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Node> node = std::make_unique<bulb::AffineTransform>(name, R_, t, s, animationParams, transformOrder);
      nodes.emplace_back(std::move(node));
      auto transform = dynamic_cast<AffineTransform*>(nodes.back().get());
      if (transform == nullptr)
         return nullptr;
      dirty = true;
      if (animationParams)
         animationTransforms[transform->name] = transform;
      if (! transform->get_name().empty())
         nodesByName[transform->get_name()] = transform;
      return transform;
   }

   bool SceneGraph::adopt_affine_transform(bulb::AffineTransform* transform, void* animationParams)
   //-----------------------------------------------------------------------------------------------
   {
      auto it = std::find_if(nodes.begin(), nodes.end(),
                             [transform](const std::unique_ptr<Node>& n) -> bool
                             { return n.get() == transform; });
      if (it == std::end(nodes))
      {
         std::unique_ptr<bulb::Node> node(transform);
         nodes.emplace_back(std::move(node));
         transform = dynamic_cast<AffineTransform*>(nodes.back().get());
         if (transform == nullptr)
            return false;
         if (animationParams)
         {
            transform->set_animation_parameters(animationParams);
            animationTransforms[transform->name] = transform;
         }
         dirty = true;
         if ( (transform) && (! transform->get_name().empty()) )
            nodesByName[transform->get_name()] = transform;
         return true;
      }
      return false;
   }

   bulb::CustomTransform* SceneGraph::make_custom_transform(const char* name, const filament::math::mat4& M,
                                                            void* animationParams)
   //------------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Node> node = std::make_unique<bulb::CustomTransform>(name, M, animationParams);
      nodes.emplace_back(std::move(node));
      auto transform = dynamic_cast<CustomTransform*>(nodes.back().get());
      if (transform == nullptr)
         return nullptr;
      dirty = true;
      if (animationParams)
      {
         transform->set_animation_parameters(animationParams);
         animationTransforms[transform->name] = transform;
      }
      if (! transform->get_name().empty())
         nodesByName[transform->get_name()] = transform;
      return transform;
   }

   bool SceneGraph::adopt_custom_transform(bulb::CustomTransform* transform, void* animationParams)
   //----------------------------------------------------------------------------------------------
   {
      auto it = std::find_if(nodes.begin(), nodes.end(),
                             [transform](const std::unique_ptr<Node>& n) -> bool
                             { return n.get() == transform; });
      if (it == std::end(nodes))
      {
         std::unique_ptr<bulb::Node> node(transform);
         nodes.emplace_back(std::move(node));
         transform = dynamic_cast<CustomTransform*>(nodes.back().get());
         if (transform == nullptr)
            return false;
         if (animationParams)
         {
            transform->set_animation_parameters(animationParams);
            animationTransforms[transform->name] = transform;
         }
         dirty = true;
         if ( (transform) && (! transform->get_name().empty()) )
            nodesByName[transform->get_name()] = transform;
         return true;
      }
      return false;
   }

   bulb::Material* SceneGraph::make_material(const char* name, filament::Material* material)
   //---------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Material> node = std::make_unique<bulb::Material>(material, name);
      nodes.emplace_back(std::move(node));
      auto m = dynamic_cast<bulb::Material*>(nodes.back().get());
      if (m == nullptr)
         return nullptr;
      dirty = true;
      if (! m->get_name().empty())
         nodesByName[m->get_name()] = m;
      return m;
   }

   bulb::Material* SceneGraph::make_material(const char* name, const void* data, size_t datasize)
   //--------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Material> node = std::make_unique<bulb::Material>(data, datasize, name);
      nodes.emplace_back(std::move(node));
      auto m = dynamic_cast<bulb::Material*>(nodes.back().get());
      if (m == nullptr)
         return nullptr;
      dirty = true;
      if (! m->get_name().empty())
         nodesByName[m->get_name()] = m;
      return m;
   }

   bulb::Material* SceneGraph::make_material(const char* name, const char* filename)
   //--------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Material> node = std::make_unique<bulb::Material>(name);
      if (!node->open(filename, name))
         return nullptr;
      nodes.emplace_back(std::move(node));
      auto m = dynamic_cast<bulb::Material*>(nodes.back().get());
      if (m == nullptr)
         return nullptr;
      dirty = true;
      if (! m->get_name().empty())
         nodesByName[m->get_name()] = m;
      return m;
   }

   bool SceneGraph::adopt_material(bulb::Material* materialNode)
   //-----------------------------------------------------------
   {
      auto it = std::find_if(nodes.begin(), nodes.end(),
                             [materialNode](const std::unique_ptr<Node>& n) -> bool
                             { return n.get() == materialNode; });
      if (it == std::end(nodes))
      {
         std::unique_ptr<bulb::Material> node(materialNode);
         nodes.emplace_back(std::move(node));
         auto m = dynamic_cast<bulb::Material*>(nodes.back().get());
         if (m == nullptr)
            return false;
         dirty = true;
         if (! m->get_name().empty())
            nodesByName[m->get_name()] = m;
         return true;
      }
      return false;
   }

   bulb::Geometry*
   SceneGraph::make_geometry(const char* name, filament::Material* defaultMaterial, Transform* internalTransform)
   //-------------------------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::Geometry> node = std::make_unique<bulb::Geometry>(name, defaultMaterial, internalTransform);
      nodes.emplace_back(std::move(node));
      auto renderable = dynamic_cast<Geometry*>(nodes.back().get());
      if (renderable == nullptr)
         return nullptr;
      if (! renderable->get_name().empty())
         nodesByName[renderable->get_name()] = renderable;
      return renderable;
   }

   bool SceneGraph::adopt_geometry(bulb::Geometry* geometry)
   //--------------------------------------------------------------------
   {
      auto it = std::find_if(nodes.begin(), nodes.end(),
                             [geometry](const std::unique_ptr<Node>& n) -> bool
                             { return n.get() == geometry; });
      if (it == std::end(nodes))
      {
         std::unique_ptr<bulb::Node> node(geometry);
         nodes.emplace_back(std::move(node));
         auto renderable = dynamic_cast<Geometry*>(nodes.back().get());
         if (renderable == nullptr)
            return false;
         dirty = true;
         if (! renderable->get_name().empty())
            nodesByName[renderable->get_name()] = renderable;
         return true;
      }
      return false;
   }

   bulb::MultiGeometry*
   SceneGraph::make_multi_geometry(const char* name, filament::Material* defaultMaterial, Transform* internalTransform)
   //------------------------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::MultiGeometry> node = std::make_unique<bulb::MultiGeometry>(name, defaultMaterial, internalTransform);
      nodes.emplace_back(std::move(node));
      auto renderable = dynamic_cast<MultiGeometry*>(nodes.back().get());
      if (renderable == nullptr)
         return nullptr;
      if (! renderable->get_name().empty())
         nodesByName[renderable->get_name()] = renderable;
      return renderable;
   }

   bulb::MultiGeometry* SceneGraph::make_multi_geometry(const char* name, const char* gltfAssetPath, Transform* internalTransform,
                                                        bool normalized, bool bestShaders)
   //------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::MultiGeometry> node = std::make_unique<bulb::MultiGeometry>(name, nullptr, internalTransform);
      if (! node->open_gltf(gltfAssetPath, normalized, bestShaders))
      {
         node.reset();
         return nullptr;
      }
      nodes.emplace_back(std::move(node));
      auto renderable = dynamic_cast<MultiGeometry*>(nodes.back().get());
      if (renderable == nullptr)
         return nullptr;
      if (! renderable->get_name().empty())
         nodesByName[renderable->get_name()] = renderable;
      return renderable;
   }

   bool SceneGraph::adopt_multi_geometry(bulb::MultiGeometry* geometry)
   //--------------------------------------------------------------------
   {
      auto it = std::find_if(nodes.begin(), nodes.end(),
                             [geometry](const std::unique_ptr<Node>& n) -> bool
                             { return n.get() == geometry; });
      if (it == std::end(nodes))
      {
         std::unique_ptr<bulb::Node> node(geometry);
         nodes.emplace_back(std::move(node));
         dirty = true;
         auto renderable = dynamic_cast<MultiGeometry*>(nodes.back().get());
         if (renderable == nullptr)
            return false;
         if (! renderable->get_name().empty())
            nodesByName[renderable->get_name()] = renderable;
         return true;
      }
      return false;
   }

   bulb::PositionalLight*
   SceneGraph::make_spotlight(const char* name, filament::LinearColor color, filament::math::float3 initialPosition,
                              filament::math::float3 direction, filament::math::float2 cone, float intensity,
                              float efficiency, float fade, bool hasShadows)
   //----------------------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::PositionalLight> node = std::make_unique<bulb::PositionalLight>(name, color, initialPosition,
         direction, cone, intensity, efficiency, fade, hasShadows);
      nodes.emplace_back(std::move(node));
      auto light = dynamic_cast<PositionalLight*>(nodes.back().get());
      if  ( (light) && (! light->get_name().empty()) )
         nodesByName[light->get_name()] = light;
      return light;
   }

   bulb::PositionalLight*
   SceneGraph::make_pointlight(const char* name, filament::LinearColor color, filament::math::float3 initialPosition,
                               filament::math::float3 direction, float intensity, float efficiency, float fade,
                               bool hasShadows)
   //---------------------------------------------------------------------------------------------------------------
   {
      std::unique_ptr<bulb::PositionalLight> node = std::make_unique<bulb::PositionalLight>(name, color, initialPosition,
            direction, intensity, efficiency, fade, hasShadows);
      nodes.emplace_back(std::move(node));
      auto light = dynamic_cast<PositionalLight*>(nodes.back().get());
      if  ( (light) && (! light->get_name().empty()) )
         nodesByName[light->get_name()] = light;
      return light;
   }

   void SceneGraph::adopt_light(bulb::PositionalLight* light)
   //--------------------------------------------------------
   {
      auto it = std::find_if(nodes.begin(), nodes.end(),
                             [light](const std::unique_ptr<Node>& n) -> bool
                             { return n.get() == light; });
      if (it == std::end(nodes))
      {
         std::unique_ptr<bulb::Node> node(light);
         nodes.emplace_back(std::move(node));
         dirty = true;
         light = dynamic_cast<PositionalLight*>(nodes.back().get());
         if  ( (light) && (! light->get_name().empty()) )
            nodesByName[light->get_name()] = light;
      }
   }

   utils::Entity&
   SceneGraph::add_sunlight(filament::LinearColor color, filament::math::float3 direction, float intensity,
                            bool hasShadows, float angularRadius, float haloSize, float falloff)
   //--------------------------------------------------------------------------------------------------------------------
   {
      utils::EntityManager& entityManager = Managers::instance().entityManager;
      filament::LightManager& lightManager = Managers::instance().lightManager;
      if (!sun.isNull())
      {
         lightManager.destroy(sun);
         entityManager.destroy(sun);
      }
      sun = entityManager.create();
      filament::LightManager::Builder(filament::LightManager::Type::SUN)
            .color(color).intensity(intensity).direction(direction).castShadows(hasShadows)
            .sunAngularRadius(angularRadius).sunHaloSize(haloSize).sunHaloFalloff(falloff)
            .build(*Managers::instance().engine, sun);
      dirty = true;
      return sun;
   }

   void SceneGraph::remove_sunlight()
   //---------------------------------
   {
      if (!sun.isNull())
      {
         if (scenePtr)
            scenePtr->remove(sun);
         Managers::instance().lightManager.destroy(sun);
         Managers::instance().entityManager.destroy(sun);
         dirty = true;
      }
   }

   utils::Entity&
   SceneGraph::add_directional_light(const char* name, filament::LinearColor color, filament::math::float3 direction,
                                     float intensity, bool hasShadows)
   //-----------------------------------------------------------------------------------------------------------
   {
      utils::EntityManager& entityManager = Managers::instance().entityManager;
      filament::LightManager& lightManager = Managers::instance().lightManager;
      std::string sname(name);
      auto it = directional_lights.find(sname);
      if (it != directional_lights.end())
      {
         utils::Entity& light = it->second;
         lightManager.destroy(light);
         entityManager.destroy(light);
         directional_lights.erase(it);
      }
      directional_lights.emplace(sname, entityManager.create());
      utils::Entity& light = directional_lights[sname];
      filament::LightManager::Builder(filament::LightManager::Type::DIRECTIONAL)
            .color(color).intensity(intensity).direction(direction).castShadows(hasShadows)
            .build(*Managers::instance().engine, light);
      dirty = true;
      return light;
   }

   void SceneGraph::remove_directional_light(const char* name)
   //---------------------------------------------------------
   {
      std::string sname(name);
      auto it = directional_lights.find(sname);
      if (it != directional_lights.end())
      {
         utils::Entity& light = it->second;
         Managers::instance().lightManager.destroy(light);
         Managers::instance().entityManager.destroy(light);
         directional_lights.erase(it);
         dirty = true;
      }
   }

   filament::Texture* SceneGraph::set_background(uint32_t width, uint32_t height, filament::TextureSampler sampler)
   //--------------------------------------------------------------------------------------------------------------
   {
      bulb::Log logger("SceneGraph::set_background");
      bulb::AssetReader& reader = bulb::AssetReader::instance();
      std::vector<char> materialData;
      if (! reader.read_asset_vector("assets/bakedTexture", materialData))
      {
         logger.error("Error reading assets/bakedTexture");
         return nullptr;
      }
      backgroundMaterial = filament::Material::Builder().package(materialData.data(), materialData.size()).build(*engine);
      if (backgroundMaterial == nullptr)
      {
         logger.error("Error creating background material using assets/bakedTexture");
         return nullptr;
      }
      backgroundSampler = sampler;
      backgroundCamera = engine->createCamera();
      backgroundCamera->setProjection(filament::Camera::Projection::ORTHO, -1, 1, -1, 1, -1, 1);
      backgroundCamera->lookAt({0, 0, 0}, {0, 0, -1}, {0, 1, 0});
      std::cout << backgroundCamera->getCullingFar() << " " << backgroundCamera->getForwardVector() << "\n" <<
                   backgroundCamera->getProjectionMatrix() << std::endl;
      if (backgroundView == nullptr)
      {
         backgroundView = engine->createView();
         if (backgroundView == nullptr)
         {
            logger.error("Error creating background view.");
            return nullptr;
         }
      }
      backgroundView->setViewport({ 0, 0, width, height });
      backgroundView->setPostProcessingEnabled(false);
      backgroundView->setShadowsEnabled(false);
      backgroundView->setCamera(backgroundCamera);
      backgroundScenePtr.reset(engine->createScene(), SceneDeleter(engine));
      if (! backgroundScenePtr)
      {
         logger.error("Error creating background scene.");
         return nullptr;
      }
      backgroundView->setScene(backgroundScenePtr.get());
      backgroundView->setClearTargets(true, true, false);
      view->setClearTargets(false, true, false);
      view->setRenderTarget(filament::View::TargetBufferFlags::DEPTH_AND_STENCIL);
      view->setPostProcessingEnabled(false); //TODO: Check if still required
      backgroundTexture = filament::Texture::Builder().width(width).height(height).levels(1).
                                                       sampler(filament::Texture::Sampler::SAMPLER_2D).
                                                       format(filament::Texture::InternalFormat::RGBA8).build(*engine);
      if (backgroundTexture == nullptr)
      {
         logger.error("Error creating background texture.");
         return nullptr;
      }
      filament::MaterialInstance* materialInstance = backgroundMaterial->getDefaultInstance();
      struct TexVertex3D
      {
         filament::math::float3 position;
         filament::math::float2 uv;
      };

      const float Z = -1.0f;
      TexVertex3D* QUAD_VERTICES = new TexVertex3D[4]
      {
            {{-1, -1, Z}, {0, 0}},
            {{ 1, -1, Z}, {1, 0}},
            {{-1,  1, Z}, {0, 1}},
            {{ 1,  1, Z}, {1, 1}},
      };
      uint16_t* QUAD_INDICES = new uint16_t[6] { 0, 1, 2, 3, 2, 1 };

      backgroundVertexBuf = filament::VertexBuffer::Builder().vertexCount(4).bufferCount(1)
            .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, 0, 20)
            .attribute(filament::VertexAttribute::UV0, 0, filament::VertexBuffer::AttributeType::FLOAT2, 12, 20)
            .build(*engine);
      backgroundIndexBuffer = filament::IndexBuffer::Builder().indexCount(6)
            .bufferType(filament::IndexBuffer::IndexType::USHORT).build(*engine);
      backgroundVertexBuf->setBufferAt(*engine, 0, filament::VertexBuffer::BufferDescriptor(QUAD_VERTICES, 80,
            [](void* p, size_t size, void* user)
            //----------------------------------
            {
               if (p)
               {
                  TexVertex3D *data = static_cast<TexVertex3D*>(p);
                  delete[] data;
               }
            }));
      backgroundIndexBuffer->setBuffer(*engine, filament::IndexBuffer::BufferDescriptor(QUAD_INDICES, 12,
            [](void* p, size_t size, void* user)
            //----------------------------------
            {
               if (p)
               {
                  uint16_t *data = static_cast<uint16_t*>(p);
                  delete[] data;
               }
            }));
      background = Managers::instance().entityManager.create();
      materialInstance->setParameter("albedo", backgroundTexture, backgroundSampler);
      filament::RenderableManager::Builder(1).boundingBox({{ -1, -1, -1 }, { 1, 1, 1 }}).material(0, materialInstance)
            .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, backgroundVertexBuf,
                      backgroundIndexBuffer, 0, 6)
            .culling(false).receiveShadows(false).castShadows(false).build(*engine, background);

//      filament::TransformManager& transformManager = engine->getTransformManager();
//      utils::EntityInstance<filament::TransformManager> backgroundTransform = transformManager.getInstance(background);
//      transformManager.setTransform(backgroundTransform, filament::math::mat4f::scaling(0.5f));

      backgroundScenePtr->addEntity(background);
      return (backgroundTexture);
   }

   bool SceneGraph::set_background_image(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels,
                                         filament::backend::BufferDescriptor::Callback dataDeletor)
   //--------------------------------------------------------------------------------------
   {
      filament::Texture::PixelBufferDescriptor buffer(data, size_t(width * height * channels),
                                                      filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE,
                                                      dataDeletor);
      backgroundTexture->setImage(*engine, 0, std::move(buffer));
      backgroundDirty = true;
      return true;

   }

   std::vector<bulb::Transform*> SceneGraph::get_animated_transforms()
   //----------------------------------------------------------------
   {
      std::vector<bulb::Transform*> result;
      std::for_each(animationTransforms.begin(), animationTransforms.end(),
                    [&result](const std::pair<std::string, bulb::Transform*> pp) -> void { result.push_back(pp.second); });
      return result;
   }

   bulb::Node* SceneGraph::get_node(std::string name)
   //-----------------------------------------------
   {
      auto it = nodesByName.find(name);
      if (it == nodesByName.end())
         return nullptr;
      return it->second;
   }
}

