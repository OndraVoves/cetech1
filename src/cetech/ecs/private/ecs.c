//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <sys/mman.h>

#include <celib/api_system.h>
#include <celib/memory.h>
#include <celib/bagraph.h>
#include <celib/cdb.h>
#include <celib/os.h>
#include <celib/log.h>

#include <celib/hashlib.h>
#include <celib/module.h>
#include <celib/ydb.h>
#include <celib/hash.inl>
#include <celib/handler.inl>
#include <celib/task.h>

#include <cetech/ecs/ecs.h>
#include <cetech/resource/resource.h>
#include <cetech/editor/resource_preview.h>
#include <cetech/kernel/kernel.h>
#include <cetech/renderer/renderer.h>
#include <celib/buffer.inl>
#include <cetech/debugui/icons_font_awesome.h>


//==============================================================================
// Globals
//==============================================================================

#define MAX_COMPONENTS 64
#define MAX_ENTITIES 100000

#define _G EntityMaagerGlobals

#define LOG_WHERE "ecs"

struct entity_storage {
    uint64_t mask;
    uint32_t n;
    struct ct_entity *entity;
    uint8_t *entity_data[MAX_COMPONENTS];
};

struct spawn_info {
    uint64_t obj;
    struct ct_entity *ents;
};

struct world_instance {
    struct ct_world world;
    struct ce_cdb_t db;

    // Entity
    struct ce_handler_t entity_handler;
    uint64_t *entity_type;
    uint64_t *entity_idx;
    uint64_t *entity_data;
    struct ct_entity *parent;
    struct ct_entity *first_child;
    struct ct_entity *next_sibling;

    // Storage
    struct ce_hash_t entity_storage_map;
    struct entity_storage *entity_storage;

    struct ce_hash_t entity_objmap;

    struct ce_hash_t obj_entmap;
    struct spawn_info *obj_spawn_info;

    struct ce_hash_t comp_entmap;
    struct spawn_info *comp_spawn_info;
};

#define _entity_data_idx(w, ent) \
    w->entity_idx[handler_idx((ent).h)]

#define _entity_type(w, ent) \
    w->entity_type[handler_idx((ent).h)]


static struct _G {
    struct ce_cdb_t db;

    // WORLD
    struct ce_hash_t world_map;
    struct ce_handler_t world_handler;
    struct world_instance *world_array;

    uint32_t component_count;
    struct ce_hash_t component_types;

    uint64_t *components_name;
    struct ce_hash_t component_interface_map;

    // SIM
    struct ce_ba_graph sg;
    struct ce_hash_t fce_map;

    struct ce_alloc *allocator;
} _G;

