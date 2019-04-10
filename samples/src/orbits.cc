#include <iostream>
#include <iostream>
#include <fstream>
#include <tuple>
#include <vector>
#include <memory>
#include <chrono>

#include <cstdint>

#include "filament/Engine.h"
#include "filament/Renderer.h"
#include "filament/Scene.h"
#include "filament/View.h"
#include "filament/Material.h"
#include "filament/VertexBuffer.h"
#include "filament/IndexBuffer.h"
#include "filament/RenderableManager.h"
#include "filament/TransformManager.h"
#include "filament/Texture.h"
#ifdef HAVE_LIBPNG
#include "backend/PixelBufferDescriptor.h"
#include "image/LinearImage.h"
#include <imageio/ImageEncoder.h>
#endif
#include "utils/EntityManager.h"
#include "utils/Entity.h"
#include "math/quat.h"

#include "bulb/SceneGraph.hh"
#include "bulb/Managers.hh"
#include "bulb/AssetReader.hh"
#include "bulb/ut.hh"
#include "bulb/nodes/Geometry.hh"
#include "bulb/nodes/AffineTransform.hh"
#include "bulb/nodes/MultiGeometry.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#ifdef HAVE_LIBPNG
#define TINYEXR_IMPLEMENTATION
#include "exr/tinyexr.h"
#endif

#include "orbitanim.hh"
//#undef HAVE_SDL2
#include "CameraManipulator.h"

#ifdef HAVE_SDL2
#include "SDL.h"
#include <SDL_syswm.h>

SDL_Window* window = nullptr;
#else
#include "Win.hh"

std::unique_ptr<Win> window;
#endif

void* nativeWindow = nullptr;
int window_width =1024, window_height =768;
bool isFreeze = false;
#ifdef HAVE_LIBPNG
bool isScreenShot = false;
#endif

filament::SwapChain* swapChain = nullptr;
filament::View* view = nullptr;
filament::VertexBuffer* vb = nullptr, *texvb = nullptr;
filament::IndexBuffer* ib = nullptr, *texib = nullptr;
filament::Camera* camera = nullptr;
CameraManipulator* cameraManipulator = nullptr;

double minor_axis(double majorAxis, double eccentricity) { return majorAxis*sqrt(1 - eccentricity*eccentricity); }

#define MERCURY_DISTANCE 0.387
#define MERCURY_ECCENTRICITY 0.206
#define MERCURY_SCALE 0.019
#define VENUS_DISTANCE 0.723
#define VENUS_ECCENTRICITY 0.0068
#define VENUS_SCALE 0.0475
#define EARTH_ECCENTRICITY 0.0167
#define EARTH_SCALE 0.05
#define MOON_EARTH_DISTANCE 0.15
#define MOON_ECCENTRICITY 0.0549
#define MOON_SCALE 0.018
#define MARS_DISTANCE 1.524
#define MARS_ECCENTRICITY 0.0934
#define MARS_SCALE 0.0265
#define STATION_MARS_DISTANCE 0.05
#define STATION_SCALE 0.005
#define JUPITER_DISTANCE 3.2 //5.2 (The solar system was reshaped by a marauding mini black hole)
#define JUPITER_ECCENTRICITY 0.0485
#define JUPITER_SCALE 0.16
#define JUPITER_MONO_DISTANCE 0.5
#define MONOLITH_SCALE 0.02
#define TEAPOT_DISTANCE 4.5
#define TEAPOT_ECCENTRICITY 0.1
#define TEAPOT_SCALE 0.15

std::unique_ptr<bulb::SceneGraph> graph;
filament::math::double3 cameraTranslation;
int toLoadCount = 10;
bulb::Composite* root;
bulb::AffineTransform* mercuryTransform, *venusTransform, *earthTransform,  *moonTransform, *moonEarthOrbit,
      *marsTransform, *stationTransform, *marsStationOrbit, *jupiterTransform,
      *monoJupiterOrbit, *monoTransform, *monoLightTransform, *teapotTransform;
bulb::PositionalLight* monoLightNode = nullptr;
bulb::MultiGeometry *sol =nullptr, *mercury =nullptr, *venus =nullptr, *earth =nullptr, *moon =nullptr,
                    *mars =nullptr, *station =nullptr, *jupiter =nullptr, *teapot = nullptr, *monolith =nullptr;
