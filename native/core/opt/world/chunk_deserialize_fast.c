#include "chunk_deserialize_fast.h"
#include "../../state.h"

static NuMC3DS_Hook chunk_hook;

typedef s32 (*InputGetByteFn)(u8 *stream);
typedef void (*InputReadFn)(u8 *stream, void *destination, u32 size);

static InputReadFn input_read(u8 *stream)
{
    return (InputReadFn)(*(u32 **)stream)[0x28u / 4u];
}

static s32 numc3ds_stream_get_byte_fast(u8 *stream)
{
    signed char value = 0;
    input_read(stream)(stream, &value, 1u);
    return (s32)value;
}

int chunk_deserialize_fast_install_hooks(void)
{
    chunk_hook.target = 0x00250FECu;
    chunk_hook.replacement = (u32)numc3ds_stream_get_byte_fast;
    chunk_hook.expected[0] = 0xE92D4008u;
    chunk_hook.expected[1] = 0xE3A01000u;

    if (s->host.install_hook(&chunk_hook)) {
        return -81;
    }
    return 0;
}
