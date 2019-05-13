#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include "filament/Engine.h"
#include "backend/Platform.h"
#include "filament/Renderer.h"
#include "filament/Camera.h"
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
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#ifdef HAVE_SDL2
#include "SDL.h"
#include <SDL_syswm.h>

SDL_Window* window = nullptr;
#else
#include "Win.hh"
#endif

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

unsigned windowWidth =0, windowHeight =0;
void* nativeWindow = nullptr;
filament::SwapChain* swapChain = nullptr;
filament::View* view = nullptr;
filament::VertexBuffer* vb = nullptr, *texvb = nullptr;
filament::IndexBuffer* ib = nullptr, *texib = nullptr;
filament::Camera* perspectiveCamera = nullptr;

#ifndef HAVE_SDL2
std::unique_ptr<Win> window;
#endif
std::unique_ptr<bulb::SceneGraph> graph;

void event_loop();
bool create_graph();
void destroy_graph();

std::string bakedColor("samples/material/bakedColor"), bakedTexture("samples/material/bakedTexture"),
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
   windowWidth = newWidth;
   windowHeight = newHeight;
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
         bulb::SELECTED_BACKEND = filament::Engine::Backend::VULKAN;
   }
#ifdef HAVE_SDL2
   if (SDL_Init(SDL_INIT_EVENTS /*| SDL_INIT_VIDEO*/) < 0)
   {
      std::cerr << "Error initializing SDL\n";
      return 1;
   }
   SDL_DisplayMode dm;
   dm.w = dm.h = -1;
   if (SDL_GetCurrentDisplayMode(0, &dm) != 0)
   {
      SDL_Window* tmpwin = SDL_CreateWindow("Dummy", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1, 1,
                                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
      if (SDL_GetCurrentDisplayMode(0, &dm) != 0)
         dm.w = dm.h = -1;
      if (tmpwin != nullptr)
         SDL_DestroyWindow(tmpwin);
   }
   if ( (dm.w > 0) && (dm.h > 0) )
   {
      windowWidth = std::max(dm.w - 20, static_cast<int>(windowWidth));
      windowHeight = std::max(dm.h - 20, static_cast<int>(windowHeight));
   }
   window = SDL_CreateWindow("SimpleTest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
   if (window == nullptr)
   {
      std::cerr << "Error creating SDL window " << SDL_GetError() << std::endl;
      return 1;
   }
   SDL_SysWMinfo wmi;
   SDL_VERSION(&wmi.version);
   if (SDL_GetWindowWMInfo(window, &wmi) == SDL_TRUE)
   {
#ifdef WIN32
      nativeWindow = reinterpret_cast<void*>(wmi.info.win.window);
#else
      nativeWindow = reinterpret_cast<void*>(wmi.info.x11.window);
#endif
   }
   else
   {
      std::cerr << "Error getting native window handle from SDL\n";
      return 1;
   }
   if (! create_graph())
   {
      std::cerr << "Error creating scene graph\n";
      std::exit(1);
   }
#else
   std::vector<unsigned char> v;
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
   nativeWindow = reinterpret_cast<void*>(window->native_window());
#endif
   event_loop();
}

#ifdef HAVE_SDL2
bool process_sdl_events(int maxEvents =16)
//-----------------------
{
   SDL_Event ev;
   int count = 0;
   static filament::math::float3 look(0, 0, -1);
   while (SDL_PollEvent(&ev))
   {
      if (count++ > maxEvents) return true;
      switch (ev.type)
      {
         case SDL_QUIT: return false;
         case SDL_KEYUP:
         {
            int key = ev.key.keysym.scancode;
            bool isCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            bool isShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            if ( (key == SDL_SCANCODE_ESCAPE) || ( (key == SDL_SCANCODE_Q) && (isCtrl) ) )
               return false;
//            switch (key)
//            {
//               case SDL_SCANCODE_LEFT:
//                  break;
//
//               case SDL_SCANCODE_RIGHT:
//                  break;
//            }
         }
      }
   }
   return true;
}
#endif

void event_loop()
//---------------
{
   double fps = 40;
   long spf = 1000000000L / fps;
   auto frame_start = std::chrono::high_resolution_clock::now();
   bool is_quit = false;
   do
   {
#ifdef HAVE_SDL2
      if (window == nullptr) continue;
      if (! process_sdl_events())
         break;
#else
      if (! window)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         continue;
      }
      if (! window->process_event())
         break;
#endif
      if (! graph) continue;
      auto frame_end = std::chrono::high_resolution_clock::now();
      long elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_end - frame_start).count(), last_duration;
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
#ifndef HAVE_SDL2
   window.reset();
#endif
}

