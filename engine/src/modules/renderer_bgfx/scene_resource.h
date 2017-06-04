#ifndef CETECH_SCENE_RESOURCE_H
#define CETECH_SCENE_RESOURCE_H


#include <cetech/core/path.h>

static const bgfx_texture_handle_t null_texture = {0};


void *_scene_resource_loader(struct vio *input,
                             struct allocator *allocator) {
    const int64_t size = vio_api_v0.size(input);
    char *data = CETECH_ALLOCATE(allocator, char, size);
    vio_api_v0.read(input, data, 1, size);

    return data;
}

void _scene_resource_unloader(void *new_data,
                              struct allocator *allocator) {
    CETECH_DEALLOCATE(allocator, new_data);
}

void _scene_resource_online(uint64_t name,
                            void *data) {
    struct scene_blob *resource = (scene_blob *) data;

    uint64_t *geom_name = scene_blob_geom_name(resource);
    uint32_t *ib_offset = scene_blob_ib_offset(resource);
    uint32_t *vb_offset = scene_blob_vb_offset(resource);
    bgfx_vertex_decl_t *vb_decl = scene_blob_vb_decl(resource);
    uint32_t *ib_size = scene_blob_ib_size(resource);
    uint32_t *vb_size = scene_blob_vb_size(resource);
    uint32_t *ib = scene_blob_ib(resource);
    uint8_t *vb = scene_blob_vb(resource);

    struct scene_instance instance = {0};
    _init_scene_instance(&instance);

    for (int i = 0; i < resource->geom_count; ++i) {
        bgfx_vertex_buffer_handle_t bvb = bgfx_create_vertex_buffer(
                bgfx_make_ref((const void *) &vb[vb_offset[i]], vb_size[i]),
                &vb_decl[i], BGFX_BUFFER_NONE);

        bgfx_index_buffer_handle_t bib = bgfx_create_index_buffer(
                bgfx_make_ref((const void *) &ib[ib_offset[i]],
                              sizeof(uint32_t) * ib_size[i]),
                BGFX_BUFFER_INDEX32);

        uint32_t idx = ARRAY_SIZE(&instance.vb);
        MAP_SET(uint8_t, &instance.geom_map, geom_name[i], idx);

        ARRAY_PUSH_BACK(uint32_t, &instance.size, ib_size[i]);
        ARRAY_PUSH_BACK(bgfx_vertex_buffer_handle_t, &instance.vb, bvb);
        ARRAY_PUSH_BACK(bgfx_index_buffer_handle_t, &instance.ib, bib);
    }

    MAP_SET(scene_instance, &_G.scene_instance, name, instance);
}

void _scene_resource_offline(uint64_t name,
                             void *data) {
    struct scene_instance instance = MAP_GET(scene_instance, &_G.scene_instance,
                                             name,
                                             (struct scene_instance) {0});
    _destroy_scene_instance(&instance);
    MAP_REMOVE(scene_instance, &_G.scene_instance, name);
}

void *_scene_resource_reloader(uint64_t name,
                               void *old_data,
                               void *new_data,
                               struct allocator *allocator) {
    _scene_resource_offline(name, old_data);
    _scene_resource_online(name, new_data);

    CETECH_DEALLOCATE(allocator, old_data);

    return new_data;
}

static const resource_callbacks_t scene_resource_callback = {
        .loader = _scene_resource_loader,
        .unloader =_scene_resource_unloader,
        .online =_scene_resource_online,
        .offline =_scene_resource_offline,
        .reloader = _scene_resource_reloader
};


#endif //CETECH_SCENE_RESOURCE_H