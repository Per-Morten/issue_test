#ifndef NOX_ECS_COMPONENT_H_
#define NOX_ECS_COMPONENT_H_
#include <nox/common/types.h>
#include <nox/ecs/EntityId.h>
#include <nox/ecs/Event.h>
#include <nox/event/Event.h>

#include <json/value.h>

namespace nox
{
    namespace ecs
    {
        class EntityManager;
    }
}

namespace nox
{
    namespace ecs
    {
        /**
         * @brief      Base class for all components. This is the only "type"
         *             that the EntityManager can interact with.
         */
        class Component
        {
        public:
            /**
             * @brief      Used to identify which entity this component belongs
             *             to.
             */
            EntityId id;

            /**
             * @brief      Pointer to the entity manager this component belongs
             *             to.
             */
            EntityManager* entityManager{};

            /**
             * @brief      Deleted as creating components without id's are
             *             illegal.
             */
            Component() = delete;

            /**
             * @brief      Creates a component with the given entityId.
             *
             * @param      entityId       the id of the entity this component is
             *                            tied to.
             * @param      entityManager  The entity manager this component is
             *                            handled by.
             */
            inline
            Component(const EntityId& entityId,
                      EntityManager* entityManager);

            /**
             * @brief      Copying components is illegal, they are only movable.
             */
            Component(const Component&) = delete;

            /**
             * @brief      Copying components is illegal, they are only movable.
             */
            Component& operator=(const Component&) = delete;

            /**
             * @brief      Components are only movable.
             */
            Component(Component&&) = default;

            /**
             * @brief      Components are only movable.
             */
            Component& operator=(Component&&) = default;

            /**
             * @brief      Overridable initialize function.
             *
             * @param[in]  value  used to initialize the component.
             */
            void
            initialize(const Json::Value& /*value*/) {}


            /**
             * @brief      Overridable awake function.
             */
            void
            awake() {}

            /**
             * @brief      Overridable activate function.
             */
            void
            activate() {}

            /**
             * @brief      Overridable deactivate function.
             */
            void
            deactivate() {}

            /**
             * @brief      Overridable hibernate function.
             */
            void
            hibernate() {}

            /**
             * @brief      Overridable update function.
             *
             * @param[in]  deltaTime  The delta time.
             */
            void
            update(const nox::Duration& /*deltaTime*/) {}

            /**
             * @brief      Overridable receiveLogicEvent function. For receiving
             *             events from the rest of the engine.
             *
             * @param[in]  event  The event.
             */
            void
            receiveLogicEvent(const std::shared_ptr<nox::event::Event>& /*event*/) {}

            /**
             * @brief      Overridable receiveEntityEvent function. For
             *             receiving events from other components or parts of
             *             the ecs.
             *
             * @param[in]  event  The event.
             */
            void
            receiveEntityEvent(const ecs::Event& /*event*/) {}
        };
    }
}

#include <nox/ecs/Component.ipp>
#endif
