#include "chunk_storage_fast.h"
#include "../../state.h"

#define SEAM_ChunkStorageRecordMap_find 0x008E4C6Cu
#define SEAM_FixedSlotStorageDB_decodeRecord 0x001BEB80u
#define SEAM_FixedSlotStorageDB_loadRecord 0x001BEDC4u
#define SEAM_FixedSlotStorageDB_saveRecord 0x001BEF58u

#define CHUNK_RECORD_CACHE_SIZE 8
#define SECTOR_LRU_CAPACITY 8
#define SECTOR_LRU_MAX_SLOT_SIZE 8192

typedef struct {
    void *map;
    u32 key;
    void *val_ptr;
} ChunkRecordMruEntry;

typedef struct {
    u32 valid;
    void *storage_ptr;
    u16 slot_file_index;
    u16 block_offset;
    u32 slot_size;
    u32 lru_tick;
    u8 *data;
} SectorLruEntry;

static ChunkRecordMruEntry s_record_cache[CHUNK_RECORD_CACHE_SIZE];
static u32 s_cache_head = 0;

static SectorLruEntry s_sector_lru[SECTOR_LRU_CAPACITY];
static u32 s_lru_clock = 0;
static u8 s_sector_pool[SECTOR_LRU_CAPACITY * SECTOR_LRU_MAX_SLOT_SIZE];
static int s_sector_pool_ready;

static NuMC3DS_Hook s_storage_hooks[4];

typedef void *(*RecordMapFindFn)(void *map, const u32 *key_ptr);
typedef int (*ValidateMagicFn)(u32 magic);
typedef int (*ValidateVersionFn)(u32 magic);
typedef void *(*StringReplaceFn)(void *str, u32 off, u32 n0, const char *ptr, u32 ptr_len, u32 off2, u32 count);
typedef int (*ReadFileFn)(void *stream, void *dst, u32 size);
typedef void (*LockFn)(void *lock);
typedef void (*UnlockFn)(void *lock);
typedef void (*LookupFn)(u16 *meta, void *storage, const u32 *key);
typedef u32 (*ReadSlotFn)(void *storage, u32 slot_idx, u32 offset, void *stream);
typedef u32 (*ReleaseSlotFn)(void *storage, u32 slot_idx, void *stream);
typedef void *(*GetBufFn)(void *stream);
typedef int (*ValidateChunkFn)(void *record);
typedef int (*SaveFn)(void *storage, void *record);

static void *chunk_storage_record_map_find_fast(void *map, const u32 *key_ptr)
{
    if (!map || !key_ptr) return 0;
    u32 key = *key_ptr;

    /* Check MRU cache */
    for (int i = 0; i < CHUNK_RECORD_CACHE_SIZE; i++) {
        if (s_record_cache[i].map == map && s_record_cache[i].key == key) {
            return s_record_cache[i].val_ptr;
        }
    }

    /* Call original red-black tree find */
    void *result = ((RecordMapFindFn)s_storage_hooks[0].trampoline)(map, key_ptr);

    if (result) {
        u32 slot = s_cache_head++ & (CHUNK_RECORD_CACHE_SIZE - 1);
        s_record_cache[slot].map = map;
        s_record_cache[slot].key = key;
        s_record_cache[slot].val_ptr = result;
    }

    return result;
}

static int sector_lru_ensure_pool(void)
{
    if (s_sector_pool_ready) return 1;
    for (u32 i = 0; i < SECTOR_LRU_CAPACITY; i++) {
        s_sector_lru[i].data = s_sector_pool + i * SECTOR_LRU_MAX_SLOT_SIZE;
        s_sector_lru[i].valid = 0;
    }
    s_sector_pool_ready = 1;
    return 1;
}

static SectorLruEntry *sector_lru_find(void *storage_ptr, u16 slot_file_index, u16 block_offset)
{
    if (!s_sector_pool_ready) return 0;
    for (u32 i = 0; i < SECTOR_LRU_CAPACITY; i++) {
        if (s_sector_lru[i].valid &&
            s_sector_lru[i].storage_ptr == storage_ptr &&
            s_sector_lru[i].slot_file_index == slot_file_index &&
            s_sector_lru[i].block_offset == block_offset) {
            return &s_sector_lru[i];
        }
    }
    return 0;
}