static void *virtual_alloc(uint64_t size) {
    return mmap(NULL,
                size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
}

//static void virtual_free(void* ptr, uint64_t size) {
//    munmap(ptr, size);
//}

static void _add_spawn_entity_obj(struct world_instance *world,
                                  uint64_t obj,
                                  struct ct_entity ent) {
    ce_hash_add(&world->entity_objmap, ent.h, obj, _G.allocator);

    uint64_t idx = ce_hash_lookup(&world->obj_entmap, obj, UINT64_MAX);

    if (idx == UINT64_MAX) {
        idx = ce_array_size(world->obj_spawn_info);
        ce_array_push(world->obj_spawn_info,
                      (struct spawn_info) {.obj = obj},
                      ce_memory_a0->system);
        ce_hash_add(&world->obj_entmap, obj, idx, _G.allocator);
    }

    struct spawn_info *info = &world->obj_spawn_info[idx];

    ce_array_push(info->ents, ent, ce_memory_a0->system);
}

static void _add_spawn_comp_obj(struct world_instance *world,
                                uint64_t obj,
                                struct ct_entity ent) {
    uint64_t idx = ce_hash_lookup(&world->comp_entmap, obj, UINT64_MAX);

    if (idx == UINT64_MAX) {
        idx = ce_array_size(world->comp_spawn_info);
        ce_array_push(world->comp_spawn_info,
                      (struct spawn_info) {.obj = obj},
                      ce_memory_a0->system);
        ce_hash_add(&world->comp_entmap, obj, idx, _G.allocator);
    }

    struct spawn_info *info = &world->comp_spawn_info[idx];

    ce_array_push(info->ents, ent, ce_memory_a0->system);
}

static void _remove_ent_from_spawn_info(struct spawn_info *spawn_info,
                                        struct ct_entity ent) {
    const uint32_t n = ce_array_size(spawn_info->ents);
    for (int i = 0; i < n; ++i) {
        if (spawn_info->ents[i].h != ent.h) {
            continue;
        }

        uint32_t last_idx = n - 1;
        spawn_info->ents[i] = spawn_info->ents[last_idx];
        ce_array_pop_back(spawn_info->ents);
    }
}

static void _remove_ent_from_spawn_infos(struct spawn_info *spawn_infos,
                                         struct ct_entity ent) {
    const uint32_t n = ce_array_size(spawn_infos);
    for (int i = 0; i < n; ++i) {
        _remove_ent_from_spawn_info(&spawn_infos[i], ent);
    }
}

static void _remove_spawn_info(struct spawn_info *spawn_infos,
                               struct ce_hash_t *map,
                               uint64_t obj) {
    if (!obj) {
        return;
    }

    uint64_t idx = ce_hash_lookup(map, obj, UINT64_MAX);

    if (idx == UINT64_MAX) {
        return;
    }

    ce_array_free(spawn_infos[idx].ents, _G.allocator);
    if (ce_array_size(spawn_infos)) {
        uint64_t last_idx = ce_array_size(spawn_infos) - 1;
        spawn_infos[idx] = spawn_infos[last_idx];
        ce_hash_add(map, spawn_infos[idx].obj, idx, _G.allocator);
    }

    ce_array_pop_back(spawn_infos);
    ce_hash_remove(map, obj);
}


static struct ct_component_i0 *get_interface(uint64_t name) {
    return (struct ct_component_i0 *) ce_hash_lookup(
            &_G.component_interface_map, name, 0);
}


static struct world_instance *get_world_instance(struct ct_world world) {
    uint64_t idx = ce_hash_lookup(&_G.world_map, world.h, UINT64_MAX);

    if (UINT64_MAX == idx) {
        return NULL;
    }

    return &_G.world_array[idx];
}


const uint64_t *component_names() {
    return _G.components_name;
}

static uint64_t component_mask(uint64_t name) {
    return (uint64_t) (1llu << ce_hash_lookup(&_G.component_types, name, 0));
}

static uint64_t component_idx(uint64_t component_name) {
    return ce_hash_lookup(&_G.component_types, component_name, UINT64_MAX);
}


static void *get_all(uint64_t component_name,
                     ct_entity_storage_t *_item) {
    struct entity_storage *item = _item;
    uint64_t com_mask = component_mask(component_name);

    if (!(com_mask & item->mask)) {
        return NULL;
    }

    struct ct_component_i0 *component_i = get_interface(component_name);

    uint32_t comp_idx = component_idx(component_name);
    return item->entity_data[comp_idx] + component_i->size();
}

static void *get_one(struct ct_world world,
                     uint64_t component_name,
                     struct ct_entity entity) {
    if (!entity.h) {
        return 0;
    }

    struct world_instance *w = get_world_instance(world);

    uint64_t ent_type = _entity_type(w, entity);

    uint64_t type_idx = ce_hash_lookup(&w->entity_storage_map,
                                       ent_type, UINT64_MAX);

    if (UINT64_MAX == type_idx) {
        return NULL;
    }

    struct ct_component_i0 *c = get_interface(component_name);

    if (!c) {
        return NULL;
    }

    uint64_t com_mask = component_mask(component_name);

    if (!(com_mask & ent_type)) {
        return NULL;
    }

    uint64_t com_idx = component_idx(component_name);
    struct entity_storage *item = &w->entity_storage[type_idx];
    uint8_t *comp_data = item->entity_data[com_idx];

    uint64_t entity_data_idx = _entity_data_idx(w, entity);
    return comp_data + (c->size() * entity_data_idx);
}

static void _add_to_type_slot(struct world_instance *w,
                              struct ct_entity ent,
                              uint64_t ent_type) {
    uint64_t type_idx = ce_hash_lookup(&w->entity_storage_map, ent_type,
                                       UINT64_MAX);

    if (UINT64_MAX == type_idx) {
        struct entity_storage storage = {
                .mask=ent_type,
                .n = 1,
        };
        ce_array_push(w->entity_storage, storage, _G.allocator);

        type_idx = ce_array_size(w->entity_storage) - 1;
        ce_hash_add(&w->entity_storage_map, ent_type, type_idx, _G.allocator);

        struct entity_storage *item = &w->entity_storage[type_idx];
        ce_array_push(item->entity, (struct ct_entity) {}, _G.allocator);

        const uint32_t component_n = ce_array_size(_G.components_name);
        for (int i = 0; i < component_n; ++i) {
            uint64_t component_name = _G.components_name[i];

            const uint32_t comp_idx = component_idx(component_name);

            uint64_t mask = (1llu << comp_idx);
            if (!(ent_type & mask)) {
                continue;
            }

            item->entity_data[comp_idx] = virtual_alloc(
                    MAX_ENTITIES * sizeof(uint64_t));
        }

    }

    struct entity_storage *item = &w->entity_storage[type_idx];

    const uint64_t ent_data_idx = item->n++;

    _entity_data_idx(w, ent) = ent_data_idx;
    _entity_type(w, ent) = ent_type;

    ce_array_push(item->entity, ent, _G.allocator);

    const uint32_t component_n = ce_array_size(_G.components_name);
    for (int i = 0; i < component_n; ++i) {
        uint64_t component_name = _G.components_name[i];

        const uint32_t comp_idx = component_idx(component_name);

        uint64_t mask = (1llu << comp_idx);
        if (!(ent_type & mask)) {
            continue;
        }

        struct ct_component_i0 *component_i = get_interface(component_name);
        uint64_t size = component_i->size();

        memset(item->entity_data[comp_idx] + (size * ent_data_idx), 0, size);
    }
}

static void _remove_from_type_slot(struct world_instance *w,
                                   struct ct_entity ent,
                                   uint64_t last_data_idx,
                                   uint64_t ent_type) {
    uint64_t type_idx = ce_hash_lookup(&w->entity_storage_map,
                                       ent_type, UINT64_MAX);

    if (UINT64_MAX == type_idx) {
        return;
    }

    struct entity_storage *item = &w->entity_storage[type_idx];

    if (!item->n) {
        return;
    }

    uint32_t last_idx = --item->n;

    uint64_t entity_data_idx = last_data_idx;

    if (last_idx == entity_data_idx) {
        return;
    }

    struct ct_entity last_ent = item->entity[last_idx];
    _entity_data_idx(w, last_ent) = entity_data_idx;

    item->entity[entity_data_idx] = last_ent;
    ce_array_pop_back(item->entity);

    const uint32_t component_n = ce_array_size(_G.components_name);
    for (int i = 0; i < component_n; ++i) {
        uint64_t component_name = _G.components_name[i];

        const uint32_t comp_idx = component_idx(component_name);

        if (!item->entity_data[comp_idx]) {
            continue;
        }

        struct ct_component_i0 *component_i = get_interface(component_name);
        uint64_t size = component_i->size();
        memcpy(item->entity_data[comp_idx] + (entity_data_idx * size),
               item->entity_data[comp_idx] + (last_idx * size), size);
    }
}

static void _move_from_type_slot(struct world_instance *w,
                                 struct ct_entity ent,
                                 uint32_t old_idx,
                                 uint64_t ent_type,
                                 uint64_t new_type) {

    uint64_t type_idx = ce_hash_lookup(&w->entity_storage_map, ent_type,
                                       UINT64_MAX);

    uint64_t new_type_idx = ce_hash_lookup(&w->entity_storage_map, new_type,
                                           UINT64_MAX);

    struct entity_storage *item = &w->entity_storage[type_idx];
    struct entity_storage *new_item = &w->entity_storage[new_type_idx];

    uint32_t idx = _entity_data_idx(w, ent);

    const uint32_t component_n = ce_array_size(_G.components_name);
    for (int i = 0; i < component_n; ++i) {
        uint64_t component_name = _G.components_name[i];

        const uint32_t comp_idx = component_idx(component_name);

        uint64_t mask = (1llu << comp_idx);
        if (!(ent_type & mask)) {
            continue;
        }

        if (!(new_type & mask)) {
            continue;
        }

        struct ct_component_i0 *component_i = get_interface(component_name);
        uint64_t size = component_i->size();

        memcpy(new_item->entity_data[comp_idx] + (idx * size),
               item->entity_data[comp_idx] + (old_idx * size), size);
    }

}

static bool has(struct ct_world world,
                struct ct_entity ent,
                uint64_t *component_name,
                uint32_t name_count) {
    struct world_instance *w = get_world_instance(world);

    uint64_t ent_type = _entity_type(w, ent);

    uint64_t mask = 0;
    for (int i = 0; i < name_count; ++i) {
        mask |= (1 << ce_hash_lookup(&_G.component_types,
                                     component_name[i], 0));
    }

    return ((ent_type & mask) == mask);
}

static uint64_t combine_component(const uint64_t *component_name,
                                  uint32_t name_count) {
    uint64_t new_type = 0;
    for (int i = 0; i < name_count; ++i) {
        new_type |= (1 << ce_hash_lookup(&_G.component_types,
                                         component_name[i], 0));
    }

    return new_type;
}

static void _add_components(struct ct_world world,
                            struct ct_entity ent,
                            uint64_t new_type) {
    struct world_instance *w = get_world_instance(world);

    uint64_t ent_type = _entity_type(w, ent);
    uint32_t idx = _entity_data_idx(w, ent);

    new_type = ent_type | new_type;

    _add_to_type_slot(w, ent, new_type);

    if (ent_type) {
        _move_from_type_slot(w, ent, idx, ent_type, new_type);
        _remove_from_type_slot(w, ent, idx, ent_type);
    }
}

static void add_components(struct ct_world world,
                           struct ct_entity ent,
                           const uint64_t *component_name,
                           uint32_t name_count,
                           void **data) {

    uint64_t new_type = combine_component(component_name, name_count);
    _add_components(world, ent, new_type);

    if (data) {
        for (int i = 0; i < name_count; ++i) {
            uint64_t name = component_name[i];


            struct ct_component_i0 *c = get_interface(name);

            void *new_comp_data = data[i];
            void *comp_data = get_one(world, name, ent);
            memcpy(comp_data, new_comp_data, c->size());

            if (c && c->spawner) {
                c->spawner(world, 0, data[i]);
            }

        }
    }
}

static void _add_components_from_obj(struct world_instance *world,
                                     struct ct_entity ent,
                                     const uint64_t component_name,
                                     uint64_t obj) {

    uint64_t new_type = combine_component(&component_name, 1);
    _add_components(world->world, ent, new_type);

    struct ct_component_i0 *c = get_interface(component_name);

    void *comp_data = get_one(world->world, component_name, ent);

    if (c && c->spawner) {
        c->spawner(world->world, obj, comp_data);
    }


    _add_spawn_comp_obj(world, obj, ent);
}

static void remove_components(struct ct_world world,
                              struct ct_entity ent,
                              const uint64_t *component_name,
                              uint32_t name_count) {
    struct world_instance *w = get_world_instance(world);

    uint64_t ent_type = _entity_type(w, ent);
    uint64_t new_type = ent_type;
    uint64_t comp_type = combine_component(component_name, name_count);
    new_type &= ~(comp_type);

    uint32_t idx = _entity_data_idx(w, ent);

    if (new_type) {
        _add_to_type_slot(w, ent, new_type);
        _move_from_type_slot(w, ent, idx, ent_type, new_type);
    }

    _remove_from_type_slot(w, ent, idx, ent_type);

    if (!new_type) {
        _entity_type(w, ent) = 0;
    }

}

static void process(struct ct_world world,
                    uint64_t components_mask,
                    ct_process_fce_t fce,
                    void *data) {
    struct world_instance *w = get_world_instance(world);

    const uint32_t type_count = ce_array_size(w->entity_storage);

    for (int i = 0; i < type_count; ++i) {
        struct entity_storage *item = &w->entity_storage[i];

        if (!item->n) {
            continue;
        }

        if ((item->mask & components_mask) != components_mask) {
            continue;
        }

        fce(world, item->entity + 1, item, item->n - 1, data);
    }
}

static void _simulate_world(struct ce_ba_graph *sg,
                            struct ct_world world,
                            float dt) {

    const uint64_t output_n = ce_array_size(sg->output);
    for (int k = 0; k < output_n; ++k) {
        ct_simulate_fce_t fce;
        fce = (ct_simulate_fce_t) ce_hash_lookup(&_G.fce_map,
                                                 sg->output[k], 0);
        fce(world, dt);
    }

}

static void _build_sim_graph(struct ce_ba_graph *sg) {
    ce_bag_clean(sg);
    ce_hash_clean(&_G.fce_map);

    struct ce_api_entry it = ce_api_a0->first(SIMULATION_INTERFACE);
    while (it.api) {
        struct ct_simulation_i0 *i = (it.api);

        uint64_t name = i->name();

        ce_hash_add(&_G.fce_map, name,
                    (uint64_t) i->simulation, _G.allocator);


        uint32_t before_n = 0;
        const uint64_t *before = NULL;
        if (i->before) {
            before = i->before(&before_n);
        }

        uint32_t after_n = 0;
        const uint64_t *after;
        if (i->after) {
            after = i->after(&after_n);
        }

        ce_bag_add(&_G.sg, name,
                   before, before_n,
                   after, after_n, _G.allocator);

        it = ce_api_a0->next(it);
    }

    ce_bag_build(&_G.sg, _G.allocator);
}

static void simulate(struct ct_world world,
                     float dt) {
    _build_sim_graph(&_G.sg);
    _simulate_world(&_G.sg, world, dt);
}

static void create_entities(struct ct_world world,
                            struct ct_entity *entity,
                            uint32_t count) {

    struct world_instance *w = get_world_instance(world);

    for (int i = 0; i < count; ++i) {
        struct ct_entity ent = {.h = ce_handler_create(&w->entity_handler,
                                                       _G.allocator)};
        entity[i] = ent;

        uint64_t idx = handler_idx(ent.h);

        w->parent[idx].h = 0;
        w->next_sibling[idx].h = 0;
        w->first_child[idx].h = 0;

        w->entity_data[idx] = 0;
    }
}

static void create_entities_objs(struct ct_world world,
                                 struct ct_entity *entity,
                                 uint32_t count,
                                 uint64_t *objs) {

    struct world_instance *w = get_world_instance(world);

    for (int i = 0; i < count; ++i) {
        struct ct_entity ent = {.h = ce_handler_create(&w->entity_handler,
                                                       _G.allocator)};
        entity[i] = ent;

        uint64_t idx = handler_idx(ent.h);

        w->parent[idx].h = 0;
        w->next_sibling[idx].h = 0;
        w->first_child[idx].h = 0;

        uint64_t obj = 0;
        if (objs[i]) {
            obj = objs[i];
        }

        w->entity_data[idx] = obj;
    }
}

static void destroy(struct ct_world world,
                    struct ct_entity *entity,
                    uint32_t count) {
    struct world_instance *w = get_world_instance(world);

    for (uint32_t i = 0; i < count; ++i) {
        struct ct_entity ent = entity[i];

        uint64_t ent_type = _entity_type(w, ent);
        uint64_t ent_last_idx = _entity_data_idx(w, ent);
        _remove_from_type_slot(w, ent, ent_last_idx, ent_type);

        _entity_type(w, ent) = 0;

        uint64_t ent_idx = handler_idx(ent.h);
        struct ct_entity ent_it = w->first_child[ent_idx];

        while (ent_it.h != 0) {
            destroy(world, &ent_it, 1);

            uint64_t idx = handler_idx(ent_it.h);
            ent_it = w->next_sibling[idx];
        }

        _remove_ent_from_spawn_infos(w->comp_spawn_info, ent);
        _remove_ent_from_spawn_infos(w->obj_spawn_info, ent);

        ce_handler_destroy(&w->entity_handler, ent.h, _G.allocator);
    }
}

static void online(uint64_t name,
                   uint64_t obj) {
    CE_UNUSED(name);
}

static void offline(uint64_t name,
                    uint64_t obj) {
    CE_UNUSED(name, obj);
}

static uint64_t cdb_type() {
    return ENTITY_RESOURCE_ID;
}

static struct ct_entity spawn_entity(struct ct_world world,
                                     uint64_t name);

static struct ct_entity load(uint64_t resource,
                             struct ct_world world) {
    struct ct_entity ent = spawn_entity(world, resource);
    return ent;
}


static void unload(uint64_t resource,
                   struct ct_world world,
                   struct ct_entity entity) {
    ct_ecs_a0->destroy(world, &entity, 1);
}

void *get_resource_interface(uint64_t name_hash) {
    static struct ct_resource_preview_i0 ct_resource_preview_i0 = {
            .load = load,
            .unload = unload,
    };

    if (name_hash == RESOURCE_PREVIEW_I) {
        return &ct_resource_preview_i0;
    };

    return NULL;
}

void create_new(uint64_t obj) {
    ce_cdb_obj_o *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);

    if (ce_cdb_a0->prop_exist(w, ENTITY_CHILDREN)) {
        uint64_t ch = ce_cdb_a0->create_object(ce_cdb_a0->db(),
                                               ENTITY_CHILDREN);
        ce_cdb_a0->set_subobject(w, ENTITY_CHILDREN, ch);
    }

    if (ce_cdb_a0->prop_exist(w, ENTITY_COMPONENTS)) {
        uint64_t ch = ce_cdb_a0->create_object(ce_cdb_a0->db(),
                                               ENTITY_COMPONENTS);
        ce_cdb_a0->set_subobject(w, ENTITY_COMPONENTS, ch);
    }

    ce_cdb_a0->write_commit(w);
}

