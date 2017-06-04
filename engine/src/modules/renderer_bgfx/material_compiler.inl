#ifndef CETECH_MATERIAL_COMPILER_H
#define CETECH_MATERIAL_COMPILER_H

#include <stdio.h>

#include <cetech/core/yaml.h>

#include <cetech/celib/array2.inl>
#include <cetech/core/path.h>

using namespace cetech;

namespace {
    struct material_compile_output {
        Array<char> uniform_names;
        Array<uint8_t> data;

        uint32_t texture_count;
        uint32_t vec4f_count;
        uint32_t mat33f_count;
        uint32_t mat44f_count;
    };

    void _preprocess(const char *filename,
                     yaml_node_t root,
                     struct compilator_api *capi) {
        yaml_node_t parent_node = yaml_get_node(root, "parent");

        if (yaml_is_valid(parent_node)) {
            char prefab_file[256] = {0};
            char prefab_str[256] = {0};
            yaml_as_string(parent_node, prefab_str,
                           CETECH_ARRAY_LEN(prefab_str));
            snprintf(prefab_file, CETECH_ARRAY_LEN(prefab_file), "%s.material",
                     prefab_str);

            capi->add_dependency(filename, prefab_file);

            char full_path[256] = {0};
            const char *source_dir = resource_api_v0.compiler_get_source_dir();
            path_v0.join(full_path, CETECH_ARRAY_LEN(full_path), source_dir,
                         prefab_file);

            struct vio *prefab_vio = vio_api_v0.from_file(full_path,
                                                          VIO_OPEN_READ,
                                                          memory_api_v0.main_allocator());

            char prefab_data[vio_api_v0.size(prefab_vio) + 1];
            memset(prefab_data, 0, vio_api_v0.size(prefab_vio) + 1);
            vio_api_v0.read(prefab_vio, prefab_data, sizeof(char),
                            vio_api_v0.size(prefab_vio));
            vio_api_v0.close(prefab_vio);

            yaml_document_t h;
            yaml_node_t prefab_root = yaml_load_str(prefab_data, &h);

            _preprocess(filename, prefab_root, capi);
            yaml_merge(root, prefab_root);
        }
    }

    void _forach_texture_clb(yaml_node_t key,
                             yaml_node_t value,
                             void *_data) {
        struct material_compile_output *output = (material_compile_output *) _data;

        output->texture_count += 1;

        char tmp_buffer[512] = {0};
        char uniform_name[32] = {0};

        yaml_as_string(key, uniform_name, CETECH_ARRAY_LEN(uniform_name) - 1);

        yaml_as_string(value, tmp_buffer, CETECH_ARRAY_LEN(tmp_buffer));
        uint64_t texture_name = hash_api_v0.id64_from_str(tmp_buffer);

        array::push(output->uniform_names, uniform_name,
                    CETECH_ARRAY_LEN(uniform_name));
        array::push(output->data, (uint8_t *) &texture_name, sizeof(uint64_t));
    }

    void _forach_vec4fs_clb(yaml_node_t key,
                            yaml_node_t value,
                            void *_data) {
        struct material_compile_output *output = (material_compile_output *) _data;

        output->vec4f_count += 1;

        char uniform_name[32] = {0};
        yaml_as_string(key, uniform_name, CETECH_ARRAY_LEN(uniform_name) - 1);

        vec4f_t v = yaml_as_vec4f_t(value);

        array::push(output->uniform_names, uniform_name,
                    CETECH_ARRAY_LEN(uniform_name));
        array::push(output->data, (uint8_t *) &v, sizeof(vec4f_t));
    }

    void _forach_mat44f_clb(yaml_node_t key,
                            yaml_node_t value,
                            void *_data) {
        struct material_compile_output *output = (material_compile_output *) _data;

        output->mat44f_count += 1;

        char uniform_name[32] = {0};
        yaml_as_string(key, uniform_name, CETECH_ARRAY_LEN(uniform_name) - 1);

        mat44f_t m = yaml_as_mat44f_t(value);

        array::push(output->uniform_names, uniform_name,
                    CETECH_ARRAY_LEN(uniform_name));
        array::push(output->data, (uint8_t *) &m, sizeof(mat44f_t));
    }

    void _forach_mat33f_clb(yaml_node_t key,
                            yaml_node_t value,
                            void *_data) {
        struct material_compile_output *output = (material_compile_output *) _data;

        output->mat33f_count += 1;

        char uniform_name[32] = {0};
        yaml_as_string(key, uniform_name, CETECH_ARRAY_LEN(uniform_name) - 1);

        mat33f_t m = yaml_as_mat33f_t(value);

        array::push(output->uniform_names, uniform_name,
                    CETECH_ARRAY_LEN(uniform_name));
        array::push(output->data, (uint8_t *) &m, sizeof(mat33f_t));
    }
}

namespace material_resource_compiler {
    int compiler(const char *filename,
                 struct vio *source_vio,
                 struct vio *build_vio,
                 struct compilator_api *compilator_api) {
        char *source_data =
                CETECH_ALLOCATE(memory_api_v0.main_allocator(), char,
                                vio_api_v0.size(source_vio) + 1);
        memset(source_data, 0, vio_api_v0.size(source_vio) + 1);

        vio_api_v0.read(source_vio, source_data, sizeof(char),
                        vio_api_v0.size(source_vio));

        yaml_document_t h;
        yaml_node_t root = yaml_load_str(source_data, &h);

        _preprocess(filename, root, compilator_api);

        yaml_node_t shader_node = yaml_get_node(root, "shader");
        CETECH_ASSERT("material", yaml_is_valid(shader_node));

        char tmp_buffer[256] = {0};
        yaml_as_string(shader_node, tmp_buffer, CETECH_ARRAY_LEN(tmp_buffer));

        struct material_compile_output output = {0};
        output.uniform_names.init(memory_api_v0.main_allocator());
        output.data.init(memory_api_v0.main_allocator());

        yaml_node_t textures = yaml_get_node(root, "textures");
        if (yaml_is_valid(textures)) {
            yaml_node_foreach_dict(textures, _forach_texture_clb, &output);
        }

        yaml_node_t vec4 = yaml_get_node(root, "vec4f");
        if (yaml_is_valid(vec4)) {
            yaml_node_foreach_dict(vec4, _forach_vec4fs_clb, &output);
        }

        yaml_node_t mat44 = yaml_get_node(root, "mat44f");
        if (yaml_is_valid(mat44)) {
            yaml_node_foreach_dict(mat44, _forach_mat44f_clb, &output);
        }

        yaml_node_t mat33 = yaml_get_node(root, "mat33f");
        if (yaml_is_valid(mat33)) {
            yaml_node_foreach_dict(mat33, _forach_mat33f_clb, &output);
        }


        struct material_blob resource = {
                .shader_name = hash_api_v0.id64_from_str(tmp_buffer),
                .texture_count =output.texture_count,
                .vec4f_count = output.vec4f_count,
                .uniforms_count = (uint32_t) (
                        array::size(output.uniform_names) / 32),
        };

        vio_api_v0.write(build_vio, &resource, sizeof(resource), 1);
        vio_api_v0.write(build_vio, output.uniform_names._data, sizeof(char),
                         array::size(output.uniform_names));
        vio_api_v0.write(build_vio, output.data._data, sizeof(uint8_t),
                         array::size(output.data));

        output.uniform_names.destroy();
        output.data.destroy();

        CETECH_DEALLOCATE(memory_api_v0.main_allocator(), source_data);
        return 1;
    }
}

#endif //CETECH_MATERIAL_COMPILER_H