// bulb::Geometry* monolith = nullptr;
std::vector<bulb::Transform*> animationTransforms;
EllipticAnimator ellipticAnimator;
EllipticRotationParams  mercuryAnimation(47.0, MERCURY_DISTANCE, minor_axis(MERCURY_DISTANCE, MERCURY_ECCENTRICITY)),
                        venusAnimation(35.0, VENUS_DISTANCE, minor_axis(VENUS_DISTANCE, VENUS_ECCENTRICITY), false),
                        earthAnimation(29.0, 1.0, minor_axis(1.0, EARTH_ECCENTRICITY)),
                        moonAnimation(100.0, MOON_EARTH_DISTANCE, minor_axis(MOON_EARTH_DISTANCE, MOON_ECCENTRICITY)),
                        marsAnimation(24.0, MARS_DISTANCE, minor_axis(MARS_DISTANCE, MARS_ECCENTRICITY)),
                        stationAnimation(45.0, STATION_MARS_DISTANCE, minor_axis(STATION_MARS_DISTANCE, 0.001)),
                        jupiterAnimation(10.0, JUPITER_DISTANCE, minor_axis(JUPITER_DISTANCE, JUPITER_ECCENTRICITY)),
                        monoAnimation(30.0, JUPITER_MONO_DISTANCE, minor_axis(JUPITER_MONO_DISTANCE, 0.0001)),
                        teapotAnimation(9.0, TEAPOT_DISTANCE, minor_axis(TEAPOT_DISTANCE, TEAPOT_ECCENTRICITY));
//CircularRotationParams stationSpinAnimation(10.0);

void event_loop();
bool create_graph();
void destroy_graph();
int load_a_node();
#ifdef HAVE_SDL2
void help();
#endif

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
      }
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
      window_width = std::max(dm.w - 20, window_width);
      window_height = std::max(dm.h - 20, window_height);
   }
   window = SDL_CreateWindow("Orbits", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height,
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
   nativeWindow = static_cast<void*>(window->native_window());
#endif
   event_loop();
}

#ifdef HAVE_SDL2
bool process_sdl_events(int maxEvents=16);
#endif

#ifdef HAVE_LIBPNG
template<typename T>
static image::LinearImage toLinear(size_t w, size_t h, size_t bpr, const uint8_t* src)
{
   image::LinearImage result(w, h, 3);
   filament::math::float3* d = reinterpret_cast<filament::math::float3*>(result.getPixelRef(0, 0));
   for (size_t y = 0; y < h; ++y)
   {
      T const* p = reinterpret_cast<T const*>(src + y * bpr);
      for (size_t x = 0; x < w; ++x, p += 3)
      {
         filament::math::float3 sRGB(p[0], p[1], p[2]);
         sRGB /= std::numeric_limits<T>::max();
         *d++ = sRGB;
      }
   }
   return result;
}


void on_post_render(filament::Engine* engine, filament::View* view, filament::Renderer* renderer,
                 filament::Scene* scene, void*)
//-------------------------------------------------------------------------------------------
{
   uint8_t* pixels = new uint8_t[window_width * window_height * 3];
   filament::backend::PixelBufferDescriptor buffer(pixels, window_width * window_height * 3,
                                                   filament::backend::PixelBufferDescriptor::PixelDataFormat::RGB,
                                                   filament::backend::PixelBufferDescriptor::PixelDataType::UBYTE,
                                                   [](void* buffer, size_t size, void* user)
                                                   {
                                                      image::LinearImage image(toLinear<uint8_t>(window_width,
                                                         window_height, window_width * 3, static_cast<uint8_t*>(buffer)));
                                                      std::ofstream outputStream("screenshot.png",
                                                                                 std::ios::binary | std::ios::trunc);
                                                      image::ImageEncoder::encode(outputStream,
                                                                                  image::ImageEncoder::Format::PNG,
                                                                                  image, "", "screenshot.png");

                                                      delete[] static_cast<uint8_t*>(buffer);
                                                   });
   renderer->readPixels(0, 0, window_width, window_height, std::move(buffer));
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
#ifdef HAVE_LIBPNG
         if (isScreenShot)
         {
            graph->render(on_post_render);
            isScreenShot = false;
         }
         else
#endif
            graph->render();
         frame_start = frame_end;
         last_duration = elapsed;
      }
      else //if (elapsed < spf)
      {
         if (toLoadCount > 0)
         {
            load_a_node();
            animationTransforms = graph->get_animated_transforms();
         }
         if ( (isFreeze) || (animationTransforms.empty()) )
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
         else
         {
            for (bulb::Transform* transform : animationTransforms)
            {
               while (!graph->start_updating());
               transform->animate(ellipticAnimator);
               graph->end_updating(true);

            }
         }
      }
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
      filament::Fence::waitAndDestroy(engine->createFence());
      if (camera) engine->destroy(camera); camera = nullptr;
      if (swapChain) engine->destroy(swapChain);
      if (view) engine->destroy(view);
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