static const char* display_icon() {
    return ICON_FA_CUBES;
}

static struct ct_resource_i0 ct_resource_i0 = {
        .cdb_type = cdb_type,
        .display_icon = display_icon,
        .online = online,
        .offline = offline,
        .get_interface = get_resource_interface,
        .create_new = create_new,
};

//==============================================================================
// Public interface
//==============================================================================

static bool alive(struct ct_world world,
                  struct ct_entity entity) {
    struct world_instance *w = get_world_instance(world);
    return ce_handler_alive(&w->entity_handler, entity.h);
}

static void link(struct ct_world world,
                 struct ct_entity parent,
                 struct ct_entity child) {
    struct world_instance *w = get_world_instance(world);

    uint64_t child_idx = handler_idx(child.h);
    uint64_t parent_idx = handler_idx(parent.h);

    w->parent[child_idx] = parent;

    struct ct_entity tmp = w->first_child[parent_idx];

    w->first_child[parent_idx] = child;
    w->next_sibling[child_idx] = tmp;
}

static struct ct_entity spawn_entity(struct ct_world world,
                                     uint64_t name) {

//    char *buf = NULL;
//    ce_cdb_a0->dump_str(ce_cdb_a0->db(), &buf, name, 0);
//    ce_log_a0->debug(LOG_WHERE, "\n%s", buf);
//    ce_buffer_free(buf, _G.allocator);

    struct world_instance *w = get_world_instance(world);

    struct ct_entity root_ent;
    create_entities_objs(world, &root_ent, 1, &name);


    const ce_cdb_obj_o *ent_reader = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                     name);

    uint64_t components;
    components = ce_cdb_a0->read_subobject(ent_reader, ENTITY_COMPONENTS, 0);

    const ce_cdb_obj_o *comp_reader = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                      components);

    uint32_t components_n = ce_cdb_a0->prop_count(comp_reader);
    const uint64_t *components_keys = ce_cdb_a0->prop_keys(comp_reader);

    uint64_t ent_type = combine_component(components_keys, components_n);

    _add_components(world, root_ent, ent_type);

    _add_spawn_entity_obj(w, name, root_ent);

    uint64_t type_idx = ce_hash_lookup(&w->entity_storage_map,
                                       ent_type, UINT64_MAX);

    struct entity_storage *item = &w->entity_storage[type_idx];

    const uint64_t idx = _entity_data_idx(w, root_ent);

    for (int i = 0; i < components_n; ++i) {
        uint64_t component_type = components_keys[i];
        uint64_t j = component_idx(component_type);

        struct ct_component_i0 *component_i;
        component_i = get_interface(component_type);

        if (!component_i) {
            continue;
        }

        uint64_t component_obj;
        component_obj = ce_cdb_a0->read_subobject(comp_reader,
                                                  component_type, 0);

        _add_spawn_comp_obj(w, component_obj, root_ent);

        uint8_t *comp_data = item->entity_data[j];

        if (component_i->spawner) {
            component_i->spawner(world, component_obj,
                                 comp_data + (component_i->size() * idx));
        }
    }

    uint64_t children;
    children = ce_cdb_a0->read_subobject(ent_reader, ENTITY_CHILDREN, 0);

    const ce_cdb_obj_o *ch_reader = ce_cdb_a0->read(ce_cdb_a0->db(), children);

    uint32_t children_n = ce_cdb_a0->prop_count(ch_reader);
    const uint64_t *children_keys = ce_cdb_a0->prop_keys(ch_reader);
    for (int i = 0; i < children_n; ++i) {
        uint64_t child;
        child = ce_cdb_a0->read_subobject(ch_reader, children_keys[i], 0);

        struct ct_entity child_ent = spawn_entity(world, child);

        link(world, root_ent, child_ent);
    }

    return root_ent;
}

