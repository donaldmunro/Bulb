#ifndef _6c7d81a9037040a79526937efd1d5c63
#define _6c7d81a9037040a79526937efd1d5c63 1

#include <vector>
#include <memory>
#include <atomic>
#include <stack>

#include "filament/Engine.h"
#include "filament/Camera.h"
#include "filament/View.h"
#include "filament/Renderer.h"
#include "filament/Scene.h"
#include "filament/Material.h"
#include "filament/Texture.h"
#include "filament/TextureSampler.h"
#include "utils/EntityManager.h"
#include "utils/Entity.h"
#include "filament/LightManager.h"
#include "filament/Color.h"
#include "math/mat3.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include "math/vec4.h"

#include "bulb/nodes/Node.hh"
#include "bulb/nodes/Composite.hh"
#include "bulb/nodes/Material.hh"
#include "bulb/nodes/AffineTransform.hh"
#include "bulb/nodes/CustomTransform.hh"
#include "bulb/nodes/Geometry.hh"
#include "bulb/nodes/MultiGeometry.hh"
#include "bulb/nodes/PositionalLight.hh"
#include "bulb/nodes/Visitor.hh"

namespace bulb
{
   class RenderVisitor;

   struct SceneDeleter
   {
      explicit SceneDeleter(std::shared_ptr<filament::Engine>& e) : engine(e) {}
      void operator()(filament::Scene* p) const { if ( (engine) && (p) ) engine->destroy(p); };

      std::shared_ptr<filament::Engine> engine;
   };

   using SceneCallback = std::function<void(filament::Scene* newScene, filament::View* view)>;
   using PostRenderCallback = std::function<void(filament::Engine*, filament::View*, filament::Renderer*,
                                                 filament::Scene*, void*)>;

