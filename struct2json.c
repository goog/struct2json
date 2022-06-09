#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#define RET_OK 0
#define RET_FAIL -1
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define MAX_STRING_BUF_SIZE 256


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
    


//typedef char *(*tostring)(const char *name, void *vaule, size_t len);
typedef int (*tostring)(const char *name, void *vaule, size_t len,
                           char *buffer, size_t size);


typedef enum
{
    INT_FIELD = 1,  // struct field is int
    FLOAT_FIELD,    // struct field is float
    STRING_FIELD,   // struct field is string pointer
    BOOL_FIELD,     // struct field is bool
    CHAR_ARRAY_FIELD,
    INT_ARRAY_FIELD,
    DOUBLE_ARRAY_FIELD,
    // above is standard field
    CUSTOM_FIELD1,
    CUSTOM_FIELD2,
    CUSTOM_FIELD3,
    MAX_FIELD
} field_type_t;


// one struct field convert rule, create by user according some target struct
typedef struct rule
{
    char *name;     // json name
    void *value;    // value pointer to data
    size_t v_len;   // length of value data
    field_type_t type;// value type like int or int array
    tostring func;  // the function to convert data to a json name-value string
} rule_t;


/** @brief convert one int value to one name-value of json  
 *  @param name, json name
 *  @param value, point to one struct data field
 *  @param len, the length of value data size
 *  @param buffer, string format is like "total": 56  
 *  @param size, the size of buffer
 *  @return 0 or -1
 * - 0 success
 * - -1 fail
 **/
int int2str(const char *name, void *value, size_t len, char *buffer, size_t size)
{
    int n = 0;
    
    n = snprintf(buffer, size, "\"%s\":%d", name, *(int *)value);
    if(n > 0 && n < size)
        return 0;
    else
        return -1;
}


/** @brief convert one string value to one name-value of json  
 *  @param name, json name
 *  @param value, point to one string in struct
 *  @param len, the value data length
 *  @param buffer, returned string format is like "name": "jobs" 
 *  @param size, the size of buffer
 *  @return
 * - 0 success
 * - -1 fail
 **/
int str2str(const char *name, void *value, size_t len, char *buffer, size_t size)
{
    int n = 0;
    n = snprintf(buffer, size, "\"%s\":\"%s\"", name, (char *)value);
    if(n > 0 && n < size)
        return 0;
    else
        return -1;
}


int bool2str(const char *name, void *value, size_t len, char *buffer, size_t size)
{
    int n = 0;
    const char *tmp_bool = NULL;
    if(*(bool *)value == false)
    {
        tmp_bool = "false";
    }
    else
    {
        tmp_bool = "true";
    }
    n = snprintf(buffer, size, "\"%s\":%s", name, tmp_bool);
    if(n > 0 && n < size)
        return 0;
    else
        return -1;
}



int int_array_to_str(const char *name, void *value, size_t len, char *buffer, size_t size)
{
    int n = 0;
    int i;
    int avail_size = 0;
    char tmp[128] = {0};
    int tmp_size = sizeof(tmp);
    int index = 0;
    int cnt = len/sizeof(int);
    tmp[0] = '[';
    index = 1;
    
    int *p = value;  // point to int array
    for(i = 0; i < cnt; i++)
    {
        avail_size = tmp_size - index;
        if(avail_size <= 0)
        {
            return -1;
        }
        
        if(i == cnt - 1)
        {
            n = snprintf(&tmp[index], avail_size, "%d", *(p+i));    
        }
        else
        {
            n = snprintf(&tmp[index], avail_size, "%d,", *(p+i));
        }

        
        if(n > 0 && n < avail_size)
        {
            index += n;
        }
        else
        {
            fprintf(stderr, "buffer size is small at %s\n", __FUNCTION__);
            return -1;
        }
        
    }

    if(index < tmp_size - 1)
        tmp[index] = ']';
    else
        return -1;
    
    n = snprintf(buffer, size, "\"%s\":%s", name, tmp);
    if(n > 0 && n < size)
        return 0;
    else
        return -1;
}


/** @brief convert one struct field to one name-value of json  
 *  @param rule, one struct convert rule pointer
 *  @param buffer, string format is like "name": "jobs", the reture value need free
 *  @param size, the size of buffer
 *  @return string 
 *  @return
 * - 0 success
 * - -1 fail
 **/
int conv_rule_to_string(rule_t *rule, char *buffer, size_t size)
{
    int type;
    int ret = -1;
    
    if(rule == NULL || buffer == NULL)
    {
        return -1;
    }

    type = rule->type;
    if(type >= MAX_FIELD)
    {
        return -1;
    }

    if(rule->name == NULL || rule->value == NULL)
    {
        return -1;
    }
    
    
    switch(type)
    {
        case INT_FIELD:
        {
            ret = int2str(rule->name, rule->value, rule->v_len, buffer, size);
            break;
        }
        case STRING_FIELD:
        {
            ret = str2str(rule->name, rule->value, rule->v_len, buffer, size);
            break;
        }
        case BOOL_FIELD:
        {
            ret = bool2str(rule->name, rule->value, rule->v_len, buffer, size);
            break;
        }
        case INT_ARRAY_FIELD:
        {
            ret = int_array_to_str(rule->name, rule->value, rule->v_len, buffer, size);
            break;
        }
        default:    // some complex data field will process here
        {
            if(rule->func == NULL)
            {
                fprintf(stderr, "rule func is null, you should set it\n");
                return -1;
            }
            
            ret = rule->func(rule->name, rule->value, rule->v_len, buffer, size);
            break;
        }
        

    }

    return ret;
}




// the popose is to get a plat json string with simple format like {"name": value}
int get_json_string(rule_t field[], size_t size, char *buffer, size_t buf_size)
{
    int ret = 0;
    
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
    char tmp_buffer[MAX_STRING_BUF_SIZE] = {0};
    for(i = 0; i < size; i++)
    {
        // convert each struct field by its own rule
        memset(tmp_buffer, 0, MAX_STRING_BUF_SIZE);
        ret = conv_rule_to_string(&field[i], tmp_buffer, MAX_STRING_BUF_SIZE);
        if(ret != 0)
        {
            fprintf(stderr, "conv_rule_to_string err\n");
            continue;
        }
        
        len = strlen(tmp_buffer);
        if(total + len + 1 >= buf_size)
        {
            fprintf(stderr, "buffer too small cant strncat\n");
            return -1;
        }

        strncat(buffer+total, tmp_buffer, len);
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
        int list[10];
        bool can_say_espanol;  // bool value
    };

    struct test t;
    t.f1 = 10;
    t.f2 = 20;
    t.name = "Gates";
    t.can_say_espanol = false;
    t.list[0] = 100;
    t.list[3] = 60;
    
    // step 1 create rules for each struct field
    // how to get struct meta info
    // maybe value will be union
    rule_t field[] =
    {
        {.name = "f1", .type = INT_FIELD, .value = &(t.f1), .v_len = 4},
        {.name = "f2", .type = INT_FIELD, .value = &(t.f2), .v_len = 4},
        {.name = "name", .type = STRING_FIELD, .value = t.name, .v_len = strlen(t.name)},
        {.name = "list", .type = INT_ARRAY_FIELD, .value = t.list, .v_len = sizeof(t.list)},
        {.name = "can_espanol", .type = BOOL_FIELD, .value = &t.can_say_espanol},
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
