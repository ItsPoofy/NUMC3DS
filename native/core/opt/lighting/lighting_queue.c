#include "lighting_queue.h"
#include "../../state.h"

typedef struct {
    u32 *front;
    u32 *storage;
    u32 *limit;
    u32 reserved_front;
    u32 *back;
    u32 reserved_back[3];
    u32 count;
    u32 reserved_map;
    u32 capacity;
} LightingQueue;

typedef void *(*QueueAllocateFn)(u32, u32);
typedef void (*QueueFreeFn)(void *, u32, u32);

typedef char QueueSizeCheck[sizeof(LightingQueue) == 44 ? 1 : -1];
typedef char QueueCountCheck[__builtin_offsetof(LightingQueue, count) == 32 ? 1 : -1];

static NuMC3DS_Hook queue_hooks[3];

static void queue_push(LightingQueue *queue, const u32 *entry)
{
    u32 value = *entry;
    if (queue->count == queue->capacity) {
        u32 capacity = queue->capacity ? queue->capacity * 2u : 64u;
        u32 *storage = ((QueueAllocateFn)0x001010CBu)(capacity * sizeof(u32), 0u);
        u32 *source = queue->front;
        u32 index;
        for (index = 0; index < queue->count; index++) {
            storage[index] = *source++;
            if (source == queue->limit) source = queue->storage;
        }
        if (queue->storage) {
            ((QueueFreeFn)0x001007D1u)(queue->storage, queue->capacity, 0u);
        }
        queue->storage = storage;
        queue->front = storage;
        queue->back = storage + queue->count;
        queue->limit = storage + capacity;
        queue->capacity = capacity;
    }
    *queue->back++ = value;
    if (queue->back == queue->limit) queue->back = queue->storage;
    queue->count++;
}

static void queue_pop(LightingQueue *queue)
{
    queue->front++;
    if (queue->front == queue->limit) queue->front = queue->storage;
    queue->count--;
}

static LightingQueue *queue_destroy(LightingQueue *queue)
{
    if (queue->storage) {
        ((QueueFreeFn)0x001007D1u)(queue->storage, queue->capacity, 0u);
    }
    queue->front = 0;
    queue->storage = 0;
    queue->limit = 0;
    queue->back = 0;
    queue->count = 0;
    queue->capacity = 0;
    return queue;
}

int lighting_queue_install_hooks(void)
{
    static const u32 targets[3] = { 0x008EA97Cu, 0x008EA8CCu, 0x008EAB8Cu };
    static const u32 expected[3][2] = {
        { 0xE92D5FF0u, 0xE1A04000u },
        { 0xE5901000u, 0xE5902020u },
        { 0xE92D41F0u, 0xE1A06000u }
    };
    u32 replacements[3];
    u32 index;
    replacements[0] = (u32)queue_push;
    replacements[1] = (u32)queue_pop;
    replacements[2] = (u32)queue_destroy;
    for (index = 0; index < 3; index++) {
        const volatile u32 *target = (const volatile u32 *)targets[index];
        if (target[0] != expected[index][0] || target[1] != expected[index][1]) return -43;
    }
    for (index = 0; index < 3; index++) {
        NuMC3DS_Hook *hook = &queue_hooks[index];
        hook->target = targets[index];
        hook->replacement = replacements[index];
        hook->expected[0] = expected[index][0];
        hook->expected[1] = expected[index][1];
        if (s->host.install_hook(hook)) return -43;
    }
    return 0;
}
