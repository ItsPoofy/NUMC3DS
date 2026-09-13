#include "../native_registry.h"
#include "../native_schema.h"
#include "../native_callbacks.h"
#include "../native_property_bag.h"
#include "../native_command_loader.h"
#include "../native_selector_registry.h"
#include "../native_command_registration.h"

typedef void *(*MapCloneFn)(NativeCommandMap*,NativeCommandMapNode*,NativeCommandMapNode*);
typedef void *(*NameMapCloneFn)(void*,NativeCommandNameMapNode*,NativeCommandNameMapNode*);
typedef int (*StringCompareFn)(const NativeGstdString*,const NativeGstdString*);
typedef void *(*OperatorNewFn)(u32);
typedef void (*OperatorDeleteFn)(void*);
typedef u32 (*GetCommandFn)(void*,NativeGstdString*,u32,NativeSharedCommand*);
typedef void (*SharedAssignFn)(NativeSharedCommand*,u32,u32);

static NativeCommandMapNode *source_overload_nodes;
static NativeCommandNameMapNode *source_name_nodes;
static NativeSharedCommand *source_command_vectors;
static unsigned *source_command_offsets;
static u32 *source_overload_key_scratch;
static u32 *source_name_key_scratch;
static u32 *source_name_value_scratch;
static unsigned *overload_order;
static unsigned *name_order;
static void *built_parser;
static void *built_commands;

static void *temporary_allocate(unsigned bytes){void *memory=((OperatorNewFn)SEAM_operator_new)(bytes);if(memory)zero(memory,bytes);return memory;}
static void temporary_free(void *memory){if(memory)((OperatorDeleteFn)SEAM_operator_delete)(memory);}

static int text_compare(const char*left,const char*right){
    unsigned char a,b;if(!left)left="";if(!right)right="";
    for(;;left++,right++){a=(unsigned char)*left;b=(unsigned char)*right;if(a!=b)return a<b?-1:1;if(!a)return 0;}
}

static void sort_overload_order(void){
    unsigned i,j,value;for(i=0;i<native_schema_root_count;i++)overload_order[i]=i;
    for(i=1;i<native_schema_root_count;i++){value=overload_order[i];j=i;while(j&&((StringCompareFn)SEAM_gstd_string_compare)(&source_overload_nodes[overload_order[j-1]].key,&source_overload_nodes[value].key)>0){overload_order[j]=overload_order[j-1];j--;}overload_order[j]=value;}
}

static void sort_name_order(void){
    unsigned i,j,value;for(i=0;i<native_schema_map_entry_count;i++)name_order[i]=i;
    for(i=1;i<native_schema_map_entry_count;i++){value=name_order[i];j=i;while(j&&((StringCompareFn)SEAM_gstd_string_compare)(&source_name_nodes[name_order[j-1]].key,&source_name_nodes[value].key)>0){name_order[j]=name_order[j-1];j--;}name_order[j]=value;}
}

static NativeCommandMapNode *build_overload_tree(int low,int high,NativeCommandMapNode*parent){
    int middle;NativeCommandMapNode*node;
    if(low>=high)return 0;middle=low+(high-low)/2;node=&source_overload_nodes[overload_order[middle]];
    node->parent=parent;node->left=build_overload_tree(low,middle,node);node->right=build_overload_tree(middle+1,high,node);return node;
}

static NativeCommandNameMapNode *build_name_tree(int low,int high,NativeCommandNameMapNode*parent){
    int middle;NativeCommandNameMapNode*node;
    if(low>=high)return 0;middle=low+(high-low)/2;node=&source_name_nodes[name_order[middle]];
    node->parent=parent;node->left=build_name_tree(low,middle,node);node->right=build_name_tree(middle+1,high,node);return node;
}

static NativeCommandMapNode *leftmost_overload(NativeCommandMapNode*node){while(node&&node->left)node=node->left;return node;}
static NativeCommandMapNode *rightmost_overload(NativeCommandMapNode*node){while(node&&node->right)node=node->right;return node;}
static NativeCommandNameMapNode *leftmost_name(NativeCommandNameMapNode*node){while(node&&node->left)node=node->left;return node;}
static NativeCommandNameMapNode *rightmost_name(NativeCommandNameMapNode*node){while(node&&node->right)node=node->right;return node;}

static NativeCommandMapNode *find_overload_node(NativeCommandMapNode*header,const char*key){
    NativeCommandMapNode*node=header?header->parent:0;int order;
    while(node){
        order=text_compare(key,node->key.handle?(const char*)node->key.handle:"");
        if(!order)return node;
        node=order<0?node->left:node->right;
    }
    return 0;
}

static NativeCommandNameMapNode *find_name_node(NativeCommandNameMapNode*header,const char*key){
    NativeCommandNameMapNode*node=header?header->parent:0;int order;
    while(node){
        order=text_compare(key,node->key.handle?(const char*)node->key.handle:"");
        if(!order)return node;
        node=order<0?node->left:node->right;
    }
    return 0;
}

