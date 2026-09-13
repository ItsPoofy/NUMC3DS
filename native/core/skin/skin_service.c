#include "skin_service.h"
#include "../world_transfer/world_transfer_fs.h"
#include "../util/string_util.h"
#include "../../state.h"

static CustomSkinEntry s_custom_skins[NUMC3DS_MAX_CUSTOM_SKINS];
static int s_custom_skin_count = 0;
static int s_hovered_skin_index = -1;
static char s_models_ini_cache[2048];
static int s_models_ini_loaded = 0;

/* Helper: check if string ends with suffix */
static int str_ends_with(const char *str, const char *suffix)
{
    unsigned str_len, suf_len, i;
    if (!str || !suffix) return 0;
    str_len = text_len(str);
    suf_len = text_len(suffix);
    if (str_len < suf_len) return 0;
    for (i = 0; i < suf_len; i++) {
        if (str[str_len - suf_len + i] != suffix[i]) return 0;
    }
    return 1;
}

/* Helper: find substring needle in haystack */
static const char *skin_strstr(const char *haystack, const char *needle)
{
    unsigned i, j;
    if (!haystack || !needle) return 0;
    if (!needle[0]) return haystack;
    for (i = 0; haystack[i]; i++) {
        for (j = 0; needle[j]; j++) {
            if (haystack[i + j] != needle[j]) break;
        }
        if (!needle[j]) return &haystack[i];
    }
    return 0;
}

/* Helper: construct path "<dir>/<file>" */
static int join_path(char *out, unsigned cap, const char *dir, const char *file)
{
    return wt_fs_join(out, cap, dir, file);
}

/* Helper: Load models.ini into memory */
static void load_models_ini(void)
{
    WtFsFile file;
    u32 read_bytes = 0;
    s_models_ini_cache[0] = '\0';
    s_models_ini_loaded = 1;

    if (wt_fs_open_file(NUMC3DS_MODELS_INI, 0, &file)) {
        wt_fs_read(&file, 0, s_models_ini_cache, sizeof(s_models_ini_cache) - 1, &read_bytes);
        s_models_ini_cache[read_bytes] = '\0';
        wt_fs_close_file(&file);
    }
}

/* Helper: Query model type for a given skin name from models.ini */
static int query_model_ini(const char *skin_name)
{
    char search_key[64];
    unsigned key_len = 0;
    if (!s_models_ini_loaded) load_models_ini();
    if (!skin_name || !skin_name[0]) return SKIN_MODEL_REGULAR;

    search_key[0] = '\0';
    append(search_key, &key_len, skin_name);
    append(search_key, &key_len, "=slim");

    if (skin_strstr(s_models_ini_cache, search_key)) {
        return SKIN_MODEL_SLIM;
    }

    return SKIN_MODEL_REGULAR;
}

/* Helper: Save all model preferences back to models.ini */
static void save_models_ini(void)
{
    WtFsFile file;
    char buffer[2048];
    unsigned offset = 0;
    int i;

    buffer[0] = '\0';
    for (i = 0; i < s_custom_skin_count; i++) {
        if (s_custom_skins[i].model_type == SKIN_MODEL_SLIM) {
            append(buffer, &offset, s_custom_skins[i].name);
            append(buffer, &offset, "=slim\n");
        } else if (s_custom_skins[i].model_type == SKIN_MODEL_REGULAR) {
            append(buffer, &offset, s_custom_skins[i].name);
            append(buffer, &offset, "=normal\n");
        }
    }

    wt_fs_delete_file(NUMC3DS_MODELS_INI);
    if (wt_fs_create_file(NUMC3DS_MODELS_INI, (long long)offset)) {
        if (wt_fs_open_file(NUMC3DS_MODELS_INI, 1, &file)) {
            u32 written = 0;
            wt_fs_write(&file, 0, buffer, (u32)offset, &written);
            wt_fs_close_file(&file);
        }
    }
    copy_text(s_models_ini_cache, buffer, sizeof(s_models_ini_cache));
}

int skin_service_init(void)
{
    if (!wt_fs_mount_sdmc()) return -1;
    if (!wt_fs_mkdirs(NUMC3DS_SKINS_DIR)) return -2;
    return skin_service_scan_skins();
}

