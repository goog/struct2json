#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

/** FALSE */
#ifndef FALSE
#define FALSE 0
#endif
/** TRUE */
#ifndef TRUE
#define TRUE (!FALSE)
#endif


#define MY_FREE(p)      \
    do                  \
    {                   \
        if(p != NULL)   \
        {               \
            free(p);    \
            p = NULL;   \
        }               \
    }                   \
    while(0)
    


typedef char *(*tostring)(const char *name, void *vaule, size_t len);

typedef enum
{
    INT_FIELD = 1,  // struct field is int
    FLOAT_FIELD,    // struct field is float
    STRING_FIELD,   // struct field is string pointer
    INT_ARRAY_FIELED,
    // above is standard field
    CUSTOM_BOOL_FIELED,  // cutom define a bool data field in your struct
    CUSTOM_FIELD1,
    CUSTOM_FIELD2,
    CUSTOM_FIELD3,
    MAX_FIELD
} field_type;


// one struct field convert rule, create by user according some target struct
typedef struct rule
{
    char *name;     // json name
    void *value;    // value pointer to data
    size_t v_len;   // length of value data
    field_type type;// value type like int or int array
    tostring func;  // the function to convert data to a json name-value string
} rule_t;


/** @brief convert one int value to one name-value of json  
 *  @param name, json name
 *  @param value, point to one struct data field
 *  @return len, the length of value data size
 *  @note string format is like "total": 56 
 **/
char *int2str(const char *name, void *value, size_t len)
{
    char tmp[128] = {0};
    snprintf(tmp, sizeof(tmp), "\"%s\":%d", name, *(int *)value);
    return strdup(tmp);
}


/** @brief convert one string value to one name-value of json  
 *  @param name, json name
 *  @param value, point to one string in struct
 *  @return len, the string length of value
 *  @note returned string format is like "name": "jobs" 
 **/
char *str2str(const char *name, void *value, size_t len)
{
    char tmp[512] = {0};
    snprintf(tmp, sizeof(tmp), "\"%s\":\"%s\"", name, (char *)value);
    return strdup(tmp);
}



char *bool2str(const char *name, void *value, size_t len)
{
    char tmp[128] = {0};
    const char *tmp_bool = NULL;
    if(*(char *)value == FALSE)
    {
        tmp_bool = "false";
    }
    else
    {
        tmp_bool = "true";
    }
    snprintf(tmp, sizeof(tmp), "\"%s\":%s", name, tmp_bool);
    return strdup(tmp);
}


/** @brief convert one struct field to one name-value of json  
 *  @param rule, one struct convert rule pointer
 *  @return string 
 *  @note string format is like "name": "jobs", the reture value need free
 **/
char *conv_rule_to_string(rule_t *rule)
{
    int type;
    char *name_string = NULL;
    
    if(rule == NULL)
    {
        return NULL;
    }

    type = rule->type;
    if(type >= MAX_FIELD)
    {
        return NULL;
    }
    
    
    switch(type)
    {
        case INT_FIELD:
        {
            name_string = int2str(rule->name, rule->value, rule->v_len);
            break;
        }
        case STRING_FIELD:
        {
            name_string = str2str(rule->name, rule->value, rule->v_len);
            break;
        }
        default:    // some complex data field will process here
        {
            if(rule->func == NULL)
            {
                fprintf(stderr, "rule func is null, you should set it\n");
                return NULL;
            }
            
            name_string = rule->func(rule->name, rule->value, rule->v_len);
            break;
        }
        

    }

    return name_string;
}


int get_json_string(rule_t field[], size_t size, char *buffer, size_t buf_size)
{
    
    if(field == NULL || buffer == NULL)
    {
        fprintf(stderr, "parameter is null\n");
        return -1;
    }

    if(size <= 0 || buf_size < 2)
    {
        return -1;
    }
    
    memset(buffer, 0, buf_size);
    *buffer = '{';

    int i;
    int len = 0, total = 1;
    for(i = 0; i < size; i++)
    {
        // convert each struct field by its own rule 
        char *output = conv_rule_to_string(&field[i]);
        
        len = strlen(output);
        if(total + len + 1 >= buf_size)
        {
            fprintf(stderr, "buffer too small cant strncat\n");
            
            MY_FREE(output);
            return -1;
        }

        strncat(buffer+total, output, len);
        MY_FREE(output);
        total += len;
        
        if(i != size - 1)
        {
            *(buffer+total) = ',';
            total++;
        }
        
    }

    *(buffer + total) = '}';  // add the ending braket

    return 0;
}

// in convert function you may use cjson function to do it
int main()
{

    struct test
    {
        int f1;
        int f2;
        char *name;
        char can_say_espanol;  // bool value
    };

    struct test t;
    t.f1 = 10;
    t.f2 = 20;
    t.name = "Gates";
    t.can_say_espanol = FALSE;

    // step 1 create rules for each struct field 
    rule_t field[] =
    {
        {.name = "f1", .type = INT_FIELD, .value = &(t.f1), .v_len = 4},
        {.name = "f2", .type = INT_FIELD, .value = &(t.f2), .v_len = 4},
        {.name = "name", .type = STRING_FIELD, .value = t.name, .v_len = strlen(t.name)},
        {.name = "can_espanol", .type = CUSTOM_BOOL_FIELED, .value = &t.can_say_espanol, 
         .v_len = sizeof(t.can_say_espanol), .func = bool2str},
    };

    
    int buf_size = 1024;
    char *buffer = malloc(buf_size);
    memset(buffer, 0, buf_size);
    int ret = get_json_string(field, ARRAY_SIZE(field), buffer, buf_size);
    if(ret == 0)
    {
        printf("struct is converted to json:\n%s\n", buffer);
    }

    free(buffer);
}
