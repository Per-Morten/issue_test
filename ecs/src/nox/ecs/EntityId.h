#ifndef NOX_ECS_ENTITYID_H_
#define NOX_ECS_ENTITYID_H_
#include <cstddef>

namespace nox
{
    namespace ecs
    {
        /**
         * @brief Used to identify entities across the ECS.
         *        Unique per entity.
         */
        using EntityId = std::size_t;
    }
}

#endif