int skin_service_scan_skins(void)
{
    WtFsDirectory dir;
    WtFsDirectoryEntry entries[WT_FS_DIRECTORY_BATCH];
    int count = 0;
    char filename[128];
    char bjson_filename[128];
    char bjson_path[128];
    int is_dir;

    s_custom_skin_count = 0;
    load_models_ini();

    if (!wt_fs_open_directory(NUMC3DS_SKINS_DIR, &dir)) {
        return 0;
    }

    while (wt_fs_read_directory(&dir, entries, WT_FS_DIRECTORY_BATCH, &count) && count > 0) {
        int i;
        for (i = 0; i < count && s_custom_skin_count < NUMC3DS_MAX_CUSTOM_SKINS; i++) {
            if (wt_fs_entry_is_directory(&entries[i])) continue;
            if (!wt_fs_entry_name_utf8(&entries[i], filename, sizeof(filename))) continue;

            /* Check for .3dst texture */
            if (str_ends_with(filename, ".3dst")) {
                CustomSkinEntry *entry = &s_custom_skins[s_custom_skin_count];
                unsigned name_len = text_len(filename) - 5; /* Strip .3dst */

                if (name_len >= sizeof(entry->name)) name_len = sizeof(entry->name) - 1;
                cp(entry->name, filename, name_len);
                entry->name[name_len] = '\0';

                if (!join_path(entry->texture_path, sizeof(entry->texture_path),
                               NUMC3DS_SKINS_DIR, filename)) continue;

                /* Check for matching <name>.bjson */
                bjson_filename[0] = '\0';
                {
                    unsigned blen = 0;
                    append(bjson_filename, &blen, entry->name);
                    append(bjson_filename, &blen, ".bjson");
                }
                if (!join_path(bjson_path, sizeof(bjson_path),
                               NUMC3DS_SKINS_DIR, bjson_filename)) continue;

                if (wt_fs_exists(bjson_path, &is_dir) && !is_dir) {
                    /* Advanced 3D Skin with custom geometry */
                    unsigned glen = 0;
                    copy_text(entry->geometry_path, bjson_path, sizeof(entry->geometry_path));
                    entry->geometry_name[0] = '\0';
                    append(entry->geometry_name, &glen, "geometry.");
                    append(entry->geometry_name, &glen, entry->name);
                    entry->model_type = SKIN_MODEL_CUSTOM_3D;
                } else {
                    /* Standard humanoid skin - check ini preference */
                    entry->geometry_path[0] = '\0';
                    entry->model_type = (str_ends_with(entry->name, "_Slim") ||
                                         str_ends_with(entry->name, "_slim"))
                                            ? SKIN_MODEL_SLIM
                                            : (numc3ds_u32)query_model_ini(entry->name);
                    if (entry->model_type == SKIN_MODEL_SLIM) {
                        copy_text(entry->geometry_name, "geometry.humanoid.customSlim", sizeof(entry->geometry_name));
                    } else {
                        copy_text(entry->geometry_name, "geometry.humanoid.custom", sizeof(entry->geometry_name));
                    }
                }

                entry->is_valid = 1;
                entry->skin_instance = 0;
                entry->texture_ptr = 0;
                entry->geometry_ptr = 0;
                s_custom_skin_count++;
            }
        }
    }

    wt_fs_close_directory(&dir);
    return s_custom_skin_count;
}

int skin_service_get_count(void)
{
    return s_custom_skin_count;
}

CustomSkinEntry *skin_service_get_entry(int index)
{
    if (index < 0 || index >= s_custom_skin_count) return 0;
    return &s_custom_skins[index];
}

int skin_service_get_hovered_index(void)
{
    return s_hovered_skin_index;
}

void skin_service_set_hovered_index(int index)
{
    s_hovered_skin_index = index;
}

CustomSkinEntry *skin_service_get_hovered_entry(void)
{
    if (s_hovered_skin_index < 0 || s_hovered_skin_index >= s_custom_skin_count) return 0;
    return &s_custom_skins[s_hovered_skin_index];
}

int skin_service_toggle_model(int index)
{
    CustomSkinEntry *entry = skin_service_get_entry(index);
    if (!entry) return -1;

    /* Advanced 3D skins cannot be toggled into slim/normal */
    if (entry->model_type == SKIN_MODEL_CUSTOM_3D) {
        return (int)entry->model_type;
    }

    /* Toggle between Regular (Steve) and Slim (Alex) */
    if (entry->model_type == SKIN_MODEL_REGULAR) {
        entry->model_type = SKIN_MODEL_SLIM;
        copy_text(entry->geometry_name, "geometry.humanoid.customSlim", sizeof(entry->geometry_name));
    } else {
        entry->model_type = SKIN_MODEL_REGULAR;
        copy_text(entry->geometry_name, "geometry.humanoid.custom", sizeof(entry->geometry_name));
    }

    save_models_ini();
    return (int)entry->model_type;
}

const char *skin_service_get_model_name(int model_type)
{
    switch (model_type) {
        case SKIN_MODEL_SLIM:
            return "Slim";
        case SKIN_MODEL_CUSTOM_3D:
            return "3D Custom";
        case SKIN_MODEL_REGULAR:
        default:
            return "Normal";
    }
}