filament::Texture* monoTexture;
filament::TextureSampler monoSampler(filament::TextureSampler::MinFilter::LINEAR,
                                     filament::TextureSampler::MagFilter::LINEAR);
filament::Material* monoMaterial = nullptr;

void rotateY(float degrees);

std::pair<bulb::AffineTransform*, bulb::MultiGeometry*> make_satellite(bulb::Composite* parent, const char* nodeName,
                                                                        const char* nodeTransformName, const char* modelPath,
                                                                        filament::math::quat& R,
                                                                        filament::math::double3 t, double scale,
                                                                        EllipticRotationParams* animationParams = nullptr)
//------------------------------------------------------------------------------------------------------------------------
{
   bulb::AffineTransform* transformNode = graph->make_affine_transform(nodeTransformName, R, t, scale, animationParams);
   parent->add_child(transformNode);
   bulb::MultiGeometry* planet = graph->make_multi_geometry(nodeName, modelPath);
   transformNode->add_child(planet);
   return std::make_pair(transformNode, planet);
}

bool create_graph()
//------------------------------------
{
   std::shared_ptr<filament::Engine> engine = bulb::Managers::instance().engine;
   swapChain = engine->createSwapChain(nativeWindow);
   view = engine->createView();
   view->setClearColor({0, 0, 0, 1.0});
#ifdef HAVE_SDL2
   SDL_GetWindowSize(window, &window_width, &window_height);
#else
   window_width = window->width; window_height = window->height;
#endif
   view->setViewport({0, 0, static_cast<uint32_t>(window_width), static_cast<uint32_t>(window_height)});
   camera = engine->createCamera();
   camera->setProjection(70, double(window_width) / double(window_height), 0.0625, 20);
   cameraManipulator = new CameraManipulator(camera, static_cast<size_t>(window_width),
                                             static_cast<size_t>(window_height));
   cameraManipulator->lookAt(filament::math::double3(0, 0, -4.2), filament::math::double3(0, 0, -1));

   cameraTranslation = cameraManipulator->get_translation();
   view->setCamera(camera);
   graph = std::make_unique<bulb::SceneGraph>(engine, swapChain, view);

   while (!graph->start_updating())  // Not really necessary here, but good practise for multithreaded updates to the graph
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
   if (root == nullptr)
      root = graph->make_root("root");
   filament::math::float3 lightDir({ 0, -1, 1 });
   filament::LinearColor lightColor = filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(0.98f, 0.92f, 0.89f));
   graph->add_sunlight(lightColor, lightDir, 800000.0f);
//   graph->add_directional_light("delight", lightColor, lightDir);
   graph->end_updating();
   return (root != nullptr);
}