   class SceneGraph
   //==============
   {
   public:
      SceneGraph(std::shared_ptr<filament::Engine>& engine, filament::SwapChain* swapChain, filament::View* view) :
         engine(engine), view(view), foregroundCamera(&view->getCamera()), swapchain(swapChain), renderer(engine->createRenderer()),
         dirty(true), isUpdating(false), scenePtr(engine->createScene(), SceneDeleter(engine))
         {
         }

      bool is_dirty() { return dirty; }

      bool start_updating();

      void end_updating(bool setDirty =true);

      bulb::Composite* make_root(const char* name ="Root", bool isReplace =false);
      void adopt_root(bulb::Composite* newroot) { nodes.clear(); root.reset(newroot); }

      bulb::Material* make_material(const char* name, filament::Material* m);
      bulb::Material* make_material(const char* name, const void* data, size_t datasize);
      bulb::Material* make_material(const char* name, const char* filename);
      bool adopt_material(bulb::Material* materialNode);

      bulb::AffineTransform* make_affine_transform(const char* name, const filament::math::quat& q,
                                                   const filament::math::double3& t,
                                                   const filament::math::double3& s = filament::math::double3(1, 1, 1),
                                                   void* animationParams =nullptr,
                                                   MatrixTransformOrder transformOrder = MatrixTransformOrder::TRS);
      bulb::AffineTransform* make_affine_transform(const char* name, const filament::math::mat3& R_,
                                                   const filament::math::double3& t,
                                                   const filament::math::double3& s = filament::math::double3(1, 1, 1),
                                                   void* animationParams =nullptr,
                                                   MatrixTransformOrder transformOrder = MatrixTransformOrder::TRS);
      bulb::AffineTransform* make_affine_transform(const char* name, const filament::math::quat& q,
                                                   const filament::math::double3& t,
                                                   const double s =1.0, void* animationParams =nullptr,
                                                   MatrixTransformOrder transformOrder = MatrixTransformOrder::TRS)
      {
         filament::math::double3 scale(s, s, s);
         return make_affine_transform(name, q, t, scale, animationParams, transformOrder);
      }
      bulb::AffineTransform* make_affine_transform(const char* name, const filament::math::mat3& R_,
                                                   const filament::math::double3& t,
                                                   const double s =1.0, void* animationParams =nullptr,
                                                   MatrixTransformOrder transformOrder = MatrixTransformOrder::TRS)
      {
         filament::math::double3 scale(s, s, s);
         return make_affine_transform(name, R_, t, scale, animationParams, transformOrder);
      }
      bool adopt_affine_transform(bulb::AffineTransform* transform, void* animationParams =nullptr);

      bulb::CustomTransform* make_custom_transform(const char* name, const filament::math::mat4& T, void* animationParams =nullptr);
      bool adopt_custom_transform(bulb::CustomTransform* transform, void* animationParams =nullptr);

      bulb::Geometry* make_geometry(const char* name = nullptr, filament::Material* mat = nullptr, Transform* internalTransform =nullptr);
      bool adopt_geometry(bulb::Geometry* geometry);

      bulb::MultiGeometry* make_multi_geometry(const char* name, filament::Material* defaultMaterial, Transform* internalTransform =nullptr);

      bulb::MultiGeometry* make_multi_geometry(const char* name, const char* gltfAssetPath, Transform* internalTransform =nullptr,
                                               bool normalized = true,
                                               bool bestShaders =false);

      bool adopt_multi_geometry(bulb::MultiGeometry* geometry);

      bulb::PositionalLight* make_spotlight(const char* name, filament::LinearColor color, filament::math::float3 initialPosition,
                                            filament::math::float3 direction,
                                            filament::math::float2 cone ={bulb::pi<float> / 8, (bulb::pi<float> / 8) * 1.1 },
                                            float intensity =5000.0f, float efficiency =filament::LightManager::EFFICIENCY_HALOGEN,
                                            float fade =2.0f, bool hasShadows =false);

      bulb::PositionalLight* make_pointlight(const char* name, filament::LinearColor color, filament::math::float3 initialPosition,
                                             filament::math::float3 direction, float intensity =5000.0f,
                                             float efficiency =filament::LightManager::EFFICIENCY_HALOGEN,
                                             float fade =2.0f, bool hasShadows =false);

      void adopt_light(bulb::PositionalLight* light);

      utils::Entity& add_sunlight(filament::LinearColor color, filament::math::float3 direction, float intensity =110000.0f,
                        bool hasShadows =true, float angularRadius = 1.9f, float sunHaloSize = 10.0f, float sunHaloFalloff = 80.0f);

      void remove_sunlight();

      utils::Entity& add_directional_light(const char* name, filament::LinearColor color, filament::math::float3 direction, float intensity =110000.0f,
                                 bool hasShadows =true);

      filament::Texture* set_background(uint32_t width, uint32_t height,
                                        filament::TextureSampler backgroundSampler =
                                                 filament::TextureSampler(filament::TextureSampler::MinFilter::LINEAR,
                                                                          filament::TextureSampler::MagFilter::LINEAR));
      filament::Texture* get_background_texture() { return backgroundTexture; }
      filament::TextureSampler& get_background_sampler() { return backgroundSampler; }
      bool set_background_image(unsigned char* data, uint32_t width, uint32_t height, uint32_t channels,
                                filament::backend::BufferDescriptor::Callback dataDeletor =nullptr);

      void remove_directional_light(const char* name);

      bool render(PostRenderCallback postRenderCallback =PostRenderCallback(), void* postRenderParams = nullptr);

      void add_scene_listener(std::weak_ptr<SceneCallback> listener) { scene_listeners.push_back(listener); }

      bulb::Node* get_node(std::string name);

      std::vector<bulb::Transform*> get_animated_transforms();

   protected:
      std::shared_ptr<filament::Engine> engine;
      filament::View* view;
      filament::Camera* foregroundCamera;
      filament::SwapChain* swapchain;
      std::unique_ptr<bulb::Composite> root;
      std::vector<std::unique_ptr<bulb::Node>> nodes;
      filament::Renderer* renderer;
      bool dirty, backgroundDirty = false;
      std::atomic_bool isUpdating;
      std::shared_ptr<filament::Scene> scenePtr;
      std::vector<std::weak_ptr<SceneCallback>> scene_listeners;
      utils::Entity sun;
      filament::Texture* backgroundTexture = nullptr;
      filament::TextureSampler backgroundSampler{filament::TextureSampler::MinFilter::LINEAR,
                                                 filament::TextureSampler::MagFilter::LINEAR};
      std::shared_ptr<filament::Scene> backgroundScenePtr;
      filament::View* backgroundView = nullptr;
      filament::Material* backgroundMaterial = nullptr;
      filament::Camera* backgroundCamera;
      filament::VertexBuffer* backgroundVertexBuf = nullptr;
      filament::IndexBuffer* backgroundIndexBuffer = nullptr;
      utils::Entity background;

      std::unordered_map<std::string, bulb::Node*> nodesByName;
      std::unordered_map<std::string, utils::Entity> directional_lights;
      std::unordered_map<std::string, bulb::Transform*> animationTransforms;
   };
}

#endif //_SCENEGRAPH_HH_
