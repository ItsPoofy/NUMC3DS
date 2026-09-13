#include "command_parameter_match.h"
#include "command_enum_registry.h"

typedef struct {const char *target;int found;} EnumMatchContext;

static char fold_char(char value){return value>='A'&&value<='Z'?(char)(value+'a'-'A'):value;}

static int equal(const char *left,const char *right){if(!left)left="";if(!right)right="";while(*left&&*left==*right){left++;right++;}return *left==*right;}

static int equal_fold(const char *left,const char *right){if(!left)left="";if(!right)right="";while(*left&&fold_char(*left)==fold_char(*right)){left++;right++;}return !*left&&!*right;}

static int number(const char *text,int relative,int fractional){
    unsigned digits=0;if(!text||!*text)return 0;if(relative&&*text=='~'){text++;if(!*text)return 1;}if(*text=='+'||*text=='-')text++;
    while(*text>='0'&&*text<='9'){text++;digits++;}if(fractional&&*text=='.'){text++;while(*text>='0'&&*text<='9'){text++;digits++;}}return digits&&!*text;
}

static int target(const McpeToken *token){
    const char *text=token->text;if(!text||!*text)return 0;if(token->is_selector){if(*text!='@')return 0;text++;if(*text!='a'&&*text!='e'&&*text!='p'&&*text!='r')return 0;text++;if(!*text)return 1;if(*text!='[')return 0;while(*text)text++;return text[-1]==']';}
    if(token->is_quoted)return 1;
    while(*text){char value=*text++;if(value==' '||value=='\t'||value=='\r'||value=='\n'||value=='@'||value=='['||value==']'||value==','||value=='{'||value=='}')return 0;}return 1;
}

static int identifier(const char *text){
    if(!text||!*text)return 0;
    while(*text){char value=*text++;if(value>='a'&&value<='z')continue;if(value>='A'&&value<='Z')continue;if(value>='0'&&value<='9')continue;if(value=='_'||value==':'||value=='.'||value=='-')continue;return 0;}return 1;
}

static void enum_match(const char *value,void *opaque){EnumMatchContext *context=(EnumMatchContext*)opaque;if(!context->found&&equal_fold(value,context->target))context->found=1;}

static int enum_contains(const NativeSchemaParameter *parameter,const char *text){EnumMatchContext context={text,0};command_enum_visit(parameter,enum_match,&context);return context.found;}

int command_parameter_matches(const NativeSchemaParameter *parameter,const McpeToken *token){
    const char *text;if(!parameter||!token)return 0;text=token->text;
    switch(parameter->type){
    case NATIVE_PARAM_FLOAT:return number(text,1,1);
    case NATIVE_PARAM_INT:return number(text,0,0);
    case NATIVE_PARAM_STRING_ENUM:if(command_enum_available(parameter))return enum_contains(parameter,text);if(number(text,0,0))return 0;return identifier(text);
    case NATIVE_PARAM_BOOL:return equal_fold(text,"true")||equal_fold(text,"false")||equal(text,"1")||equal(text,"0");
    case NATIVE_PARAM_TARGET:return target(token);
    case NATIVE_PARAM_BLOCKPOS:
    case NATIVE_PARAM_ROTATION:
    case NATIVE_PARAM_VEC3:return number(text,1,1);
    case NATIVE_PARAM_COMPONENTS:
    case NATIVE_PARAM_STRING:
    case NATIVE_PARAM_RAWTEXT:
    default:return text&&*text;
    }
}