// Filament gltf loader not multithreaded.
int load_a_node()
//--------------
{
   if (sol == nullptr)
   {
      sol = graph->make_multi_geometry("Sun", "samples/model/Sol/Sol.gltf",
                                       new bulb::AffineTransform("SunTransform", filament::math::mat3(1.0),
                                                                 filament::math::double3(0, 0, 0.5), 0.5));
      if (sol == nullptr)
      {
         std::cerr << "The sun has expired\n";
         std::exit(1);
      }
      root->add_child(sol);
      toLoadCount--;
      return toLoadCount;
   }

   static filament::math::quat I(1, 0, 0, 0);
   static filament::math::quat yRotation = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0, 1, 0)), 0);

   if (mercury == nullptr)
   {
      bulb::AffineTransform* mercuryOrbit = graph->make_affine_transform("MercuryOrbit", yRotation,
                                                                         filament::math::double3(MERCURY_DISTANCE, 0, 0),
                                                                         1.0f,
                                                                         &mercuryAnimation,
                                                                         bulb::MatrixTransformOrder::STR);
      root->add_child(mercuryOrbit);
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0, 0, 1)), 0.03682645);
      std::tie(mercuryTransform, mercury) = make_satellite(mercuryOrbit, "Mercury", "MercuryTransform",
                                                           "samples/model/Mercury/Mercury.gltf",
                                                           Q, filament::math::double3(0, 0, 0), MERCURY_SCALE);
      if (  (mercuryTransform == nullptr) || (mercury == nullptr) )
      {
         std::cerr << "Mercury has been destroyed by the Death Star\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }
   if (venus == nullptr)
   {
      bulb::AffineTransform* venusOrbit = graph->make_affine_transform("VenusOrbit", yRotation,
                                                                     filament::math::double3(VENUS_DISTANCE, 0, 0), 1.0f,
                                                                     &venusAnimation,
                                                                     bulb::MatrixTransformOrder::STR);
      root->add_child(venusOrbit);
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0, 0, 1)), 2.64);
      std::tie(venusTransform, venus) = make_satellite(venusOrbit, "Venus", "VenusTransform", "samples/model/Venus/Venus.gltf",
                                                      Q, filament::math::double3(0, 0, 0), VENUS_SCALE);
      if (  (venusTransform == nullptr) || (venus == nullptr) )
      {
         std::cerr << "Venus has been destroyed by the Death Star\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }

   if (earth == nullptr)
   {
      bulb::AffineTransform* earthOrbit = graph->make_affine_transform("EarthOrbit", yRotation,
                                                                     filament::math::double3(1.0, 0, 0), 1.0,
                                                                     &earthAnimation, bulb::MatrixTransformOrder::STR);
      root->add_child(earthOrbit);
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0, 0, 1)), 0.40910518);
      std::tie(earthTransform, earth) = make_satellite(earthOrbit, "Earth", "EarthTransform", "samples/model/Earth/Earth.gltf",
                                                      Q, filament::math::double3(0, 0, 0), EARTH_SCALE);
      if (  (earthTransform == nullptr) || (earth == nullptr) )
      {
         std::cerr << "Earth has been destroyed by the Death Star\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }

   if (moon == nullptr)
   {
      moonAnimation.a *= 1.0/EARTH_SCALE; moonAnimation.b *= 1.0/EARTH_SCALE;
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0.2, 1, 0.2)), 0.40910518);
      moonEarthOrbit = graph->make_affine_transform("MoonEarthOrbit", Q,
                                                    1.0/EARTH_SCALE*filament::math::double3(MOON_EARTH_DISTANCE, 0, 0),
                                                    1.0, &moonAnimation, bulb::MatrixTransformOrder::STR);
      earthTransform->add_child(moonEarthOrbit);
      std::tie(moonTransform, moon) = make_satellite(moonEarthOrbit, "Moon", "MoonTransform", "samples/model/Luna/Luna.gltf",
                                                   I, filament::math::double3(0, 0, 0), 1.0/EARTH_SCALE*MOON_SCALE);
      if (  (moonTransform == nullptr) || (moon == nullptr) )
      {
         std::cerr << "The Moon has been destroyed by the Death Star\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }

   if (mars == nullptr)
   {
      bulb::AffineTransform* marsOrbit = graph->make_affine_transform("MarsOrbit", yRotation,
                                                                     filament::math::double3(MARS_DISTANCE, 0, 0), 1.0,
                                                                     &marsAnimation, bulb::MatrixTransformOrder::STR);
      root->add_child(marsOrbit);
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0, 0, 1)), 0.436332);
      std::tie(marsTransform, mars) = make_satellite(marsOrbit, "Mars", "MarsTransform", "samples/model/Mars/Mars.gltf",
                                                   Q, filament::math::double3(0, 0, 0), MARS_SCALE);
      if (  (marsTransform == nullptr) || (mars == nullptr) )
      {
         std::cerr << "Mars has been destroyed by the Death Star\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }
   if (station == nullptr)
   {
      stationAnimation.a *= 1.0/MARS_SCALE; stationAnimation.b *= 1.0/MARS_SCALE;
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0.2, 1, 0.2)), 0.40910518);
      marsStationOrbit = graph->make_affine_transform("MarsStationOrbit", Q,
                                                      1.0/MARS_SCALE*filament::math::double3(STATION_MARS_DISTANCE, 0, 0),
                                                      1.0, &stationAnimation, bulb::MatrixTransformOrder::STR);
      marsTransform->add_child(marsStationOrbit);
      std::tie(stationTransform, station) = make_satellite(marsStationOrbit, "Station", "StationTransform",
                                                         "samples/model/Station/scene.gltf",
                                                         I, filament::math::double3(0, 0, 0), 1.0/MARS_SCALE*STATION_SCALE);
