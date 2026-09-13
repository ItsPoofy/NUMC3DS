#ifndef NUMC3DS_RT_H
#define NUMC3DS_RT_H
#include "../include/numc3ds_abi.h"
#include "seams.h"

typedef unsigned int u32; typedef int s32; typedef unsigned short u16; typedef unsigned char u8;

enum { MAGIC=0x5452754E, CHAT=3, KEY0=10, KEYS=36, CAPS=46, DEL=47, SPACE=48, SEND=49, BACK=50, SLASH=51, PERIOD=52, MODE=53, LEFT=54, RIGHT=55, UP=56, DOWN=57, TAB=59, AT_SIGN=60, TILDE=61, CARET=62, COMMAND_KEY=63, CONTROLS=17, KEYBOARD_ROWS=7, KEYBOARD_LABEL_RUNS=KEYS+CONTROLS, KEYBOARD_SYMBOLS=8, ORIGINALS=32, WORLD_GAMERULE_CONTROLS=8, WORLD_CHEATS=0x3f0, OPTIONS_CHEATS=0x3f1, OPTIONS_ALWAYS_DAY=0x3f2, OPTIONS_HIDE_HAND=0x3f3, OPTIONS_HIDE_HUD=0x3f4, OPTIONS_SHOW_FPS=0x3f6, OPTIONS_LABEL_CAP=48, MAX_TEXT=120, HISTORY=100, SOURCE_TEXT=31, WRAPPED_LINES=120, CHAT_VISIBLE_LINES=11, AUTOCOMPLETE_MAX_OPTIONS=64, AUTOCOMPLETE_VISIBLE_LINES=12, AUTOCOMPLETE_TEXT=64, PROGRESS_TIP_LINES=5, EXEC_TARGET_CAPACITY=128 };

/* == shared types == */
typedef struct { void *object, *control; } Shared;
typedef struct { int x,y,w,h; } Rect;
typedef struct { float r,g,b,a; } Color;
typedef struct { unsigned char type; unsigned char reserved[3]; u32 age; char source[SOURCE_TEXT+1],message[MAX_TEXT+1]; } HistoryEntry;
typedef struct { u32 handle; int width; u8 ready, reserved[3]; void *font; char text[152]; } UiCachedText;
typedef struct { u32 words[25]; void *texture; u8 ready, reserved[3]; } UiMeshCache;
typedef struct { UiMeshCache empty, fill; void *material, *texture; int texture_width, texture_height, source_fill; u8 ready, reserved[3]; } UiXpProgressBar;
typedef struct { u32 lines[PROGRESS_TIP_LINES]; int widths[PROGRESS_TIP_LINES]; u32 next_change, rng; int current; u8 line_count, reserved[3]; } UiProgressTips;
typedef struct { const char*name; int id; } NameId;

