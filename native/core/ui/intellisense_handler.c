#include "intellisense_handler.h"
#include "ui_localize.h"
#include "ui_runtime.h"
#include "../internal.h"

enum { INTELLISENSE_DISPLAY_CAPACITY=192 };

typedef void *(*ItemInstanceItemCountAuxCtorFn)(void*,void*,int,int);

static char *duplicate(const char *text){unsigned length=text_len(text?text:"");char *copy=s->host.heap_alloc(length+1);if(!copy)return 0;cp(copy,text?text:"",length);copy[length]=0;return copy;}
static char *duplicate_prefix(const char *text,u32 length){char *copy=s->host.heap_alloc(length+1);if(!copy)return 0;cp(copy,text,length);copy[length]=0;return copy;}

static void *screen_level(void *game){void *clients,*client;if(!game)return 0;clients=*(void**)((u8*)game+0xC0);client=clients?*(void**)clients:0;return client?((GetLevel)SEAM_ClientInstance_getLevel)(client):0;}

static void origin_from_screen(void *screen,CommandParserIntellisenseOrigin *origin){
    void *game=ui_screen_game(screen);zero(origin,sizeof(*origin));if(!game)return;origin->player=((GetPlayer)SEAM_Player_getPlayer)(game);origin->level=screen_level(game);origin->permission=origin->player?((GetPermissionsLevel)SEAM_Player_getPermissionsLevel)(origin->player):0;
}

static int grow_intellisense(IntellisenseHandler *handler){
    unsigned count=handler->intellisense_begin?(unsigned)(handler->intellisense_end-handler->intellisense_begin):0,capacity=handler->intellisense_begin?(unsigned)(handler->intellisense_capacity-handler->intellisense_begin):0,next=capacity?capacity*2:8;char **items=s->host.heap_alloc(next*sizeof(*items));
    if(!items)return 0;
    zero(items,next*sizeof(*items));if(count)cp(items,handler->intellisense_begin,count*sizeof(*items));if(handler->intellisense_begin)s->host.heap_free(handler->intellisense_begin);handler->intellisense_begin=items;handler->intellisense_end=items+count;handler->intellisense_capacity=items+next;return 1;
}

static int add_intellisense(IntellisenseHandler *handler,const char *text){
    char *copy;if(handler->intellisense_end==handler->intellisense_capacity&&!grow_intellisense(handler))return 0;copy=duplicate(text);if(!copy)return 0;*handler->intellisense_end++=copy;return 1;
}

static int reserve_messages(IntellisenseHandler *handler,u32 required){
    u32 capacity=handler->messages_begin?(u32)(handler->messages_capacity-handler->messages_begin):0;IntellisenseAutoCompleteMessage *items;
    if(required<=capacity)return 1;
    if(handler->messages_begin)s->host.heap_free(handler->messages_begin);
    items=s->host.heap_alloc(required*sizeof(*items));
    if(!items){handler->messages_begin=handler->messages_end=handler->messages_capacity=0;return 0;}
    zero(items,required*sizeof(*items));handler->messages_begin=handler->messages_end=items;handler->messages_capacity=items+required;return 1;
}

static int parameter_is_item(const NativeSchemaParameter *parameter){const char *name=parameter?native_schema_text(parameter->enum_name):"";return streq(name,"itemType")||streq(name,"blockType");}

static void append_text(char *out,unsigned *length,const char *text){while(*text&&*length+1<INTELLISENSE_DISPLAY_CAPACITY)out[(*length)++]=*text++;out[*length]=0;}

static int add_message(IntellisenseHandler *handler,const CommandParserIntellisenseAutoCompleteOption *option,const NativeSchemaParameter *parameter){
    char display[INTELLISENSE_DISPLAY_CAPACITY],localized[INTELLISENSE_DISPLAY_CAPACITY];unsigned length=0;IntellisenseAutoCompleteMessage *message;
    display[0]=0;append_text(display,&length,option->text);
    if(option->description&&option->description[0]){ui_localized_label(localized,sizeof(localized),option->description,option->description);if(localized[0]){append_text(display,&length," - ");append_text(display,&length,localized);}}
    if(handler->messages_end==handler->messages_capacity)return 0;
    message=handler->messages_end;zero(message,sizeof(*message));message->text=duplicate(display);if(!message->text)return 0;message->match_start=option->match_start;message->match_length=option->match_length;if(parameter_is_item(parameter)){void *item=item_by_name(option->text);if(item){((ItemInstanceItemCountAuxCtorFn)SEAM_ItemInstance_itemCountAuxCtor)(message->item_instance,item,1,0);message->has_item=1;}}handler->messages_end++;return 1;
}

