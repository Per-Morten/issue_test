#include <nox/ecs/ComponentCollection.h>

#include <cstdlib>

nox::ecs::ComponentCollection::ComponentCollection(const MetaInformation& info)
    : info(info)
    , gen(0)
    , active(static_cast<Byte*>(std::malloc((GROWTH_FACTOR + 1) * info.size))) // + 1 for swap area.
    , inactive(active)
    , hibernating(active)
    , memory(active)
    , cap(active + (GROWTH_FACTOR * info.size))
{
}

nox::ecs::ComponentCollection::ComponentCollection(ComponentCollection&& source)
    : info(std::move(source.info))
    , gen(std::move(source.gen))
    , componentMap(std::move(source.componentMap))
    , active(std::move(source.active))
    , inactive(std::move(source.inactive))
    , hibernating(std::move(source.hibernating))
    , memory(std::move(source.memory))
    , cap(std::move(source.cap))
{
    source.active = nullptr;
    source.inactive = nullptr;
    source.hibernating = nullptr;
    source.memory = nullptr;
    source.cap = nullptr;
    source.gen = 0;
}

nox::ecs::ComponentCollection&
nox::ecs::ComponentCollection::operator=(ComponentCollection&& source)
{
    if (this != &source)
    {
        this->destroyRange(this->active, this->memory);
        std::free(this->active);
        this->info = std::move(source.info);
        this->gen = std::move(source.gen);
        this->componentMap = std::move(source.componentMap);
        this->active = std::move(source.active);
        this->inactive = std::move(source.inactive);
        this->hibernating = std::move(source.hibernating);
        this->memory = std::move(source.memory);
        this->cap = std::move(source.cap);

        source.active = nullptr;
        source.inactive = nullptr;
        source.hibernating = nullptr;
        source.memory = nullptr;
        source.cap = nullptr;
        source.gen = 0;
    }

    return *this;
}

nox::ecs::ComponentCollection::~ComponentCollection()
{
    auto begin = this->active;
    const auto end = this->memory;

    while (begin != end)
    {
        auto ptr = this->cast(begin);
        this->info.destruct(ptr);
        begin += this->info.size;
    }

    std::free(this->active);
    this->active = nullptr;
    this->inactive = nullptr;
    this->hibernating = nullptr;
    this->memory = nullptr;
    this->cap = nullptr;
}

void
nox::ecs::ComponentCollection::create(const EntityId& id,
                                      EntityManager* manager)
{
    if (this->size() >= this->capacity())
    {
        this->reallocate();
    }

    auto itr = this->findBefore(id);
    this->componentMap.insert(itr, { id, reinterpret_cast<Component*>(this->memory) });

    this->info.construct(this->cast(this->memory), id, manager);
    this->memory += this->info.size;
}

void
nox::ecs::ComponentCollection::adopt(Component& component)
{
    if (this->size() >= this->capacity())
    {
        this->reallocate();
    }

    auto itr = this->findBefore(component.id);
    this->componentMap.insert(itr, { component.id, reinterpret_cast<Component*>(this->memory) });

    this->info.moveConstruct(this->cast(this->memory), &component);
    this->memory += this->info.size;
}

void
nox::ecs::ComponentCollection::initialize(const EntityId& id,
                                          const Json::Value& value)
{
    if (!this->info.initialize)
    {
        return;
    }

    auto component = this->find(id);
    if (component != std::end(componentMap))
    {
        this->info.initialize(component->component, value);
    }
}

void
nox::ecs::ComponentCollection::awake(const EntityId& id)
{
    auto component = this->find(id);

    if (component != std::end(componentMap))
    {
        auto ptr = this->cast(this->hibernating);
        this->hibernating += this->info.size;
        this->swap(component->component, ptr);

        component->component = ptr;

        if (this->info.awake)
        {
            this->info.awake(component->component);
        }
    }
}

void
nox::ecs::ComponentCollection::activate(const EntityId& id)
{
    auto component = this->find(id);

    if (component != std::end(componentMap))
    {
        auto ptr = this->cast(this->inactive);
        this->inactive += this->info.size;
        this->swap(component->component, ptr);

        component->component = ptr;

        if (this->info.activate)
        {
            this->info.activate(component->component);
        }
    }
}

void
nox::ecs::ComponentCollection::deactivate(const EntityId& id)
{
    auto component = this->find(id);

    if (component != std::end(componentMap))
    {
        this->inactive -= this->info.size;
        auto ptr = this->cast(this->inactive);
        this->swap(component->component, ptr);

        component->component = ptr;

        if (this->info.deactivate)
        {
            this->info.deactivate(ptr);
        }
    }
}

void
nox::ecs::ComponentCollection::hibernate(const EntityId& id)
{
    auto component = this->find(id);

    if (component != std::end(componentMap))
    {
        this->hibernating -= this->info.size;
        auto ptr = this->cast(this->hibernating);
        this->swap(component->component, ptr);

        component->component = ptr;

        if (this->info.hibernate)
        {
            this->info.hibernate(ptr);
        }
    }
}

void
nox::ecs::ComponentCollection::remove(const EntityId& id)
{
    auto component = this->find(id);

    if (component != std::end(componentMap))
    {
        this->memory -= this->info.size;
        auto ptr = this->cast(this->memory);

        this->info.moveAssign(component->component, ptr);
        this->info.destruct(ptr);
        this->componentMap.erase(component);

        auto itr = find(component->id);
        itr->component = component->component;

        this->gen++;
    }
}

void
nox::ecs::ComponentCollection::update(const nox::Duration& duration)
{
    if (this->info.update)
    {
        auto begin = this->cast(this->active);
        auto end = this->cast(this->inactive);

        this->info.update(begin, end, duration);
    }
}