static void sector_lru_insert(void *storage_ptr, u16 slot_file_index, u16 block_offset, u32 slot_size, const u8 *src)
{
    if (slot_size > SECTOR_LRU_MAX_SLOT_SIZE || !src) return;
    if (!sector_lru_ensure_pool()) return;

    SectorLruEntry *entry = sector_lru_find(storage_ptr, slot_file_index, block_offset);
    if (!entry) {
        u32 oldest_idx = 0;
        u32 oldest_tick = 0xFFFFFFFFu;
        for (u32 i = 0; i < SECTOR_LRU_CAPACITY; i++) {
            if (!s_sector_lru[i].valid) {
                oldest_idx = i;
                break;
            }
            if (s_sector_lru[i].lru_tick < oldest_tick) {
                oldest_tick = s_sector_lru[i].lru_tick;
                oldest_idx = i;
            }
        }
        entry = &s_sector_lru[oldest_idx];
    }

    entry->valid = 1;
    entry->storage_ptr = storage_ptr;
    entry->slot_file_index = slot_file_index;
    entry->block_offset = block_offset;
    entry->slot_size = slot_size;
    entry->lru_tick = ++s_lru_clock;
    for (u32 j = 0; j < slot_size; j++) {
        entry->data[j] = src[j];
    }
}

static void sector_lru_invalidate(void *storage_ptr)
{
    for (u32 i = 0; i < SECTOR_LRU_CAPACITY; i++) {
        if (s_sector_lru[i].storage_ptr == storage_ptr) {
            s_sector_lru[i].valid = 0;
        }
    }
}

static int numc3ds_decode_record_payload(void *chunk_record, const u8 *sector_data, u32 *out_status)
{
    ValidateMagicFn val_magic = (ValidateMagicFn)0x00689F9Cu;
    ValidateVersionFn val_ver = (ValidateVersionFn)0x00689FC8u;
    /* std_string_replace_cstr_range is Thumb code; set bit 0 for BX/BLX interworking */
    StringReplaceFn str_replace = (StringReplaceFn)0x00101121u;

    u32 magic = *(const u32 *)sector_data;
    int ver = val_ver(magic);
    if (ver != 0 && ver != 1) {
        return 0;
    }

    u8 desc_buf[100];
    const u8 *src_desc = sector_data + 12;
    for (int j = 0; j < 100; j++) {
        desc_buf[j] = src_desc[j];
    }

    *(u8 *)chunk_record = desc_buf[0];

    for (u32 i = 0; i < 6; i++) {
        const u32 *desc = (const u32 *)(desc_buf + i * 16);
        u32 offset = desc[2];
        u32 len = desc[3];
        void *str_obj = (void *)((u8 *)chunk_record + 0x40 + i * 4);
        if (len != 0) {
            const char *src = (const char *)(sector_data + offset);
            /* Clear existing string */
            str_replace(str_obj, 0, (u32)-1, "", 0, 0, 0);
            /* Assign directly from sector without intermediate heap malloc/free */
            str_replace(str_obj, 0, 0, src, len, 0, len);
            *(u32 *)((u8 *)chunk_record + 0x10 + i * 4) = *(const u32 *)(desc_buf + 16 + i * 16);
        }
    }

    if (out_status) {
        *out_status = (u32)ver;
    }
    return 1;
}

static int numc3ds_decode_record_fast(
    void *this_ptr,
    void *stream,
    void *chunk_record,
    u32 slot_size,
    u32 is_cached,
    u32 buf_or_offset,
    u32 *out_status
)
{
    const u8 *sector_data;

    if (is_cached) {
        u8 *cached_buf = *(u8 **)((u8 *)this_ptr + 160);
        if (!cached_buf) return 0;
        sector_data = cached_buf + buf_or_offset;
    } else {
        void *map = (u8 *)this_ptr + 0x4C;
        void **slot = (void **)chunk_storage_record_map_find_fast(map, &slot_size);
        if (!slot) {
            slot = (void **)((RecordMapFindFn)s_storage_hooks[0].trampoline)(map, &slot_size);
        }
        if (!slot) return 0;
        void *sl = *slot;
        if (!sl) {
            sl = ((void *(*)(u32))0x00124DE0u)(slot_size);
            if (!sl) return 0;
            *slot = sl;
        }

        ReadFileFn read_fn = (ReadFileFn)0x001AAEC0u;
        if (!read_fn(stream, sl, slot_size)) {
            return 0;
        }
        sector_data = (const u8 *)sl;
    }

    return numc3ds_decode_record_payload(chunk_record, sector_data, out_status);
}

