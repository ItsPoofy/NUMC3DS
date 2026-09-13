#include "command_block_model.h"
#include "../../entity/minecart_command_block.h"
#include "../../internal.h"

typedef void (*UpdateBlockFn)(void*,void*,void*,void*,int,int,int,void*,int);

static void *model_source(const CommandBlockModel *model){
    return model->player?*(void**)((u8*)model->player+0x210):0;
}

static void *model_entity(const CommandBlockModel *model){
    void *source=model_source(model),*entity;
    if(!source)return 0;
    entity=((void*(*)(void*,const int*))SEAM_BlockSource_getBlockEntity)(source,model->position);
    return entity&&((int(*)(void*,int))0x00173EC8u)(entity,0x1a)?entity:0;
}

int command_block_can_use(void *player){
    return player&&((int(*)(void*))0x00727604u)(player);
}

const char *command_block_model_text(u32 field){return field?(const char*)field:"";}

void command_block_model_set_text(u32 *field,const char *text){
    u32 replacement=0,scratch=0;
    ((StrCtor)SEAM_StrCtor)(&replacement,text?text:"",&scratch);
    if(!replacement)return;
    if(*field)((StrDtor)SEAM_StrDtor)(field);
    *field=replacement;
}

void command_block_model_destroy(CommandBlockModel *model){
    if(model->command)((StrDtor)SEAM_StrDtor)(&model->command);
    if(model->name)((StrDtor)SEAM_StrDtor)(&model->name);
    if(model->output)((StrDtor)SEAM_StrDtor)(&model->output);
    zero(model,sizeof(*model));
}

int command_block_model_valid(const CommandBlockModel *model){
    void *source;
    if(!model||!model->player||!command_block_can_use(model->player))return 0;
    source=model_source(model);
    if(!source||*(void**)((u8*)source+0x0c)!=model->level)return 0;
    if(model->is_minecart){
        return model->entity!=0&&is_minecart_command_block(model->entity);
    }
    return model_entity(model)!=0;
}

int command_block_model_init(CommandBlockModel *model,void *player,const int position[3]){
    void *entity,*source;
    zero(model,sizeof(*model));
    model->player=player;
    cp(model->position,position,sizeof(model->position));
    source=model_source(model);
    if(!source||!command_block_can_use(player))return 0;
    model->level=*(void**)((u8*)source+0x0c);
    entity=model_entity(model);
    if(!entity)return 0;
    model->mode=(u8)((int(*)(void*,void*))0x006F4D08u)(entity,source);
    model->conditional=(u8)((int(*)(void*,void*))0x006F4B64u)(entity,source);
    model->redstone=*((u8*)entity+0x8a);
    model->track_output=*((u8*)entity+0x64);
    ((void(*)(void*,const void*))0x002FF261u)(&model->command,(u8*)entity+0x6c);
    ((void(*)(void*,const void*))0x002FF261u)(&model->name,(u8*)entity+0x70);
    ((void(*)(void*,const void*))0x002FF261u)(&model->output,(u8*)entity+0x68);
    return model->command&&model->name&&model->output;
}

int command_block_model_init_entity(CommandBlockModel *model,void *player,void *entity){
    void *source,*comp,*base;
    zero(model,sizeof(*model));
    if(!player||!entity||!command_block_can_use(player))return 0;
    model->player=player;
    model->entity=entity;
    model->is_minecart=1;
    source=model_source(model);
    if(!source)return 0;
    model->level=*(void**)((u8*)source+0x0c);
    comp=minecart_command_block_get_or_create_component(entity);
    if(!comp)return 0;
    base=(u8*)comp+8;
    model->track_output=*((u8*)base+4);
    ((void(*)(void*,const void*))0x002FF261u)(&model->output,(u8*)base+8);
    ((void(*)(void*,const void*))0x002FF261u)(&model->command,(u8*)base+12);
    ((void(*)(void*,const void*))0x002FF261u)(&model->name,(u8*)base+16);
    model->mode=0;
    model->conditional=0;
    model->redstone=1;
    return model->command&&model->name&&model->output;
}

int command_block_model_save(CommandBlockModel *model){
    void *entity,*source,*comp;
    if(!command_block_model_valid(model))return 0;
    if(model->is_minecart){
        comp=minecart_command_block_get_or_create_component(model->entity);
        if(!comp)return 0;
        typedef void (*OnCbUpdateFn)(void*,const void*,const void*,int,const void*);
        ((OnCbUpdateFn)0x004099B8u)(comp,&model->command,&model->name,model->track_output,&model->output);
        return 1;
    }
    if(model->mode>2)return 0;
    source=model_source(model);entity=model_entity(model);
    if(!entity)return 0;
    ((UpdateBlockFn)0x00172DC4u)(entity,source,&model->command,&model->name,
        model->mode,model->conditional,model->redstone,(u8*)entity+0x68,model->track_output);
    return 1;
}