void destroy_graph()
//--------------------
{
   std::shared_ptr<filament::Engine> engine = bulb::Managers::instance().engine;
   if (engine)
   {
      if (perspectiveCamera) engine->destroy(perspectiveCamera); perspectiveCamera = nullptr;
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
   swapChain = engine->createSwapChain(nativeWindow);
   view = engine->createView();
   view->setClearColor(filament::Color::toLinear({0, 0.5, 0, 0}));
   view->setClearTargets(false, true, false);
   view->setPostProcessingEnabled(true);
   view->setViewport({ 0, 0, windowWidth, windowHeight });
   perspectiveCamera = engine->createCamera();
   perspectiveCamera->lookAt({0, 0, 0}, {0, 0, -1}, {0, 1, 0});
   std::cout << "Camera position " << perspectiveCamera->getPosition() << "Forward " << perspectiveCamera->getForwardVector() << std::endl;
//   camera->setProjection(45, 16.0/9.0, 0.1, 1.0);
   perspectiveCamera->setProjection(50, double(windowWidth) / double(windowHeight), 0.0625, 10.0); //, filament::Camera::Fov::HORIZONTAL);
//   std::cout << "Contains (-0.15, 0.06, " << TRIANGLE_Z << ") " << (perspectiveCamera->getFrustum().contains({-0.15, 0.06, TRIANGLE_Z}) < 0) << std::endl;
   view->setCamera(perspectiveCamera);
   graph = std::make_unique<bulb::SceneGraph>(engine, swapChain, view);

   while (! graph->start_updating())  // Not really necessary here, but good practise for multithreaded updates to the graph
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
   bulb::Composite* root = graph->make_root("root");
   bulb::Material* colorMaterialNode = graph->make_material("Color Material", bakedColor.c_str());
   if (colorMaterialNode == nullptr)
   {
      std::cerr << "Error opening material file " << bakedColor << std::endl;
      return false;
   }
   root->add_child(colorMaterialNode);

   bulb::AffineTransform* triangleTransform1 = graph->make_affine_transform("Triangle 1 Transform", filament::math::mat3(1),
                                                                            filament::math::double3(-0.15, 0.06, 0),
                                                                            filament::math::double3(0.025, 0.025, 1));
   colorMaterialNode->add_child(triangleTransform1);
   vb = filament::VertexBuffer::Builder().bufferCount(1).vertexCount(3)
                                         .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, 0, 16)
                                         .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, 12, 16)
                                         .normalized(filament::VertexAttribute::COLOR)
                                         .build(*engine);
   ib = filament::IndexBuffer::Builder().indexCount(3).bufferType(filament::IndexBuffer::IndexType::USHORT).build(*engine);
   vb->setBufferAt(*engine, 0, filament::VertexBuffer::BufferDescriptor(TRIANGLE_VERTICES.data(), 48, nullptr));
   ib->setBuffer(*engine, filament::IndexBuffer::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr));
   bulb::Geometry* triangleNode1 = graph->make_geometry("Triangle 1");
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
   triangleTransform1->add_child(triangleTransform2);
   bulb::Geometry* triangleNode2 = graph->make_geometry("Triangle 2");
   triangleTransform2->add_child(triangleNode2);
   filament::RenderableManager::Builder(1).boundingBox(to_box(TRIANGLE_VERTICES))
                                          .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 3)
                                          .culling(false)
                                          .receiveShadows(false)
                                          .castShadows(false)
                                          .build(*engine, triangleNode2->get_renderable());

   bulb::Material* texMaterialNode = graph->make_material("Texture Material", bakedTexture.c_str());
   if (texMaterialNode == nullptr)
   {
      std::cerr << "Error opening material file " << bakedTexture << std::endl;
      return false;
   }
   root->add_child(texMaterialNode);
   int w, h, channels;
   unsigned char* img = stbi_load(textureFile.c_str(), &w, &h, &channels, 4);
   assert(img != nullptr);
   filament::Texture* texture = filament::Texture::Builder().width(uint32_t(w)).height(uint32_t(h)).
                                                             levels(1).sampler(filament::Texture::Sampler::SAMPLER_2D).
                                                             format(filament::backend::TextureFormat::RGBA8).build(*engine);
   filament::Texture::PixelBufferDescriptor buffer(img, size_t(w * h * 4),
                                                   filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE,
                                                   [](void* p, size_t size, void* user) { if (p) stbi_image_free(p); });
   texture->setImage(*engine, 0, std::move(buffer));
   texMaterialNode->setTexture("albedo", texture, filament::TextureSampler::MinFilter::LINEAR,
                               filament::TextureSampler::MagFilter::LINEAR);
   bulb::AffineTransform* triangleTransform3 = graph->make_affine_transform("Triangle 3 Transform", filament::math::mat3(1),
                                                                            filament::math::double3(0, 0, 0.08),
                                                                            filament::math::double3(0.025, 0.025, 1));
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
                                          .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, texvb,
                                                    texib, 0, 3).
                                          culling(false).receiveShadows(false).castShadows(false).
                                          build(*engine, triangleNode3->get_renderable());
   triangleTransform3->add_child(triangleNode3);

   create_cube(engine.get(), graph.get(), root);

   filament::Texture* backgroundTexture = graph->set_background(windowWidth, windowHeight);
   int imageWidth, imageHeight, imageChannels=3, reqChannels;
   filament::Texture::InternalFormat texFormat = backgroundTexture->getFormat();
   if (texFormat == filament::Texture::InternalFormat::RGBA8)
      reqChannels = 4;
   else
      reqChannels = 3;
   size_t size = windowWidth*windowHeight*reqChannels;
   unsigned char *data = stbi_load("samples/images/Filament_Logo.png", &imageWidth, &imageHeight, &imageChannels,
                                    reqChannels);
   if (data != nullptr)
   {
      unsigned char* pdata = new unsigned char[size];
      if ( (imageWidth != windowWidth) || (imageHeight != windowHeight) )
      {
         if (! stbir_resize_uint8(data, imageWidth, imageHeight, 0, pdata, windowWidth, windowHeight, 0,
                                    reqChannels))
         {
            delete[] pdata;
            pdata = nullptr;
            std::cerr << "Error resizing background image" << std::endl;
         }
      }
      else
      {
         memcpy(pdata, data, size);
         pdata = data;
      }
      stbi_image_free(data);
      if (pdata)
      {
         stbi_write_jpg("resized.jpg", windowWidth, windowHeight, 4, pdata, 9);
         graph->set_background_image(pdata, windowWidth, windowHeight, 4,
         [](void* p, size_t size, void* user)
         //----------------------------------
         {
            if (p)
            {
               unsigned char *data = static_cast<unsigned char *>(p);
               delete[] data;
            }
         });
      }
   }

   filament::LinearColor lightColor = filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(0.98f, 0.98f, 0.98f));
   graph->add_sunlight(lightColor, { 0, -1, -0.8 }, 200000.0f);
   graph->end_updating();
   return true;
}