//==============================================================================
// Public interface
//==============================================================================
static struct world_instance *_new_world(struct ct_world world) {
    uint32_t idx = ce_array_size(_G.world_array);

    struct world_instance wi = {
            .entity_type = virtual_alloc(sizeof(uint64_t) * MAX_ENTITIES),
            .entity_idx  = virtual_alloc(sizeof(uint64_t) * MAX_ENTITIES),
            .entity_data = virtual_alloc(sizeof(uint64_t) * MAX_ENTITIES),

            .parent = virtual_alloc(sizeof(struct ct_entity) * MAX_ENTITIES),
            .first_child = virtual_alloc(
                    sizeof(struct ct_entity) * MAX_ENTITIES),
            .next_sibling = virtual_alloc(
                    sizeof(struct ct_entity) * MAX_ENTITIES),
    };

    ce_array_push(_G.world_array, wi, _G.allocator);

    ce_handler_create(&_G.world_array[idx].entity_handler, _G.allocator);

    ce_hash_add(&_G.world_map, world.h, idx, _G.allocator);
    return &_G.world_array[idx];
}

static struct ct_world create_world() {
    struct ct_world world = {.h = ce_handler_create(&_G.world_handler,
                                                    _G.allocator)};

    struct world_instance *w = _new_world(world);

