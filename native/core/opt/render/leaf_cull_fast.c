#include "leaf_cull_fast.h"
#include "../../state.h"

typedef signed char s8;

static NuMC3DS_Hook graphics_mode_hook;
static void *s_options_ptr = 0;
static s8 s_last_fancy = -1;

static int read_leaf_options(int *fancy)
{
    if (__builtin_expect(!s_options_ptr, 0)) {
        if (s && s != (State *)0xFFFFFFFFu && s->magic == MAGIC && s->host.minecraft_game) {
            typedef void *(*get_options_fn)(void *);
            s_options_ptr = ((get_options_fn)0x00225EC4u)(s->host.minecraft_game);
        }
    }
    if (!s_options_ptr) return 0;
    *fancy = *(s8 *)((u8 *)s_options_ptr + 0x6E) != 0;
    *(s8 *)((u8 *)s_options_ptr + 0x6F) = (s8)*fancy;
    return 1;
}

int is_fancy_graphics(void)
{
    int fancy;
    if (read_leaf_options(&fancy)) return fancy;
    return 1;
}

static void apply_leaf_graphics_mode(int fancy)
{
    void *mLeaves = *(void **)0x00A34774u;
    void *mLeaves2 = *(void **)0x00A348A8u;
    typedef void (*leaf_mode_fn)(void *, int, int, int);
    int mode = fancy ? 1 : 0;
    if (mLeaves) {
        ((leaf_mode_fn)0x0067B7D4u)(mLeaves, mode, 0, mode);
    }
    if (mLeaves2) {
        ((leaf_mode_fn)0x0067B7D4u)(mLeaves2, mode, 0, mode);
    }
}

void leaf_refresh_graphics_mode(int force)
{
    int fancy;
    if (!read_leaf_options(&fancy)) return;
    if (force || fancy != (int)s_last_fancy) {
        s_last_fancy = (s8)fancy;
        apply_leaf_graphics_mode(fancy);

        if (s && s != (State *)0xFFFFFFFFu && s->magic == MAGIC && s->host.minecraft_game) {
            void *level_renderer = *(void **)((u8 *)s->host.minecraft_game + 0x4C);
            if (level_renderer) {
                typedef void (*all_changed_fn)(void *);
                ((all_changed_fn)0x00222684u)(level_renderer);
            }
        }
    }
}

static void on_call_on_graphics_mode_changed(int fancy_graphics, int param1, int transparent_leaves)
{
    int fancy = fancy_graphics != 0;
    (void)transparent_leaves;
    s_last_fancy = (s8)fancy;
    if (s_options_ptr) *(s8 *)((u8 *)s_options_ptr + 0x6F) = (s8)fancy;

    ((void (*)(int, int, int))graphics_mode_hook.trampoline)
        (fancy, param1, fancy);

    apply_leaf_graphics_mode(fancy);
}

int leaf_cull_fast_install_hook(void)
{
    graphics_mode_hook.target = 0x005BD9B0u;
    graphics_mode_hook.replacement = (u32)on_call_on_graphics_mode_changed;
    graphics_mode_hook.expected[0] = 0xE92D41F0u;
    graphics_mode_hook.expected[1] = 0xE1A06002u;
    if (s->host.install_hook(&graphics_mode_hook)) return -66;

    leaf_refresh_graphics_mode(1);

    return 0;
}
