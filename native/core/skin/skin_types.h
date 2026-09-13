#ifndef NUMC3DS_SKIN_TYPES_H
#define NUMC3DS_SKIN_TYPES_H

#include "../../include/numc3ds_abi.h"

typedef unsigned char numc3ds_u8;

#define NUMC3DS_MAX_CUSTOM_SKINS 64
#define NUMC3DS_SKINS_DIR "sdmc:/Minecraft 3DS/skins"
#define NUMC3DS_MODELS_INI "sdmc:/Minecraft 3DS/skins/models.ini"
#define NUMC3DS_SKIN_PACK_NAME "Custom Skins"
#define NUMC3DS_SKIN_PACK_ID "numc3ds_custom_pack"

/* Skin Model Types */
enum {
    SKIN_MODEL_REGULAR   = 0, /* Steve model: 4px arms (geometry.humanoid.custom) */
    SKIN_MODEL_SLIM      = 1, /* Alex model:  3px arms (geometry.humanoid.customSlim) */
    SKIN_MODEL_CUSTOM_3D = 2  /* 3D model:    Loaded from matching <name>.bjson */
};

/* Custom Skin Entry Descriptor */
typedef struct {
    char name[48];              /* Skin display name / identifier */
    char texture_path[96];      /* Full path to .3dst texture */
    char geometry_path[96];     /* Full path to .bjson (empty if standard humanoid) */
    char geometry_name[48];     /* Registered geometry key */
    numc3ds_u32 model_type;     /* SKIN_MODEL_REGULAR, SLIM, or CUSTOM_3D */
    numc3ds_u32 is_valid;       /* 1 if successfully loaded and verified */
    void *skin_instance;        /* Pointer to instantiated Skin object in game heap */
    void *texture_ptr;          /* Pointer to loaded TextureData / TextureGroup entry */
    void *geometry_ptr;         /* Pointer to loaded GeometryData */
} CustomSkinEntry;

#endif /* NUMC3DS_SKIN_TYPES_H */