//      bulb::AffineTransform* stationSpin = graph->make_affine_transform("MarsStationSpin", yRotation,
//                                                                       filament::math::double3(0, 0, 0), 1.0,
//                                                                        &stationSpinAnimation,
//                                                                        bulb::MatrixTransformOrder::STR);
//      station->set_transform(stationSpin);
      if (  (stationTransform == nullptr) || (station == nullptr) )
      {
         std::cerr << "The Mars station has been destroyed by NASA mixing up metres and feet.\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }

   if (jupiter == nullptr)
   {
      bulb::AffineTransform* jupiterOrbit = graph->make_affine_transform("JupiterOrbit", yRotation,
                                                                        filament::math::double3(JUPITER_DISTANCE, 0, 0),
                                                                        1.0, &jupiterAnimation,
                                                                        bulb::MatrixTransformOrder::STR);
      root->add_child(jupiterOrbit);
      std::tie(jupiterTransform, jupiter) = make_satellite(jupiterOrbit, "Jupiter", "JupiterTransform",
                                                           "samples/model/Jupiter/Jupiter.gltf",
                                                           I, filament::math::double3(0, 0, 0), JUPITER_SCALE);
      if ( (jupiterTransform == nullptr) || (jupiter == nullptr) )
      {
         std::cerr << "Jupiter has been destroyed by the Death Star\n";
         std::exit(1);
      }
      toLoadCount--;
      return toLoadCount;
   }
   if (monolith == nullptr)
   {
      monoAnimation.a *= 1.0/JUPITER_SCALE; monoAnimation.b *= 1.0/JUPITER_SCALE;
      //filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(0.2, 1, 0.2)), 0.5);
      monoJupiterOrbit = graph->make_affine_transform("MonolithJupiterOrbit", yRotation,
                                                      1.0/JUPITER_SCALE*filament::math::double3(JUPITER_MONO_DISTANCE, 0, 0),
                                                      1.0, &monoAnimation, bulb::MatrixTransformOrder::STR);
      jupiterTransform->add_child(monoJupiterOrbit);
      bulb::AffineTransform* jupiterOrbit = dynamic_cast<bulb::AffineTransform*>(graph->get_node("JupiterOrbit"));
      std::tie(monoTransform, monolith) = make_satellite(monoJupiterOrbit, "Monolith", "MonoTransform",
                                                         "samples/model/Monolith/Monolith.gltf",
                                                         I, filament::math::double3(0, 0, 0),
                                                         1.0/JUPITER_SCALE*MONOLITH_SCALE);

      if ( (monoTransform == nullptr) || (monolith == nullptr) || (! monolith->get_renderable()) )
         std::cerr << "The alien monolith has grown tired of waiting and returned to base.\n";
      else
      {
         monoLightTransform = graph->make_affine_transform("Cube Light Transform", filament::math::mat3(1),
                                                           filament::math::double3(0, 0, 0),
                                                           filament::math::double3(1, 1, 1));
         monoTransform->add_child(monoLightTransform);
         filament::LinearColor lightColor = filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(0.98f, 0.92f, 0.89f));
         monoLightNode = graph->make_pointlight("Point", lightColor, filament::math::float3({0, 0, 0.5}),
                                                filament::math::float3({0, 0, -1}), 60000.0f,
                                                filament::LightManager::EFFICIENCY_LED);
         monoLightTransform->add_child(monoLightNode);
      }
      toLoadCount--;
#ifdef HAVE_SDL2
      help();
#endif
      return toLoadCount;
   }
   if (teapot == nullptr)
   {
      bulb::AffineTransform* teapotOrbit = graph->make_affine_transform("TeapotOrbit", yRotation,
                                                                         filament::math::double3(TEAPOT_DISTANCE, 0, 0),
                                                                         1.0, &teapotAnimation,
                                                                         bulb::MatrixTransformOrder::STR);
      root->add_child(teapotOrbit);
      filament::math::quat Q = filament::math::quat::fromAxisAngle(normalize(filament::math::double3(1, 0, 0)),
                                                                   bulb::pi<double>);
      std::tie(teapotTransform, teapot) = make_satellite(teapotOrbit, "Teapot", "TeapotTransform",
                                                         "samples/model/Teapot/Teapot.gltf",
                                                         Q, filament::math::double3(0, 0, 0), TEAPOT_SCALE);
      toLoadCount--;
      return toLoadCount;
   }
   return toLoadCount;
}