/* == game function pointer typedefs == */
typedef int (*ClearInventoryFn)(void*,int);
typedef int (*GetShortFn)(void*,void*);
typedef int (*HasCommandsEnabled)(void*);
typedef int (*RuleGetBoolFn)(void*);
typedef int (*InvAddItemFn)(void*,void*);
typedef int (*IsNullFn)(void*);
typedef int (*LevelGetInt)(void*);
typedef int (*ListSizeFn)(void*);
typedef int (*AllowsHotbarInputFn)(void*);
typedef int (*ButtonIsPressedFn)(void*,int,int);
typedef int (*ClientInputLookupFn)(void*,void*);
typedef int (*SetBlockIdDataFn)(void*,void*,unsigned char*,unsigned char,int,void*);
typedef int (*SignedDivide)(int,int);
typedef int (*TopSolidFn)(void*,void*,int);
typedef numc3ds_u32 (*RequestCommandExecution)(void*,void**,void*,void*);
typedef unsigned (*HashCodeFn)(const char*);
typedef unsigned char (*GetPermissionsLevel)(void*);
typedef void (*AddEffectFn)(void*,void*);
typedef void (*AppendTagFn)(void*,void*);
typedef void (*BuildSpawnFn)(numc3ds_u32*,numc3ds_u32*,numc3ds_u32*,numc3ds_u32*);
typedef void (*ChunkBlockPosCtorFn)(void*,void*);
typedef void (*ChunkPosCtorFn)(void*,void*);
typedef void (*ContainerAddChild)(void*,void*,int);
typedef void (*CopyEntityName)(void*,void*);
typedef void (*CreateWorldStart)(void*,void*);
typedef void (*EffectCtorFn)(void*,int,int,int,int,int);
typedef void (*EntityTeleport)(void*,const float*,int,int);
typedef void (*FillRect)(void*,int,int,int,int,const Color*);
typedef void (*GuiShaderColorFn)(void*,const void*);
typedef void (*GetBlockIdFn)(unsigned char*,void*,void*);
typedef void (*GetBlockIdAndDataFn)(unsigned char*,void*,void*);
typedef void (*GuiButtonBuildBackground)(void*);
typedef void (*HudButtonFactory)(Shared*,void*,unsigned char*,int*,int*,int*,int*);
typedef void (*HudButtonSkin)(void*,void*,void*,Rect*,Rect*,Rect*,Rect*,Rect*,int,int,int,int);
typedef void (*LevelSetInt)(void*,int);
typedef void (*LevelDestroyBlockFn)(void*,void*,const int*,int);
typedef void *(*LocalizationGetFn)(void*,void*,void*);
typedef void (*OptionsSetIntFn)(void*,const void*,int);
typedef float (__attribute__((pcs("aapcs-vfp"))) *OptionsGetFloatFn)(void*,const void*);
typedef void (*LoopbackSend)(void*,void*);
typedef void (*PacketDtor)(void*);
typedef void (*PlayerSetGameType)(void*,int);
typedef void (*PlayerSetRespawnPositionFn)(void*,const int*,int);
typedef void (*PushFn)(void*,Shared*); typedef void (*ReleaseFn)(Shared*);
typedef void (*PutListFn)(void*,void*,void*);
typedef void (*PutShortFn)(void*,void*,int);
typedef void (*ResourceLocationCtor)(void*,const char*,int);
typedef void (*RuleSetBoolFn)(void*,int);
typedef void (*RuleSetIntFn)(void*,int);
typedef void (*SetCommandsEnabled)(void*,int);
typedef void (*SetPermissionsLevel)(void*,unsigned char);
typedef void (*StrAssign)(void*,const void*);
typedef void (*TagCtorFn)(void*);
typedef void (*TexturePtrAssign)(void*,void*);
typedef void (*TexturePtrDtor)(void*);
typedef void (*TessellatorBeginFn)(void*,u32);
typedef void (*TessellatorDrawTextureFn)(void*,void*,void*);
typedef void (*TessellatorEndFn)(void*,void*,void*,int);
typedef void (*MeshRenderTexturedFn)(void*,void*,void*,u32,u32);
typedef void (*MeshDtorFn)(void*);
typedef void (*MinecraftGamePushScreenFn)(void*,Shared*,int);
typedef void (*MinecraftGameSchedulePopScreenFn)(void*,int);
typedef int (*FontUsesDstColorFn)(void*);
typedef void (*ThisFn)(void*); typedef void (*PressFn)(void*,void*);
typedef void (*WorldCreatePressFn)(void*,void*,void*,void*);
typedef void (*WorldEditPressFn)(void*,void*);
typedef void (*XpFn)(void*,int);
typedef void *(*BlockLookupFn)(void*,int);
typedef void *(*ControlAlloc)(void); typedef void (*ControlFn)(void*);
typedef void *(*DamageSourceCtor)(void*,int);
typedef void *(*DefFromIdFn)(int,int);
typedef void *(*GameAllocWithSelector)(numc3ds_u32,void*);
typedef void *(*GetCompoundFn)(void*,int);
typedef void *(*GetDimensionFn)(void*,int);
typedef void *(*GetInventoryFn)(void*);
typedef void *(*GetLevel)(void*);
typedef void *(*MinecraftGameGetOptionsFn)(void*);
typedef void *(*GetLevelChunkFn)(void*,void*);
typedef void *(*GetListFn)(void*,void*);
typedef void *(*GetPlayer)(void*); typedef void (*PacketCtor)(void*,unsigned char,void*,void*,void*);
typedef void *(*ClientInputHandlerFn)(void*);
typedef void *(*GetSuppliesFn)(void*);
typedef void (*EntityDefinitionInitByIdFn)(void*,int);
typedef void *(*GuiButtonCtor)(void*,void*,int,int,int,int,int,const char*,int,int);
typedef void *(*GuiTextCtor)(void*,void*,void*,void*,int,int,int,int,int);
typedef void *(*InvGetItemFn)(void*,int);
typedef void *(*ItemCountAuxCtorFn)(void*,void*,int,int);
typedef int (*ItemInstanceGetMaxStackFn)(void*);
typedef void (*ItemInstanceDtorFn)(void*);
typedef int (*FillingContainerAddFn)(void*,void*,int);
typedef void (*EntityDropFn)(void*,void*,int);
typedef void *(*ListCtorFn)(void*);
typedef void *(*PlayerCommandOriginCtor)(void*,void*);
typedef void *(*SpawnMobFn)(void**,void*,void*,void*,const float*,int,int,int);
typedef void *(*StrCtor)(void*,const char*,void*); typedef void (*StrDtor)(void*);
typedef void *(*TexturePathStateInit)(void*,void*,void*,int);
typedef void *(*TexturePtrCopyCtor)(void*,void*);
typedef void *(*TexturePtrCtor)(void*);
typedef void *(*WarningScreenCtor)(void*,void*,void*,int,int,int);
typedef void *(*WorldSwitchButtonCtor)(void*,void*,int,void*,int);
typedef void *(*ScreenCtorFn)(void*,void*,void*);
typedef void *(*SpriteCtorFn)(void*,void*,int,int,int,int,void*,int,int,int,int);
typedef void *(*TexturePtrDerefFn)(void*);
typedef void (*GamePlaySoundFn)(void*,u32,float,float);
typedef int (*ItemInstanceIdFn)(void*);
typedef void (*RemoveSlotFn)(void*,int);
typedef void (*SubFn)(void*,int);
typedef int (__attribute__((pcs("aapcs-vfp"))) *TextWidth)(void*,void*,int,float);
typedef void (__attribute__((pcs("aapcs-vfp"))) *DrawText)(void*,float,float,float,const void*,const Color*,int,u32);
typedef void (__attribute__((pcs("aapcs-vfp"))) *NinePatchSetSizeFn)(void*,float,float);
typedef void (__attribute__((pcs("aapcs-vfp"))) *TessellatorVertexUvFn)(void*,float,float,float,float,float);
typedef void (__attribute__((pcs("aapcs-vfp"))) *RuleSetFloatFn)(void*,float);
typedef void (__attribute__((pcs("aapcs-vfp"))) *GamePlaySoundFn2)(void*,u32,float,float);
typedef void (__attribute__((pcs("aapcs-vfp"))) *SoundEnginePlayFn)(void*,void*,int,float,float,float,float,float);
typedef float (__attribute__((pcs("aapcs-vfp"))) *EntityDistanceToVec3Fn)(void*,const float*);
/* == state == */
#include "state.h"

