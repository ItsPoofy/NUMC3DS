#include "mcpe_item_compat.h"
#include "../session/mcpe_transport_selector.h"
#include "../../hook_manager.h"
#include "../../seams.h"
#include "../../state.h"

typedef void (*ReadItemInstanceFn)(void *, void *);
typedef void (*WriteItemInstanceFn)(void *, const void *);
typedef int (*ReadVarIntFn)(void *);
typedef void (*WriteUnsignedVarIntFn)(void *, u32);
typedef void (*ItemInstanceCtorFn)(void *, int, int, int);
typedef void (*ItemInstanceDefaultCtorFn)(void *);
typedef void (*ItemInstanceDtorFn)(void *);
typedef void (*ItemReadSparseTagsFn)(void *, void *, void *);
typedef void (*ItemWriteSparseTagsFn)(void *, const void *, void *);
typedef int (*GetIdFn)(const void *);

typedef struct {
    u32 vtable;
    void *stream;
} McpeBinaryDataInput;

typedef struct {
    u32 vtable;
    void *stream;
} McpeBinaryDataOutput;

static NuMC3DS_Hook read_item_hook;
static NuMC3DS_Hook write_item_hook;

static void read_item_sparse_tags(void *item, void *instance, void *stream)
{
    McpeBinaryDataInput input;
    input.vtable = SEAM_ReadOnlyBinaryStream_IDataInput_vtable;
    input.stream = stream;
    if (item) {
        void **vtable = *(void ***)item;
        ((ItemReadSparseTagsFn)vtable[51])(item, instance, &input);
    } else {
        ((ItemReadSparseTagsFn)SEAM_Item_readUserDataDefault)(0, instance, &input);
    }
}

static void write_item_sparse_tags(void *item, const void *instance, void *stream)
{
    McpeBinaryDataOutput output;
    void **vtable;

    output.vtable = SEAM_BinaryStream_IDataOutput_vtable;
    output.stream = stream;
    vtable = *(void ***)item;
    ((ItemWriteSparseTagsFn)vtable[52])(item, instance, &output);
}

static void on_read_item_instance(void *out, void *stream)
{
    ReadItemInstanceFn original;
    ReadVarIntFn read_varint;
    int id;
    int aux_and_count;
    int count;
    short aux;
    void *item;

    if (!out || !stream) return;

    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) {
        original = (ReadItemInstanceFn)read_item_hook.trampoline;
        original(out, stream);
        return;
    }

    read_varint = (ReadVarIntFn)SEAM_ReadOnlyBinaryStream_readVarInt;
    id = read_varint(stream);
    if (id <= 0) {
        ((ItemInstanceDefaultCtorFn)SEAM_ItemInstance_defaultCtor)(out);
        return;
    }

    aux_and_count = read_varint(stream);
    count = aux_and_count & 0xFF;
    aux = (short)(aux_and_count >> 8);

    ((ItemInstanceCtorFn)SEAM_ItemInstance_idCountAuxCtor)(out, id, count, (int)aux);
    if (!((u8 *)out)[4]) {
        ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(out);
        ((ItemInstanceDefaultCtorFn)SEAM_ItemInstance_defaultCtor)(out);
        return;
    }

    item = id > 0 && id < 512 ? ((void **)SEAM_ItemRegistry_byNumericId)[id] : 0;
    read_item_sparse_tags(item, out, stream);
    if (!item) {
        ((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(out);
        ((ItemInstanceDefaultCtorFn)SEAM_ItemInstance_defaultCtor)(out);
    }
}

static void on_write_item_instance(void *stream, const void *inst)
{
    WriteItemInstanceFn original;
    int id;
    short aux;
    u8 count;
    int aux_and_count;
    u32 id_zigzag;
    u32 ac_zigzag;
    WriteUnsignedVarIntFn write_uvarint;
    void *item;

    if (!stream) return;

    if (mcpe_transport_selector_kind() != MCPE_TRANSPORT_IPV4) {
        original = (WriteItemInstanceFn)write_item_hook.trampoline;
        original(stream, inst);
        return;
    }

    write_uvarint = (WriteUnsignedVarIntFn)SEAM_BinaryStream_writeUnsignedVarInt;

    if (!inst) {
        write_uvarint(stream, 0);
        return;
    }

    id = ((GetIdFn)SEAM_ItemInstance_getId)(inst);
    if (id <= 0 || !((const u8 *)inst)[4] || ((const u8 *)inst)[0] == 0) {
        write_uvarint(stream, 0);
        return;
    }

    item = ((void **)SEAM_ItemRegistry_byNumericId)[id];

    aux = *(const short *)((const u8 *)inst + 2);
    count = *(const u8 *)inst;
    aux_and_count = (int)aux * 256 | (int)count;

    id_zigzag = (u32)(id << 1) ^ (u32)(id >> 31);
    write_uvarint(stream, id_zigzag);

    ac_zigzag = ((u32)aux_and_count << 1) ^ (u32)(aux_and_count >> 31);
    write_uvarint(stream, ac_zigzag);
    write_item_sparse_tags(item, inst, stream);
}

int mcpe_item_compat_install(void)
{
    zero(&read_item_hook, sizeof(read_item_hook));
    read_item_hook.target = SEAM_ReadOnlyBinaryStream_readItemInstance;
    read_item_hook.replacement = (u32)on_read_item_instance;
    read_item_hook.expected[0] = 0xE92D4FF0u;
    read_item_hook.expected[1] = 0xE24DDF4Du;
    if (s->host.install_hook(&read_item_hook)) return -87;

    zero(&write_item_hook, sizeof(write_item_hook));
    write_item_hook.target = SEAM_BinaryStream_writeItemInstance;
    write_item_hook.replacement = (u32)on_write_item_instance;
    write_item_hook.expected[0] = 0xE92D40F0u;
    write_item_hook.expected[1] = 0xE1A04000u;
    if (s->host.install_hook(&write_item_hook)) return -89;

    return 0;
}