#ifdef HAVE_SDL2
void help()
//--------
{
   SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Help",
                            "Left Arrow - Translate Left\nRight Arrow - Translate Right\n"
                            "Up Arrow - Translate Up\nDown Arrow - Translate Down\n"
                            "W - Zoom in (Z Translate)\nS - Zoom out\n"
                            "2 - Rotate up\nX - Rotate down\n"
                            "A - Rotate left\nD - Rotate right\n"
                            "Home - Reset\nF - Freeze\nF1 - Help\n"
#ifdef HAVE_LIBPNG
                            "Ctrl-P, Ctrl-PrntScr - Screendump\n"
#endif
                            "Esc,Ctrl-Q - Quit"
         ,window);
}

bool process_sdl_events(int maxEvents)
//-----------------------
{
   SDL_Event ev;
   int count = 0;
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
            switch (key)
            {
               case SDL_SCANCODE_LEFT:
                  cameraTranslation[0] -= 0.05;
                  cameraManipulator->translate(cameraTranslation);
                  break;
               case SDL_SCANCODE_RIGHT:
                  cameraTranslation[0] += 0.05;
                  cameraManipulator->translate(cameraTranslation);
                  break;
               case SDL_SCANCODE_W:
                  cameraTranslation[2] += 0.1;
                  cameraManipulator->translate(cameraTranslation);
                  break;
               case SDL_SCANCODE_S:
                  cameraTranslation[2] -= 0.1;
                  cameraManipulator->translate(cameraTranslation);
                  break;
               case SDL_SCANCODE_UP:
                  cameraTranslation[1] -= 0.05;
                  cameraManipulator->translate(cameraTranslation);
                  break;
               case SDL_SCANCODE_DOWN:
                  cameraTranslation[1] += 0.05;
                  cameraManipulator->translate(cameraTranslation);
                  break;
               case SDL_SCANCODE_A:
                  cameraManipulator->rotate(filament::math::double2(-5, 0));
                  cameraTranslation = cameraManipulator->get_translation();
                  break;
               case SDL_SCANCODE_D:
                  cameraManipulator->rotate(filament::math::double2(5, 0));
                  cameraTranslation = cameraManipulator->get_translation();
                  break;
               case SDL_SCANCODE_2:
                  cameraManipulator->rotate(filament::math::double2(0, 5));
                  cameraTranslation = cameraManipulator->get_translation();
                  break;
               case SDL_SCANCODE_X:
                  cameraManipulator->rotate(filament::math::double2(0, -5));
                  cameraTranslation = cameraManipulator->get_translation();
                  break;
               case SDL_SCANCODE_HOME:
                  cameraManipulator->lookAt(filament::math::double3(0, 0, -6.5), filament::math::double3(0, 0, -1));
                  cameraTranslation = cameraManipulator->get_translation();
                  break;
               case SDL_SCANCODE_F:
                  isFreeze = !isFreeze;
                  break;
#ifdef HAVE_LIBPNG
               case SDL_SCANCODE_PRINTSCREEN:
                  if (isCtrl)
                     isScreenShot = true;
                  break;
#endif
               case SDL_SCANCODE_F1:
                  help();
            }
            break;
         }
/*
         case SDL_WINDOWEVENT:
         {
            switch (ev.window.event)
            {
               case SDL_WINDOWEVENT_RESIZED:
                  SDL_Log("Window %d resized to %dx%d", ev.window.windowID, ev.window.data1, ev.window.data2);
                  if ( (window_width != ev.window.data1) || (window_height != ev.window.data2) )
                  {
                     window_resized(window_width, window_height, ev.window.data1, ev.window.data2);
                     window_width = ev.window.data1;
                     window_height = ev.window.data2;
                  }
                  break;
               case SDL_WINDOWEVENT_SIZE_CHANGED:
                  SDL_Log("Window %d size changed to %dx%d", ev.window.windowID, ev.window.data1, ev.window.data2);
                  if ( (window_width != ev.window.data1) || (window_height != ev.window.data2) )
                  {
                     window_resized(window_width, window_height, ev.window.data1, ev.window.data2);
                     window_width = ev.window.data1;
                     window_height = ev.window.data2;
                  }
                  break;
            }
         }
*/
         default:
            break;
      }
   }
   return true;
}

#endif