filament::Material* cubeMaterial = nullptr;

bool create_cube(filament::Engine* engine, bulb::SceneGraph* graph, bulb::Composite* root)
//----------------------------------------------------------------------------------------
{
   filament::math::quat rotation = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(1, 1, 0)),
                                                                       bulb::pi<double>/4.0);
   bulb::AffineTransform* cubeTransform = graph->make_affine_transform("CubeTransform", rotation,
                                                                       filament::math::double3(0.2, -0.001, -0.5), 0.1f);
   root->add_child(cubeTransform);
   bulb::AssetReader& reader = bulb::AssetReader::instance();
   std::vector<char> materialData;
   reader.read_asset_vector("samples/material/cube", materialData);
//   reader.read_asset_vector("assets/bakedTexture", materialData);
   cubeMaterial = filament::Material::Builder().package(materialData.data(), materialData.size()).build(*engine);
   int w, h, channels;
   unsigned char* img = stbi_load(cubeTextureFile.c_str(), &w, &h, &channels, 4);
   if (! img)
   {
      std::cerr <<  "Error opening cube texture " << cubeTextureFile << std::endl;
      return false;
   }
   monoTexture = filament::Texture::Builder().width(uint32_t(w)).height(uint32_t(h)).
                                              levels(1).sampler(filament::Texture::Sampler::SAMPLER_2D).
                                              format(filament::backend::TextureFormat::RGBA8).
                                              build(*engine);
   filament::Texture::PixelBufferDescriptor buffer(img, size_t(w * h * 4),
                                                   filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE,
                                                   [](void* p, size_t size, void* user) { if (p) stbi_image_free(p); });
   monoTexture->setImage(*engine, 0, std::move(buffer));
   cubeMaterial->setDefaultParameter("albedo", monoTexture, monoSampler);
   cubeMaterial->setDefaultParameter("metallic", 1.0f);
   cubeMaterial->setDefaultParameter("roughness", 0.4f);
   cubeMaterial->setDefaultParameter("reflectance", 0.4f);
   cubeMaterial->setDefaultParameter("anisotropy", 0.0f);
   bulb::Geometry* cubeNode = graph->make_geometry("Cube");
   if (! cubeNode->open_filamesh(cubeModel.c_str(), cubeMaterial))
   {
      std::cerr << "Error opening filamesh mesh file " << cubeModel << std::endl;
      return false;
   }

   cubeTransform->add_child(cubeNode);
   filament::LinearColor lightColor = filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(0.5f, 0.5f, 0.5f));
   bulb::PositionalLight* cubeLight = graph->make_pointlight("Cube Light", lightColor,
                                                             filament::math::double3(0.15, 0.01, -0.35),
                                                             filament::math::float3(0, 0.1, -1), 5000.0f, 0.3f);
   root->add_child(cubeLight);
   return true;
}
