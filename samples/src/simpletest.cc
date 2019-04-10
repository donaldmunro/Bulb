#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include "filament/Engine.h"
#include "backend/Platform.h"
#include "filament/Renderer.h"
#include "filament/Scene.h"
#include "filament/View.h"
#include "filament/Material.h"
#include "filament/VertexBuffer.h"
#include "filament/IndexBuffer.h"
#include "filament/RenderableManager.h"
#include "filament/TransformManager.h"
#include <filament/Texture.h>
#include <bulb/nodes/AffineTransform.hh>
#include "bulb/SceneGraph.hh"
#include "utils/EntityManager.h"
#include "utils/Entity.h"
#include "math/quat.h"

#include "bulb/Managers.hh"
#include "bulb/ut.hh"
#include "bulb/AssetReader.hh"
#include "bulb/nodes/Geometry.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "Win.hh"

struct ColorVertex3D
{
   filament::math::float3 position;
   uint32_t color;
};

struct TexVertex3D
{
   filament::math::float3 position;
   filament::math::float2 uv;
};

const float TRIANGLE_Z = -0.2f;

static std::vector<ColorVertex3D> TRIANGLE_VERTICES =
{
      {{1, 0, TRIANGLE_Z}, 0xffff0000u},
      {{cos(M_PI * 2 / 3), sin(M_PI * 2 / 3), TRIANGLE_Z}, 0xff00ff00u},
      {{cos(M_PI * 4 / 3), sin(M_PI * 4 / 3), TRIANGLE_Z}, 0xff0000ffu},
};

static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

static std::vector<TexVertex3D> TEX_TRIANGLE_VERTICES =
{
      {{1, 0, TRIANGLE_Z}, {1, 0.5}},
      {{cos(M_PI * 2 / 3), sin(M_PI * 2 / 3), TRIANGLE_Z}, {0, 1}},
      {{cos(M_PI * 4 / 3), sin(M_PI * 4 / 3), TRIANGLE_Z}, {0, 0}}
};

static constexpr uint16_t TEX_TRIANGLE_INDICES[3] = { 0, 1, 2 };

static std::vector<TexVertex3D> QUAD_VERTICES =
      {
            {{-1, -1, TRIANGLE_Z}, {0, 0}},
            {{ 1, -1, TRIANGLE_Z}, {1, 0}},
            {{-1,  1, TRIANGLE_Z}, {0, 1}},
            {{ 1,  1, TRIANGLE_Z}, {1, 1}},
      };

static constexpr uint16_t QUAD_INDICES[6] = {0, 1, 2, 3, 2, 1,};

filament::SwapChain* swapChain = nullptr;
filament::View* view = nullptr;
filament::VertexBuffer* vb = nullptr, *texvb = nullptr;
filament::IndexBuffer* ib = nullptr, *texib = nullptr;
filament::Camera* camera = nullptr;

std::unique_ptr<Win> window;
std::unique_ptr<bulb::SceneGraph> graph;

void event_loop();
bool create_graph();
void destroy_graph();

std::string bakedColor("samples/material/bakedColor.opengl"), bakedTexture("samples/material/bakedTexture.opengl"),
            textureFile("samples/texture/gravel.png"), cubeModel("samples/model/cube/cube.filamesh"),
            cubeTextureFile("samples/model/cube/default.png");

filament::Texture* monoTexture;
filament::TextureSampler monoSampler(filament::TextureSampler::MinFilter::LINEAR,
                                     filament::TextureSampler::MagFilter::LINEAR);

void window_resized(unsigned lastWidth, unsigned lastHeight, unsigned newWidth, unsigned newHeight)
//-------------------------------------------------------------------------------------------------
{
   if ( (graph) && ( (lastWidth != newWidth) || (lastHeight != newHeight) ) )
      destroy_graph();
   if (! graph)
   {
      if (! create_graph())
      {
         std::cerr << "Error creating scene graph\n";
         std::exit(1);
      }
   }
}

