#include <nimorak/module.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void module_init_list(ModuleList *list, int capacity)
{
    list->size = 0;
    list->capacity = capacity;
    list->items = malloc(list->capacity * sizeof(ModuleCall));
    
    if (!list->items) {
        perror("malloc");
        exit(1);
    }
}

void module_free_list(ModuleList *list)
{
    for (size_t i = 0; i < list->size; i++)
    {
        free(list->items[i].identifier);
    }

    free(list->items);

    list->items = NULL;
    list->size = list->capacity = 0;
}

void module_add(ModuleList *list, ModuleFunction function, void *arg, char *identifier)
{
    if (list->size == list->capacity) {
        list->capacity *= 2;

        ModuleCall *tmp = realloc(list->items, list->capacity * sizeof(ModuleCall));
        
        if (!tmp) {
            perror("realloc");
            exit(1);
        }
        
        list->items = tmp;
    }
    
    list->items[list->size].function = function;
    list->items[list->size].arg = arg;
    list->items[list->size].identifier = strdup(identifier);
    list->size++;
}

void module_pop(ModuleList *list)
{
    if (list->size > 0) {
        free(list->items[list->size - 1].identifier);
        list->size--;
    }
}

void module_del(ModuleList *list, char *identifier)
{
    for (size_t i = 0; i < list->size; i++) {
        if (strcmp(list->items[i].identifier, identifier) == 0) {
            free(list->items[i].identifier);

            // shift remaining elements left
            for (size_t j = i; j < list->size - 1; j++) {
                list->items[j] = list->items[j + 1];
            }

            list->size--;
            return; // stop after first match
        }
    }
}

void module_run_list(ModuleList *list)
{
    for (size_t i = 0; i < list->size; i++) {
        list->items[i].function(list->items[i].arg);
    }
}