static int numc3ds_load_record_fast(void *this_ptr, void *chunk_record, u32 *out_status)
{
    LockFn fn_lock = (LockFn)0x00123170u;
    UnlockFn fn_unlock = (UnlockFn)0x001231B0u;
    LookupFn fn_lookup = (LookupFn)0x008B5C78u;
    ValidateChunkFn fn_val_chunk = (ValidateChunkFn)0x003DDD08u;

    void *lock = (u8 *)this_ptr + 20;
    fn_lock(lock);

    s32 x = *(s32 *)((u8 *)chunk_record + 4);
    s32 z = *(s32 *)((u8 *)chunk_record + 8);
    u32 dim = *(u32 *)((u8 *)chunk_record + 12);
    u32 key = (dim << 28) | (((z + 8192) & 0x3FFF) << 14) | ((x + 8192) & 0x3FFF);

    u32 meta[6];
    fn_lookup((u16 *)meta, this_ptr, &key);
    s32 status = (s32)meta[2];
    if (status == -1) {
        fn_unlock(lock);
        return 0;
    }

    u16 slot_file_index = (u16)meta[0];
    u16 block_offset = (u16)(meta[0] >> 16);
    u16 num_sectors = (u16)(meta[1] >> 16);
    u32 slot_size = (u32)num_sectors << 10;
    u32 byte_offset = (u32)block_offset * slot_size;

    u32 is_cached = 0;
    u32 buf_or_offset = 0;
    u8 stream_desc[64];

    u32 stock_cached_slot = *(u32 *)((u8 *)this_ptr + 164);
    if (stock_cached_slot == slot_file_index) {
        is_cached = 1;
        buf_or_offset = byte_offset;
    } else {
        SectorLruEntry *hit = sector_lru_find(this_ptr, slot_file_index, block_offset);
        if (hit) {
            hit->lru_tick = ++s_lru_clock;
            u32 local_status = 0;
            int dec_ok = numc3ds_decode_record_payload(chunk_record, hit->data, &local_status);
            if (dec_ok && fn_val_chunk(chunk_record)) {
                if (out_status) *out_status = local_status;
                fn_unlock(lock);
                return 1;
            }
            fn_unlock(lock);
            return 0;
        }

        u32 *vtable = *(u32 **)this_ptr;
        ReadSlotFn read_slot = (ReadSlotFn)vtable[19];
        if (!read_slot(this_ptr, slot_file_index, byte_offset + 20, stream_desc)) {
            fn_unlock(lock);
            return 0;
        }
        GetBufFn get_buf = (GetBufFn)0x001AAF1Cu;
        buf_or_offset = (u32)get_buf(stream_desc);
    }

    u32 local_out_status = 0;
    int decode_ok = numc3ds_decode_record_fast(
        this_ptr,
        stream_desc,
        chunk_record,
        slot_size,
        is_cached,
        buf_or_offset,
        &local_out_status
    );

    if (!is_cached) {
        if (decode_ok && slot_size <= SECTOR_LRU_MAX_SLOT_SIZE) {
            void *map = (u8 *)this_ptr + 0x4C;
            void *found = chunk_storage_record_map_find_fast(map, &slot_size);
            if (found && *(const u8 **)found) {
                sector_lru_insert(this_ptr, slot_file_index, block_offset, slot_size, *(const u8 **)found);
            }
        }

        u32 *vtable = *(u32 **)this_ptr;
        ReleaseSlotFn rel_slot = (ReleaseSlotFn)vtable[20];
        u32 rel_ok = rel_slot(this_ptr, slot_file_index, stream_desc);
        decode_ok = decode_ok && rel_ok;
    }

    if (!decode_ok) {
        fn_unlock(lock);
        return 0;
    }

    if (!fn_val_chunk(chunk_record)) {
        fn_unlock(lock);
        return 0;
    }

    if (out_status) {
        *out_status = local_out_status;
    }

    fn_unlock(lock);
    return 1;
}

static int numc3ds_save_record_fast(void *this_ptr, void *chunk_record)
{
    int res = ((SaveFn)s_storage_hooks[3].trampoline)(this_ptr, chunk_record);
    sector_lru_invalidate(this_ptr);
    return res;
}

int chunk_storage_fast_install_hooks(void)
{
    s_storage_hooks[0].target = SEAM_ChunkStorageRecordMap_find;
    s_storage_hooks[0].replacement = (u32)chunk_storage_record_map_find_fast;
    s_storage_hooks[0].expected[0] = 0xE92D4010u;
    s_storage_hooks[0].expected[1] = 0xE590C010u;

    s_storage_hooks[1].target = SEAM_FixedSlotStorageDB_decodeRecord;
    s_storage_hooks[1].replacement = (u32)numc3ds_decode_record_fast;
    s_storage_hooks[1].expected[0] = 0xE92D4FF0u;
    s_storage_hooks[1].expected[1] = 0xE24DD07Cu;

    s_storage_hooks[2].target = SEAM_FixedSlotStorageDB_loadRecord;
    s_storage_hooks[2].replacement = (u32)numc3ds_load_record_fast;
    s_storage_hooks[2].expected[0] = 0xE92D47F0u;
    s_storage_hooks[2].expected[1] = 0xE2804014u;

    s_storage_hooks[3].target = SEAM_FixedSlotStorageDB_saveRecord;
    s_storage_hooks[3].replacement = (u32)numc3ds_save_record_fast;
    s_storage_hooks[3].expected[0] = 0xE92D4FF0u;
    s_storage_hooks[3].expected[1] = 0xE2805014u;

    for (int i = 0; i < 4; i++) {
        if (s->host.install_hook(&s_storage_hooks[i])) {
            return -82 - i;
        }
    }
    return 0;
}