void intellisense_handler_init(IntellisenseHandler *handler){zero(handler,sizeof(*handler));handler->reset_tab_complete=1;handler->needs_layout_update=1;handler->auto_complete_grid_size=AUTOCOMPLETE_VISIBLE_LINES;}

void intellisense_handler_clear_messages(IntellisenseHandler *handler){
    char **text;IntellisenseAutoCompleteMessage *message;if(!handler)return;text=handler->intellisense_begin;while(text&&text!=handler->intellisense_end){if(*text)s->host.heap_free(*text);text++;}handler->intellisense_end=handler->intellisense_begin;message=handler->messages_begin;while(message&&message!=handler->messages_end){if(message->has_item)((ItemInstanceDtorFn)SEAM_ItemInstance_dtor)(message->item_instance);if(message->text)s->host.heap_free(message->text);message++;}handler->messages_end=handler->messages_begin;
}

void intellisense_handler_destroy(IntellisenseHandler *handler){
    if(!handler)return;
    intellisense_handler_clear_messages(handler);if(handler->intellisense_begin)s->host.heap_free(handler->intellisense_begin);if(handler->messages_begin)s->host.heap_free(handler->messages_begin);if(handler->tab_complete_input)s->host.heap_free(handler->tab_complete_input);intellisense_handler_init(handler);
}

void intellisense_handler_reset_tab_complete_progress(IntellisenseHandler *handler){if(!handler)return;handler->last_tab_complete_index=0;handler->reset_tab_complete=1;if(handler->tab_complete_input){s->host.heap_free(handler->tab_complete_input);handler->tab_complete_input=0;}}

void intellisense_handler_update(IntellisenseHandler *handler,void *screen,const char *input,u32 cursor){
    CommandParserIntellisenseOrigin origin;CommandParserIntellisenseInformationResult information;CommandParserIntellisenseAutoCompleteInformation options;CommandParserIntellisenseInformation *info;CommandParserIntellisenseAutoCompleteOption *option;
    if(!handler)return;
    intellisense_handler_clear_messages(handler);origin_from_screen(screen,&origin);command_parser_get_intellisense_information(&origin,input,cursor,&information);command_parser_get_auto_complete_options(&origin,input,cursor,&options);
    info=information.begin;while(info&&info!=information.end){option=options.begin;while(option&&option!=options.end&&!streq(option->text,info->text))option++;if((!option||option==options.end)&&!add_intellisense(handler,info->text))break;info++;}
    if(reserve_messages(handler,options.begin?(u32)(options.end-options.begin):0)){option=options.begin;while(option&&option!=options.end){if(!add_message(handler,option,options.parameter))break;option++;}}
    command_parser_intellisense_information_destroy(&information);command_parser_auto_complete_information_destroy(&options);handler->needs_layout_update=1;
}

static int replace_input(char *input,u32 *length,u32 *cursor,u32 limit,const char *base,const CommandParserIntellisenseAutoCompleteOption *option){
    u32 base_length=text_len(base),replacement_length=text_len(option->replacement),start=option->replacement_start,total;
    if(start>base_length)start=base_length;
    total=start+replacement_length;if(total>limit)return 0;cp(input,base,start);cp(input+start,option->replacement,replacement_length);input[total]=0;*length=total;*cursor=total;return 1;
}

