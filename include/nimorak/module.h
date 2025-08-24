#ifndef MODULE_H
#define MODULE_H

#include <stdlib.h>

#include <nimorak.h>

void module_init_list(ModuleList *list, int capacity);
void module_free_list(ModuleList *list);
void module_add(ModuleList *list, ModuleFunction function, void *arg, char *identifier);
void module_pop(ModuleList *list);
void module_del(ModuleList *list, char *identifier);
void module_run_list(ModuleList *list);

#endif