static NativeCommandNameMapNode *find_name_node_native(NativeCommandNameMapNode*header,const NativeGstdString*key){
    NativeCommandNameMapNode*node,*candidate;
    if(!header||!key)return 0;
    candidate=header;node=header->parent;
    while(node){
        if(((StringCompareFn)SEAM_gstd_string_compare)(&node->key,key)<0)node=node->right;
        else{candidate=node;node=node->left;}
    }
    if(candidate==header||((StringCompareFn)SEAM_gstd_string_compare)(key,&candidate->key)<0)return 0;
    return candidate;
}

static NativeCommandMapNode *find_overload_node_native(NativeCommandMapNode*header,const NativeGstdString*key){
    NativeCommandMapNode*node,*candidate;
    if(!header||!key)return 0;
    candidate=header;node=header->parent;
    while(node){
        if(((StringCompareFn)SEAM_gstd_string_compare)(&node->key,key)<0)node=node->right;
        else{candidate=node;node=node->left;}
    }
    if(candidate==header||((StringCompareFn)SEAM_gstd_string_compare)(key,&candidate->key)<0)return 0;
    return candidate;
}

static int build_source_nodes(void){
    unsigned root_index,entry_index,version_index,total_versions=0;
    for(root_index=0;root_index<native_schema_root_count;root_index++)total_versions+=native_schema_roots[root_index].version_count;
    source_overload_nodes=(NativeCommandMapNode*)temporary_allocate(sizeof(*source_overload_nodes)*native_schema_root_count);source_name_nodes=(NativeCommandNameMapNode*)temporary_allocate(sizeof(*source_name_nodes)*native_schema_map_entry_count);source_command_vectors=(NativeSharedCommand*)temporary_allocate(sizeof(*source_command_vectors)*total_versions);source_command_offsets=(unsigned*)temporary_allocate(sizeof(*source_command_offsets)*(native_schema_root_count+1));source_overload_key_scratch=(u32*)temporary_allocate(sizeof(*source_overload_key_scratch)*native_schema_root_count);source_name_key_scratch=(u32*)temporary_allocate(sizeof(*source_name_key_scratch)*native_schema_map_entry_count);source_name_value_scratch=(u32*)temporary_allocate(sizeof(*source_name_value_scratch)*native_schema_map_entry_count);overload_order=(unsigned*)temporary_allocate(sizeof(*overload_order)*native_schema_root_count);name_order=(unsigned*)temporary_allocate(sizeof(*name_order)*native_schema_map_entry_count);
    if(!source_overload_nodes||!source_name_nodes||!source_command_vectors||!source_command_offsets||!source_overload_key_scratch||!source_name_key_scratch||!source_name_value_scratch||!overload_order||!name_order)return 0;
    total_versions=0;
    for(root_index=0;root_index<native_schema_root_count;root_index++){
        const NativeSchemaRoot*root=&native_schema_roots[root_index];NativeCommandMapNode*node=&source_overload_nodes[root_index];node->color=1;((StrCtor)SEAM_StrCtor)(&node->key,native_schema_text(root->name),&source_overload_key_scratch[root_index]);
        source_command_offsets[root_index]=total_versions;{const NativeSharedCommand *loaded=native_command_loader_versions(root_index);if(!loaded)return 0;for(version_index=0;version_index<root->version_count;version_index++)source_command_vectors[total_versions++]=loaded[version_index];}
        node->commands.begin=(u32)&source_command_vectors[source_command_offsets[root_index]];node->commands.end=(u32)&source_command_vectors[total_versions];node->commands.capacity=node->commands.end;
    }
    source_command_offsets[native_schema_root_count]=total_versions;
    for(entry_index=0;entry_index<native_schema_map_entry_count;entry_index++){
        const NativeSchemaMapEntry*entry=&native_schema_map_entries[entry_index];const NativeSchemaRoot*root=&native_schema_roots[entry->root_index];NativeCommandNameMapNode*node=&source_name_nodes[entry_index];node->color=1;((StrCtor)SEAM_StrCtor)(&node->key,native_schema_text(entry->name),&source_name_key_scratch[entry_index]);((StrCtor)SEAM_StrCtor)(&node->value,native_schema_text(root->name),&source_name_value_scratch[entry_index]);
    }
    sort_overload_order();sort_name_order();return 1;
}

static void release_source_storage(void){temporary_free(name_order);temporary_free(overload_order);temporary_free(source_name_value_scratch);temporary_free(source_name_key_scratch);temporary_free(source_overload_key_scratch);temporary_free(source_command_offsets);temporary_free(source_command_vectors);temporary_free(source_name_nodes);temporary_free(source_overload_nodes);name_order=0;overload_order=0;source_name_value_scratch=0;source_name_key_scratch=0;source_overload_key_scratch=0;source_command_offsets=0;source_command_vectors=0;source_name_nodes=0;source_overload_nodes=0;}