    w->world = world;
    w->db = ce_cdb_a0->db();

    return world;
}

static void destroy_world(struct ct_world world) {
    ce_handler_destroy(&_G.world_handler, world.h, _G.allocator);
}

static struct ct_ecs_a0 _api = {
        //ENT
        .create = create_entities,
        .destroy = destroy,
        .alive = alive,
        .spawn = spawn_entity,
        .has = has,

        .create_world = create_world,
        .destroy_world = destroy_world,

        //SIMU
        .simulate = simulate,
        .process = process,

        //COMP
        .get_interface = get_interface,
        .mask = component_mask,
        .get_all = get_all,
        .get_one = get_one,
        .add = add_components,
        .remove = remove_components,
};

struct ct_ecs_a0 *ct_ecs_a0 = &_api;


static void _init_api(struct ce_api_a0 *api) {
    api->register_api(CT_ECS_API, &_api);
}

static void _componet_api_add(uint64_t name,
                              void *api) {
    struct ct_component_i0 *component_i = api;

    ce_array_push(_G.components_name, component_i->cdb_type(), _G.allocator);

    ce_hash_add(&_G.component_interface_map, component_i->cdb_type(),
                (uint64_t) component_i, _G.allocator);

    const uint64_t cid = _G.component_count++;
    ce_hash_add(&_G.component_types, component_i->cdb_type(), cid,
                _G.allocator);
}