int main(int argc, char** argv)
//------------------------------
{
   if (argc > 1)
   {
      std::string arg(argv[1]);
      std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
      if ( (arg == "vulkan") || (arg == "vulcan") )
      {
         bulb::SELECTED_BACKEND = filament::Engine::Backend::VULKAN;
         bakedColor = "samples/material/bakedColor.vulkan";
         bakedTexture = "samples/material/bakedTexture.vulkan";
      }
   }
   std::vector<unsigned char> v;
   unsigned window_width, window_height;
   const std::vector<std::pair<unsigned int, unsigned int>>& winSizes = Win::sizes();
   if (! winSizes.empty())
   {
      window_width = winSizes[0].first;
      window_height = winSizes[0].second;
   }
   else
   {
      window_width = 1024;
      window_height = 768;
   }
   std::function<void(unsigned, unsigned, unsigned, unsigned)> f = window_resized;
   window.reset(new Win(window_width, window_height, f));
   event_loop();
}

void event_loop()
//---------------
{
   double fps = 40;
   long spf = 1000000000L / fps;
   auto frame_start = std::chrono::high_resolution_clock::now();
   bool is_quit = false;
   do
   {
      if (! window)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         continue;
      }
      if (! window->process_event())
         break;
      if (! graph) continue;
      auto frame_end = std::chrono::high_resolution_clock::now();
      long elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_end - frame_start).count(), last_duration;
      std::cout << elapsed << " " << spf << std::endl;
      if (elapsed >= spf)
      {
         graph->render();
         frame_start = frame_end;
         last_duration = elapsed;
      }
      else //if (elapsed < spf)
         std::this_thread::sleep_for(std::chrono::milliseconds(5));
   } while (! is_quit);
   destroy_graph();
   window.reset();
}

void destroy_graph()
//--------------------
{
   std::shared_ptr<filament::Engine> engine = bulb::Managers::instance().engine;
   if (engine)
   {
      if (camera) engine->destroy(camera); camera = nullptr;
      if (vb) engine->destroy(vb); vb = nullptr;
      if (ib) engine->destroy(ib); ib = nullptr;
      graph.reset(nullptr);
   }
}

template <typename T>
filament::Box to_box(std::vector<T>& vertices)
//--------------------------------------------------------
{
   float maxx = std::numeric_limits<float>::min(), maxy = std::numeric_limits<float>::min(),
         maxz = std::numeric_limits<float>::min(), minx = std::numeric_limits<float>::max(),
         miny = std::numeric_limits<float>::max(), minz = std::numeric_limits<float>::max();
   for (T vertex : vertices)
   {
      if (vertex.position.x > maxx) maxx = vertex.position.x;
      if (vertex.position.y > maxy) maxy = vertex.position.y;
      if (vertex.position.z > maxz) maxz = vertex.position.z;
      if (vertex.position.x < minx) minx = vertex.position.x;
      if (vertex.position.y < miny) miny = vertex.position.y;
      if (vertex.position.z < minz) minz = vertex.position.z;
   }
   return filament::Box({filament::math::float3(minx, miny, minz), filament::math::float3(maxx, maxy, maxz)});
}

bool create_cube(filament::Engine* engine, bulb::SceneGraph* graph, bulb::Composite* root);