static void destroy_source_keys(void){
    unsigned index;for(index=0;index<native_schema_root_count;index++)if(source_overload_nodes[index].key.handle)((StrDtor)SEAM_StrDtor)(&source_overload_nodes[index].key);
    for(index=0;index<native_schema_map_entry_count;index++){
        if(source_name_nodes[index].key.handle)((StrDtor)SEAM_StrDtor)(&source_name_nodes[index].key);
        if(source_name_nodes[index].value.handle)((StrDtor)SEAM_StrDtor)(&source_name_nodes[index].value);
    }
}

void *native_registry_commands_for_game(void*game){
    void*client,*owner;
    if(!game)return 0;
    client=((void*(*)(void*))SEAM_MinecraftGame_getClientInstance)(game);if(!client)return 0;
    owner=*(void**)((u8*)client+SEAM_ClientInstance_storageOwnerOffset);if(!owner)return 0;
    return *(void**)((u8*)owner+SEAM_StorageOwner_commandsOffset);
}

static int registry_live(void*commands,u8*parser){
    NativeCommandMapNode*command_header;NativeCommandNameMapNode*name_header;
    if(commands!=built_commands||parser!=built_parser)return 0;
    command_header=*(NativeCommandMapNode**)(parser+SEAM_CommandParser_commandHeaderOffset);name_header=*(NativeCommandNameMapNode**)(parser+SEAM_CommandParser_commandNameMapHeaderOffset);
    return command_header&&name_header&&command_header->parent&&name_header->parent&&*(u32*)(parser+SEAM_CommandParser_commandCountOffset)==native_schema_root_count&&*(u32*)(parser+SEAM_CommandParser_commandNameMapCountOffset)==native_schema_map_entry_count;
}

static int registry_resolves(void*parser){
    NativeGstdString key;NativeSharedCommand result;u32 scratch=0,code;
    zero(&key,sizeof(key));zero(&result,sizeof(result));((StrCtor)SEAM_StrCtor)(&key,"help",&scratch);
    code=((GetCommandFn)SEAM_CommandParser_getCommand)(parser,&key,0,&result);
    if(result.control)((SharedAssignFn)0x008B1150u)(&result,0,0);
    ((StrDtor)SEAM_StrDtor)(&key);return (u8)code!=0;
}

int native_registry_is_attached(void*minecraft_commands){
    void*parser=minecraft_commands?*(void**)((u8*)minecraft_commands+8):0;
    return parser&&registry_live(minecraft_commands,(u8*)parser);
}

int native_registry_attach(void*minecraft_commands,void*game){
    u8*parser;NativeCommandMap*command_map;NativeCommandMapNode*command_header,*command_source_root,*command_root;NativeCommandNameMapNode*name_header,*name_source_root,*name_root;u32 name_count;
    native_command_trace('G');if(!minecraft_commands){native_command_trace('g');return 0;}parser=*(u8**)((u8*)minecraft_commands+8);if(!parser){native_command_trace('p');return 0;}
    s->minecraft_commands=minecraft_commands;s->command_parser=parser;s->command_game=game;if(registry_live(minecraft_commands,parser)){native_command_trace('I');return 1;}
    command_map=(NativeCommandMap*)(parser+SEAM_CommandParser_commandMapOffset);command_header=*(NativeCommandMapNode**)(parser+SEAM_CommandParser_commandHeaderOffset);
    name_header=*(NativeCommandNameMapNode**)(parser+SEAM_CommandParser_commandNameMapHeaderOffset);name_count=*(u32*)(parser+SEAM_CommandParser_commandNameMapCountOffset);
    if(!command_header||!name_header||!command_header->left||!command_header->right||!name_header->left||!name_header->right){native_command_trace('g');return 0;}
    if(command_map->count||name_count||command_header->parent||name_header->parent){native_command_trace('h');return 0;}
    if(!native_selector_registry_load()||!native_command_loader_load(parser)||!native_command_registration_register()||!build_source_nodes()){native_command_trace('i');return 0;}native_command_trace('H');
    command_source_root=build_overload_tree(0,(int)native_schema_root_count,command_header);name_source_root=build_name_tree(0,(int)native_schema_map_entry_count,name_header);
    command_root=((MapCloneFn)SEAM_CommandMap_cloneSubtree)(command_map,command_source_root,command_header);name_root=((NameMapCloneFn)SEAM_CommandNameMap_cloneSubtree)(parser+SEAM_CommandParser_commandNameMapAllocatorOffset,name_source_root,name_header);destroy_source_keys();release_source_storage();
    if(!command_root||!name_root){native_command_trace('j');return 0;}native_command_trace('J');
    command_header->parent=command_root;command_header->left=leftmost_overload(command_root);command_header->right=rightmost_overload(command_root);name_header->parent=name_root;name_header->left=leftmost_name(name_root);name_header->right=rightmost_name(name_root);
    command_map->count=native_schema_root_count;*(u32*)(parser+SEAM_CommandParser_commandCountOffset)=native_schema_root_count;*(u32*)(parser+SEAM_CommandParser_commandNameMapCountOffset)=native_schema_map_entry_count;
    if(!registry_resolves(parser)){native_command_trace('k');return 0;}
    built_parser=parser;built_commands=minecraft_commands;native_command_trace('T');return 1;
}
