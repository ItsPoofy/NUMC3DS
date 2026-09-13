#include "../native_selector_registry.h"
#include "../../internal.h"

typedef struct NativeSelectorNode {
    u8 color;
    u8 reserved[3];
    struct NativeSelectorNode *parent,*left,*right;
    char key;
    u8 key_padding[3];
    NativeGstdString value;
} NativeSelectorNode;

typedef struct {
    void *blocks,*free_list,*next,*end;
    NativeSelectorNode *header;
    u32 count;
    u8 reserved[4];
} NativeSelectorMap;

typedef void *(*SelectorMapCloneFn)(NativeSelectorMap*,NativeSelectorNode*,NativeSelectorNode*);

static NativeSelectorNode *selector_leftmost(NativeSelectorNode *node){while(node&&node->left)node=node->left;return node;}
static NativeSelectorNode *selector_rightmost(NativeSelectorNode *node){while(node&&node->right)node=node->right;return node;}

int native_selector_registry_load(void){
    static const char keys[4]={'a','e','p','r'};
    static const char *const names[4]={"allPlayers","allEntities","nearestPlayer","randomPlayer"};
    NativeSelectorMap *map=(NativeSelectorMap*)0x00AC6C6Cu;NativeSelectorNode source[4],*root;u32 scratch=0;unsigned index;
    if(!map||!map->header)return 0;if(map->count)return map->count==4;zero(source,sizeof(source));
    for(index=0;index<4;index++){source[index].key=keys[index];((StrCtor)SEAM_StrCtor)(&source[index].value,names[index],&scratch);scratch=0;if(!source[index].value.handle)goto fail;}
    source[1].left=&source[0];source[0].parent=&source[1];source[1].right=&source[2];source[2].parent=&source[1];source[2].right=&source[3];source[3].parent=&source[2];
    root=((SelectorMapCloneFn)SEAM_CommandTargetTypeMap_cloneSubtree)(map,&source[1],map->header);
    for(index=0;index<4;index++)if(source[index].value.handle)((StrDtor)SEAM_StrDtor)(&source[index].value);
    if(!root)return 0;map->header->parent=root;map->header->left=selector_leftmost(root);map->header->right=selector_rightmost(root);map->count=4;return 1;
fail:
    for(index=0;index<4;index++)if(source[index].value.handle)((StrDtor)SEAM_StrDtor)(&source[index].value);return 0;
}