bool create_graph()
//------------------------------------
{
   std::shared_ptr<filament::Engine> engine = bulb::Managers::instance().engine;
   swapChain = engine->createSwapChain(window->native_window());
   view = engine->createView();
   view->setClearColor({0, 0, 0, 1.0});
   view->setViewport({ 0, 0, window->width, window->height });
   camera = engine->createCamera();
//   camera->lookAt({0, 0, 0}, {0, 0, -1}, {0, 1, 0});
   std::cout << "Camera position " << camera->getPosition() << "Forward " << camera->getForwardVector() << std::endl;
//   camera->setProjection(45, 16.0/9.0, 0.1, 1.0);
   view->setCamera(camera);
   graph = std::make_unique<bulb::SceneGraph>(engine, swapChain, view);

   while (! graph->start_updating())  // Not really necessary here, but good practise for multithreaded updates to the graph
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
   bulb::Composite* root = graph->make_root("root");
   bulb::Material* colorMaterialNode = graph->make_material(bakedColor.c_str(), "Color Material");
   if (colorMaterialNode == nullptr)
   {
      std::cerr << "Error opening material file " << bakedColor << std::endl;
      return false;
   }
   root->add_child(colorMaterialNode);

   bulb::AffineTransform* triangleTransform1 = graph->make_affine_transform("Triangle 1 Transform", filament::math::mat3(1),
                                                                            filament::math::double3(-0.75, 0.75, 0),
                                                                            filament::math::double3(0.25, 0.25, 1));
   triangleTransform1->set_name("Transform 1");
   colorMaterialNode->add_child(triangleTransform1);
   vb = filament::VertexBuffer::Builder().bufferCount(1).vertexCount(3)
                                         .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, 0, 16)
                                         .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, 12, 16)
                                         .normalized(filament::VertexAttribute::COLOR)
                                         .build(*engine);
   ib = filament::IndexBuffer::Builder().indexCount(3).bufferType(filament::IndexBuffer::IndexType::USHORT).build(*engine);
   vb->setBufferAt(*engine, 0, filament::VertexBuffer::BufferDescriptor(TRIANGLE_VERTICES.data(), 48, nullptr));
   ib->setBuffer(*engine, filament::IndexBuffer::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr));
   bulb::Geometry* triangleNode1 = graph->make_geometry();
   triangleNode1->set_name("Triangle 1");
   triangleTransform1->add_child(triangleNode1);
   filament::RenderableManager::Builder(1).boundingBox(to_box(TRIANGLE_VERTICES)) //material is inherited from material node
                                          .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 3)
                                          .culling(false)
                                          .receiveShadows(false)
                                          .castShadows(false)
                                          .build(*engine, triangleNode1->get_renderable());

   bulb::AffineTransform* triangleTransform2 = graph->make_affine_transform("Triangle 2 Transform", filament::math::mat3(1),
                                                                            filament::math::double3(0.5*4, 0, 0),
                                                                            filament::math::double3(1, 1, 1));
   triangleTransform2->set_name("Transform 2");
   triangleTransform1->add_child(triangleTransform2);
   bulb::Geometry* triangleNode2 = graph->make_geometry();
   triangleNode2->set_name("Triangle 2");
   triangleTransform2->add_child(triangleNode2);
   filament::RenderableManager::Builder(1).boundingBox(to_box(TRIANGLE_VERTICES))
                                          .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 3)
                                          .culling(false)
                                          .receiveShadows(false)
                                          .castShadows(false)
                                          .build(*engine, triangleNode2->get_renderable());

   bulb::Material* texMaterialNode = graph->make_material(bakedTexture.c_str(), "Texture Material");
   if (texMaterialNode == nullptr)
   {
      std::cerr << "Error opening material file " << bakedTexture << std::endl;
      return false;
   }
   root->add_child(texMaterialNode);
   int w, h, channels;
   std::shared_ptr<unsigned char> img(stbi_load(textureFile.c_str(), &w, &h, &channels, 4),
                                      [](unsigned char* p) { if (p) stbi_image_free(p); });
   assert(img);
   filament::Texture* texture = filament::Texture::Builder().width(uint32_t(w)).height(uint32_t(h))
                                                                 .levels(1).sampler(filament::Texture::Sampler::SAMPLER_2D)
                                                                 .format(filament::backend::TextureFormat::RGBA8).build(*engine);
   filament::Texture::PixelBufferDescriptor buffer(img.get(), size_t(w * h * 4),
                                                   filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE);
   texture->setImage(*engine, 0, std::move(buffer));
   (*texMaterialNode)("albedo", texture, filament::TextureSampler::MinFilter::LINEAR,
                        filament::TextureSampler::MagFilter::LINEAR);
   bulb::AffineTransform* triangleTransform3 = graph->make_affine_transform("Triangle 3 Transform", filament::math::mat3(1),
                                                                            filament::math::double3(0.25, -0.25, 0.95),
                                                                            filament::math::double3(0.25, 0.25, 1));
   triangleTransform3->set_name("Transform 3");
   texMaterialNode->add_child(triangleTransform3);
   bulb::Geometry* triangleNode3 = graph->make_geometry("Triangle 3");
   texvb = filament::VertexBuffer::Builder().vertexCount(3).bufferCount(1)
                                            .attribute(filament::VertexAttribute::POSITION, 0,
                                                       filament::VertexBuffer::AttributeType::FLOAT3, 0, 20)
                                            .attribute(filament::VertexAttribute::UV0, 0,
                                                       filament::VertexBuffer::AttributeType::FLOAT2, 12, 20).build(*engine);
   texib = filament::IndexBuffer::Builder().indexCount(3).bufferType(filament::IndexBuffer::IndexType::USHORT).build(*engine);
   texvb->setBufferAt(*engine, 0, filament::VertexBuffer::BufferDescriptor(TEX_TRIANGLE_VERTICES.data(), 60, nullptr));
   texib->setBuffer(*engine, filament::IndexBuffer::BufferDescriptor(TEX_TRIANGLE_INDICES, 6, nullptr));
   filament::RenderableManager::Builder(1).boundingBox(to_box(TEX_TRIANGLE_VERTICES))
                                          .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, texvb, texib, 0, 3)
                                          .culling(false)
                                          .receiveShadows(false)
                                          .castShadows(false)
                                          .build(*engine, triangleNode3->get_renderable());
   triangleTransform3->add_child(triangleNode3);

   create_cube(engine.get(), graph.get(), root);

   graph->end_updating();
   return true;
}