int intellisense_handler_handle_tab_complete(IntellisenseHandler *handler,void *screen,char *input,u32 *length,u32 *cursor,u32 limit){
    CommandParserIntellisenseOrigin origin;CommandParserIntellisenseAutoCompleteInformation options;CommandParserIntellisenseAutoCompleteOption *option;u32 count,index,saved_index;int result=0;
    if(!handler||!input||!length||!cursor)return 0;
    if(handler->reset_tab_complete){u32 input_length=text_len(input),prefix_length=*cursor<input_length?*cursor:input_length;if(handler->tab_complete_input)s->host.heap_free(handler->tab_complete_input);handler->tab_complete_input=duplicate_prefix(input,prefix_length);handler->last_tab_complete_index=0;handler->reset_tab_complete=0;}if(!handler->tab_complete_input)return 0;
    origin_from_screen(screen,&origin);command_parser_get_auto_complete_options(&origin,handler->tab_complete_input,text_len(handler->tab_complete_input),&options);count=options.begin?(u32)(options.end-options.begin):0;if(count){index=handler->last_tab_complete_index%count;option=&options.begin[index];result=replace_input(input,length,cursor,limit,handler->tab_complete_input,option);if(result)handler->last_tab_complete_index++;}
    command_parser_auto_complete_information_destroy(&options);if(result){saved_index=handler->last_tab_complete_index;intellisense_handler_update(handler,screen,input,*cursor);handler->last_tab_complete_index=saved_index;handler->reset_tab_complete=0;}return result;
}

static u32 information_count(const IntellisenseHandler *handler){return handler->intellisense_begin?(u32)(handler->intellisense_end-handler->intellisense_begin):0;}
static u32 message_count(const IntellisenseHandler *handler){return handler->messages_begin?(u32)(handler->messages_end-handler->messages_begin):0;}
static u32 auto_complete_space(const IntellisenseHandler *handler){u32 info=information_count(handler),messages=message_count(handler),needed=info+1;if(messages+needed<=handler->auto_complete_grid_size)return messages;if(handler->auto_complete_grid_size<=needed)return 0;return handler->auto_complete_grid_size-needed;}

u32 intellisense_handler_get_auto_complete_grid_size(const IntellisenseHandler *handler){return handler?handler->auto_complete_grid_size:0;}
void intellisense_handler_set_auto_complete_grid_size(IntellisenseHandler *handler,u32 size){if(handler)handler->auto_complete_grid_size=size;}
u32 intellisense_handler_get_last_tab_complete_index(const IntellisenseHandler *handler){return handler?handler->last_tab_complete_index:0;}
int intellisense_handler_get_needs_layout_update(const IntellisenseHandler *handler){return handler?handler->needs_layout_update:0;}
void intellisense_handler_set_needs_layout_update(IntellisenseHandler *handler,int value){if(handler)handler->needs_layout_update=(u8)(value!=0);}

u32 intellisense_handler_get_display_count(const IntellisenseHandler *handler){
    u32 info,messages,space,count;if(!handler)return 0;info=information_count(handler);messages=message_count(handler);space=auto_complete_space(handler);count=info;if(messages)count+=space+1;if(count>handler->auto_complete_grid_size)count=handler->auto_complete_grid_size;return count;
}

const char *intellisense_handler_get_auto_complete_text(const IntellisenseHandler *handler,u32 index){
    u32 messages=message_count(handler),space=auto_complete_space(handler),info=information_count(handler);if(!handler)return "";
    if(messages){if(index<space){if(messages>space&&index+1==space)return "...";return handler->messages_begin[index].text;}if(index==space)return "";index-=space+1;}
    return index<info?handler->intellisense_begin[index]:"";
}

const void *intellisense_handler_get_auto_complete_item(const IntellisenseHandler *handler,u32 index){u32 messages,space;if(!handler)return 0;messages=message_count(handler);space=auto_complete_space(handler);if(index>=space||(messages>space&&index+1==space)||!handler->messages_begin[index].has_item)return 0;return handler->messages_begin[index].item_instance;}
u32 intellisense_handler_get_auto_complete_match_start(const IntellisenseHandler *handler,u32 index){u32 messages,space;if(!handler)return 0;messages=message_count(handler);space=auto_complete_space(handler);if(index>=space||(messages>space&&index+1==space))return 0;return handler->messages_begin[index].match_start;}
u32 intellisense_handler_get_auto_complete_match_length(const IntellisenseHandler *handler,u32 index){u32 messages,space;if(!handler)return 0;messages=message_count(handler);space=auto_complete_space(handler);if(index>=space||(messages>space&&index+1==space))return 0;return handler->messages_begin[index].match_length;}