/* == rt helpers == */
void cp(void*d,const void*v,u32 n); void zero(void*d,u32 n);
void copy_text(char*d,const char*t,unsigned max);
unsigned text_len(const char*t); void history_clear(void);
void history_add(unsigned char type,const char*source,const char*message);
int streq(const char*a,const char*b);
const char *spaces(const char*p); const char *word(const char*p,char*out,unsigned cap);
int integer(const char**at,int*out); int decimal(const char**at,float base,float*out);
void append(char*out,unsigned*len,const char*text); void append_int(char*out,unsigned*len,int value);
int divide(int value,int divisor); int fail_syntax(void); int fail_command(const char*message); int fail_command_localized(const char*key); int fail_command_localized_args(const char*key,const char*const*arguments,unsigned argument_count); int fail_number_too_small(int value,int minimum); int fail_number_too_big(int value,int maximum);
void result_text(const char*t); void result_number(const char*prefix,int value);
const char*strip_ns(const char*n); float pos_base(void*player,int axis);
u32 make_str(u32*out,const char*t); void drop_str(u32*h); void release_obj(void*o);
void *map_find_int(void*map,u32 key); int name_ieq(const char*a,const char*b);
void entity_name(void*e,char*out,unsigned cap); float entity_pos(void*e,int axis);
unsigned resolve_selector(void*executor,void*level,void*game,const char*token,void**out,unsigned max);
void *item_by_name(const char*name); void *item_by_id(int id);
void *item_from_arg(const char*name);
void *block_by_name(const char*name); void *supplies(void*player);
void *inventory_of(void*player); void *world_source(void*level);
int send_command_chat(void*player,const char*s_text,const char*message);
void action_message(void*player,const char*message);
int apply_enchant(void*level,void*target,int eid,int lvl);
void *rule_dfs(void*node,const char*name); void *rules_root(void*rules);
int gamerule_set(void*rules,const char*name,const char*value);
int rule_value_bool(void*rule); void title_apply_local(const char*op,const char*text_arg);
int resolve_block_arg(char*name,unsigned char*idOut);
int boxes_overlap(const int lo1[3],const int hi1[3],const int lo2[3],const int hi2[3]);
int topsolid(void*src,const int*pos); u32 rng_next(void);
void item_not_found(const char*name); float entity_dist2(void*e,void*ref);
unsigned collect_targets(void*executor,void*level,const char*token,void**out,unsigned max);
unsigned collect_world_entities(void*game,void**out,unsigned max);
extern const char command_failed[]; extern const char command_syntax[];


/* == additional shared declarations == */
extern const char command_sent[];
extern const char command_unavailable[];
extern const char command_permission[];
extern const char sent[];
extern const char *const lower[36];
int chat_ui_install_hooks(void);
int ui_text_style_install_hooks(void);
int world_settings_ui_install_hooks(void);
int progress_screen_ui_install_hooks(void);
int options_ui_install_hooks(void);
int hand_visibility_install_hook(void);
int native_command_system_install_hook(void);
typedef void (*WeatherBroadcastFn)(void*,u32,const float*,int,int);
#endif