bool create_cube(filament::Engine* engine, bulb::SceneGraph* graph, bulb::Composite* root)
//----------------------------------------------------------------------------------------
{
   filament::math::quat rotation = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(1, 1, 0)),
                                                                       bulb::pi<double>/4.0);
   bulb::AffineTransform* cubeTransform = graph->make_affine_transform("Cube", rotation, filament::math::double3(0, 0, 0.25), 0.25);
   root->add_child(cubeTransform);
   bulb::AssetReader& reader = bulb::AssetReader::instance();
   std::vector<char> materialData;
   reader.read_asset_vector("samples/material/cube.cmat", materialData);
   filament::Material* cubeMaterial = filament::Material::Builder().package(materialData.data(), materialData.size()).build(*engine);
   int w, h, channels;
   std::shared_ptr<unsigned char> img(stbi_load(cubeTextureFile.c_str(), &w, &h, &channels, 4),
                                      [](unsigned char* p) { if (p) stbi_image_free(p); });
   if (! img)
   {
      std::cerr <<  "Error opening cube texture " << cubeTextureFile << std::endl;
      return false;
   }
   monoTexture = filament::Texture::Builder().width(uint32_t(w)).height(uint32_t(h))
                                                                 .levels(1).sampler(filament::Texture::Sampler::SAMPLER_2D)
                                                                 .format(filament::backend::TextureFormat::RGBA8).build(*engine);
   filament::Texture::PixelBufferDescriptor buffer(img.get(), size_t(w * h * 4),
                                                   filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE);
   monoTexture->setImage(*engine, 0, std::move(buffer));
   cubeMaterial->setDefaultParameter("albedo", monoTexture, monoSampler);
   cubeMaterial->setDefaultParameter("metallic", 1.0f);
   cubeMaterial->setDefaultParameter("roughness", 0.4f);
   cubeMaterial->setDefaultParameter("reflectance", 0.4f);
   cubeMaterial->setDefaultParameter("anisotropy", 0.0f);
   bulb::Geometry* cubeNode = graph->make_geometry("Cube", cubeMaterial);
   if (! cubeNode->open_filamesh(cubeModel.c_str(), cubeMaterial))
   {
      std::cerr << "Error opening filamesh mesh file " << cubeModel << std::endl;
      return false;
   }
   filament::LinearColor lightColor = filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(0.98f, 0.92f, 0.89f));
   cubeTransform->add_child(cubeNode);

   bulb::AffineTransform* cubeLightTransform = graph->make_affine_transform("Cube Light Transform", filament::math::mat3(1),
                                                                    filament::math::double3(0, 0, 0),
                                                                    filament::math::double3(1, 1, 1));
   root->add_child(cubeLightTransform);
   bulb::PositionalLight* pointLightNode = graph->make_pointlight("Point", lightColor, filament::math::float3({0, 0, 0.5}),
                                                                 filament::math::float3({0, 0, -1}), 5000.0f,
                                                                 filament::LightManager::EFFICIENCY_LED);
   cubeLightTransform->add_child(pointLightNode);
   graph->add_sunlight(lightColor, { 0, -1, -0.8 }, 300000.0f);
   return true;
}