static uint64_t task_name() {
    return CT_ECS_SYNC_TASK;
}

static uint64_t *update_after(uint64_t *n) {
    static uint64_t a[] = {
            CT_RENDER_TASK,
    };

    *n = CE_ARRAY_LEN(a);
    return a;
}

static void _sync_ent_obj(struct world_instance *world) {
    uint64_t *empty_si = NULL;

    // OBJS
    uint32_t objs_n = ce_array_size(world->obj_spawn_info);
    for (uint32_t j = 0; j < objs_n; ++j) {
        struct spawn_info *si = &world->obj_spawn_info[j];

        const ce_cdb_obj_o *ent_r = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                    si->obj);

        // Children
        uint64_t children = ce_cdb_a0->read_subobject(ent_r,
                                                      ENTITY_CHILDREN, 0);

        if (children) {
            const ce_cdb_obj_o *ents_r = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                         children);
            uint32_t change_n = 0;
            const struct ce_cdb_change_ev0 *changes;
            changes = ce_cdb_a0->changed(ents_r, &change_n);

            for (int ch = 0; ch < change_n; ++ch) {
                struct ce_cdb_change_ev0 ev = changes[ch];
                if (ev.type == CE_CDB_REMOVE) {
                    uint64_t ent_obj = ev.old_value.subobj;

                    uint64_t idx = ce_hash_lookup(&world->obj_entmap, ent_obj,
                                                  UINT64_MAX);
                    if (idx == UINT64_MAX) {
                        continue;
                    }

                    struct spawn_info *ch_si = &world->obj_spawn_info[idx];

                    struct ct_entity *ents = ch_si->ents;
                    uint32_t ents_n = ce_array_size(ents);
                    if (ents_n) {
                        destroy(world->world, ents, ents_n);
                    }

                    ce_array_push(empty_si, ent_obj, _G.allocator);

                } else if (ev.type == CE_CDB_CHANGE) {
                    uint64_t ent_obj = ev.new_value.subobj;
                    struct ct_entity *ents = si->ents;
                    uint32_t ents_n = ce_array_size(ents);

                    for (int e = 0; e < ents_n; ++e) {
                        struct ct_entity new_ents;
                        new_ents = spawn_entity(world->world, ent_obj);

                        link(world->world, ents[e], new_ents);
                    }

                }
            }
        }

        // Components
        uint64_t comps = ce_cdb_a0->read_subobject(ent_r,
                                                   ENTITY_COMPONENTS, 0);

        if (comps) {
            const ce_cdb_obj_o *comps_r = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                          comps);
            uint32_t change_n = 0;
            const struct ce_cdb_change_ev0 *changes;
            changes = ce_cdb_a0->changed(comps_r, &change_n);

            for (int ch = 0; ch < change_n; ++ch) {
                struct ce_cdb_change_ev0 ev = changes[ch];
                if (ev.type == CE_CDB_REMOVE) {
                    uint64_t comp_obj = ev.old_value.subobj;
                    const ce_cdb_obj_o *r = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                            comp_obj);
                    uint64_t k = ce_cdb_a0->obj_type(r);

                    struct ct_entity *ents = si->ents;
                    uint32_t ents_n = ce_array_size(ents);
                    for (int e = 0; e < ents_n; ++e) {
                        struct ct_entity ent = ents[e];
                        remove_components(world->world, ent, &k, 1);
                        _remove_spawn_info(world->comp_spawn_info,
                                           &world->comp_entmap, comp_obj);
                    }

                } else if (ev.type == CE_CDB_CHANGE) {
                    uint64_t comp_obj = ev.new_value.subobj;
                    const ce_cdb_obj_o *r = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                            comp_obj);
                    uint64_t k = ce_cdb_a0->obj_type(r);

                    struct ct_entity *ents = si->ents;
                    uint32_t ents_n = ce_array_size(ents);
                    for (int e = 0; e < ents_n; ++e) {
                        struct ct_entity ent = ents[e];
                        _add_components_from_obj(world, ent, k, comp_obj);
                    }
                }
            }
        }
    }

    uint32_t empty_si_n = ce_array_size(empty_si);
    for (int i = 0; i < empty_si_n; ++i) {
        _remove_spawn_info(world->obj_spawn_info,
                           &world->obj_entmap,
                           empty_si[i]);
    }
    ce_array_free(empty_si, _G.allocator);
}