void
nox::ecs::ComponentCollection::receiveLogicEvent(const std::shared_ptr<nox::event::Event>& event)
{
    if (this->info.receiveLogicEvent)
    {
        auto begin = this->cast(this->active);
        auto end = this->cast(this->memory);

        this->info.receiveLogicEvent(begin, end, event);
    }
}

void
nox::ecs::ComponentCollection::receiveEntityEvent(const ecs::Event& event)
{
    if (!this->info.receiveEntityEvent)
    {
        return;
    }

    if (event.getReceiver() == ecs::Event::BROADCAST)
    {
        auto begin = this->cast(this->active);
        auto end = this->cast(this->memory);
        this->info.receiveEntityEvent(begin, end, event);
    }
    else
    {
        auto component = this->find(event.getReceiver());
        if (component != std::end(componentMap))
        {
            // Ugly I know. However I must increment the bytes the correct number.
            // And I can't do that without casting it over to bytes.
            auto end = reinterpret_cast<Component*>(reinterpret_cast<Byte*>(component->component) + this->info.size);
            this->info.receiveEntityEvent(component->component, end, event);
        }
    }
}

std::size_t
nox::ecs::ComponentCollection::count() const
{
    return (this->memory - this->active) / this->info.size;
}

nox::ecs::ComponentHandle<nox::ecs::Component>
nox::ecs::ComponentCollection::getComponent(const EntityId& id)
{
    auto itr = this->find(id);
    auto component = (itr != std::end(componentMap)) ? itr->component : nullptr;
    ComponentHandle<Component> handle(id,
                                      component,
                                      this->gen,
                                      this);
    return handle;
}

std::size_t
nox::ecs::ComponentCollection::getGeneration() const
{
    return this->gen;
}

const nox::ecs::TypeIdentifier&
nox::ecs::ComponentCollection::getTypeIdentifier() const
{
    return this->info.typeIdentifier;
}

const nox::ecs::MetaInformation&
nox::ecs::ComponentCollection::getMetaInformation() const
{
    return this->info;
}

nox::ecs::Component*
nox::ecs::ComponentCollection::cast(Byte* entity) const
{
    return reinterpret_cast<Component*>(entity);
}

nox::ecs::ComponentCollection::IndexMap::iterator
nox::ecs::ComponentCollection::find(const EntityId& id)
{
    auto component = std::lower_bound(std::begin(this->componentMap),
                                      std::end(this->componentMap),
                                      id,
                                      [](const auto& element, const auto& value)
                                      { return element.id < value; });

    if (component != std::end(componentMap) &&
        component->id > id)
    {
        return std::end(componentMap);
    }
    return component;
}

nox::ecs::ComponentCollection::IndexMap::const_iterator
nox::ecs::ComponentCollection::find(const EntityId& id) const
{
    const auto component = std::lower_bound(std::cbegin(this->componentMap),
                                            std::cend(this->componentMap),
                                            id,
                                            [](const auto& element, const auto& value)
                                            { return element.id < value; });
    if (component != std::cend(componentMap) &&
        component->id > id)
    {
        return std::cend(componentMap);
    }
    return component;
}

nox::ecs::ComponentCollection::IndexMap::iterator
nox::ecs::ComponentCollection::findBefore(const EntityId& id)
{
    auto component = std::lower_bound(std::begin(this->componentMap),
                                      std::end(this->componentMap),
                                      id,
                                      [](const auto& element, const auto& value)
                                      { return element.id < value; });
    return component;
}

std::size_t
nox::ecs::ComponentCollection::size() const
{
    return std::size_t(this->memory - this->active);
}

std::size_t
nox::ecs::ComponentCollection::capacity() const
{
    return std::size_t(this->cap - this->active);
}

void
nox::ecs::ComponentCollection::reallocate()
{
    const auto newCap = this->capacity() * GROWTH_FACTOR;

    // + this->info.size for swapArea
    const auto newFirst = static_cast<Byte*>(std::malloc(newCap + this->info.size));

    auto newItr = newFirst;
    auto begin = this->active;
    const auto end = this->memory;

    while (begin != end)
    {
        auto newComp = this->cast(newItr);
        auto oldComp = this->cast(begin);

        this->info.moveConstruct(newComp, oldComp);

        newItr += this->info.size;
        begin += this->info.size;
    }

    this->destroyRange(active, memory);

    this->inactive = newFirst + std::distance(this->active, this->inactive);
    this->hibernating = newFirst + std::distance(this->active, this->hibernating);
    this->memory = newFirst + std::distance(this->active, this->memory);
    this->cap = newFirst + newCap;

    std::free(this->active);

    this->active = newFirst;
    this->gen++;

    this->updateWholeMap();
}

void
nox::ecs::ComponentCollection::destroyRange(Byte* begin,
                                            Byte* end)
{
    while (begin != end)
    {
        auto ptr = this->cast(begin);
        this->info.destruct(ptr);
        begin += this->info.size;
    }
}

void
nox::ecs::ComponentCollection::swap(Component* lhs,
                                    Component* rhs)
{
    if (lhs != rhs)
    {
        auto swapArea = this->cast(this->cap);
        this->info.moveConstruct(swapArea, rhs);
        this->info.moveAssign(rhs, lhs);
        this->info.moveAssign(lhs, swapArea);
        this->info.destruct(swapArea);
        this->gen++;
    }
}

void
nox::ecs::ComponentCollection::updateWholeMap()
{
    auto begin = this->active;
    auto end = this->memory;
    while (begin != end)
    {
        auto component = this->cast(begin);
        auto itr = this->find(component->id);
        itr->component = component;
        begin += this->info.size;
    }
}
