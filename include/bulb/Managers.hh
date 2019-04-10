#ifndef _MANAGERS_HH_
#define _MANAGERS_HH_
#include <memory>

#include "filament/Engine.h"
#include "utils/EntityManager.h"
#include "filament/TransformManager.h"
#include "filament/LightManager.h"
#include "filament/RenderableManager.h"

const filament::Engine::Backend DEFAULT_BACKEND = filament::Engine::Backend::OPENGL;

namespace bulb
{
   static filament::Engine::Backend SELECTED_BACKEND = DEFAULT_BACKEND;

   class Managers
   {
   public:
      static Managers& instance()
      //--------------------------------
      {
         static Managers the_instance;
         return the_instance;
      }

      Managers(Managers const&) = delete;
      Managers(Managers&&) = delete;
      Managers& operator=(Managers const&) = delete;
      Managers& operator=(Managers &&) = delete;

      std::shared_ptr<filament::Engine> engine;
      utils::EntityManager& entityManager;
      filament::TransformManager& transformManager;
      filament::RenderableManager& renderManager;
      filament::LightManager& lightManager;

   private:
      Managers() : engine(filament::Engine::create(SELECTED_BACKEND),
                                                   [](filament::Engine* p) { if (p) filament::Engine::destroy(&p); }),
                   entityManager(utils::EntityManager::get()),
                   transformManager(engine->getTransformManager()),
                   renderManager(engine->getRenderableManager()),
                   lightManager(engine->getLightManager()) {}
   };
}

#endif //_MANAGERS_HH_