static void _sync_comp_obj(struct world_instance *world) {
    uint64_t *empty_si = NULL;

    uint32_t objs_n = ce_array_size(world->comp_spawn_info);
    for (uint32_t j = 0; j < objs_n; ++j) {
        struct spawn_info *si = &world->comp_spawn_info[j];

        if (!ce_array_size(si->ents)) {
            ce_array_push(empty_si, si->obj, _G.allocator);
            continue;
        }

        const ce_cdb_obj_o *comp_r = ce_cdb_a0->read(ce_cdb_a0->db(),
                                                     si->obj);

        if (!comp_r) {
            continue;
        }

        uint32_t change_n = 0;
        const struct ce_cdb_change_ev0 *changes;
        changes = ce_cdb_a0->changed(comp_r, &change_n);

        uint64_t component_type = ce_cdb_a0->obj_type(comp_r);

        if (change_n) {
            struct ct_component_i0 *ci = get_interface(component_type);

            struct ct_entity *ents = si->ents;
            uint32_t ents_n = ce_array_size(ents);
            for (int e = 0; e < ents_n; ++e) {
                struct ct_entity ent = ents[e];
                void *data = get_one(world->world, component_type, ent);

                if (!data) {
                    continue;
                }

                if (ci->changer) {
                    ci->changer(world->world, changes, change_n, data);
                } else if (ci->spawner) {
                    ci->spawner(world->world, si->obj, data);
                }
            }
        }
    }

    uint32_t empty_si_n = ce_array_size(empty_si);
    for (int i = 0; i < empty_si_n; ++i) {
        _remove_spawn_info(world->comp_spawn_info,
                           &world->comp_entmap,
                           empty_si[i]);
    }
    ce_array_free(empty_si, _G.allocator);
}

static void _update(float dt) {
    uint32_t n = ce_array_size(_G.world_array);
    for (uint32_t i = 0; i < n; ++i) {
        struct world_instance *world = &_G.world_array[i];
        _sync_ent_obj(world);
        _sync_comp_obj(world);
    }
}

static struct ct_kernel_task_i0 ecs_sync_task = {
        .name = task_name,
        .update = _update,
        .update_after = update_after,
};


static void _init(struct ce_api_a0 *api) {
    _init_api(api);

    _G = (struct _G) {
            .allocator = ce_memory_a0->system,
            .db = ce_cdb_a0->db()
    };

    ce_handler_create(&_G.world_handler, _G.allocator);

    ce_api_a0->register_api(RESOURCE_I, &ct_resource_i0);
    ce_api_a0->register_api(KERNEL_TASK_INTERFACE, &ecs_sync_task);
    ce_api_a0->register_on_add(COMPONENT_I, _componet_api_add);
}

static void _shutdown() {
}


CE_MODULE_DEF(
        ecs,
        {
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ct_resource_a0);
            CE_INIT_API(api, ce_os_a0);
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ce_cdb_a0);
            CE_INIT_API(api, ce_task_a0);
        },

        {
            CE_UNUSED(reload);
            _init(api);
        },
        {
            CE_UNUSED(reload);
            CE_UNUSED(api);
            _shutdown();
        }
)