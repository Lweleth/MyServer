#ifndef __MAPH__
#define __MAPH__ 

#include "s_string.h"
#define LL long long 
#define MAXRANGE (100000 + 10)

typedef struct node
{
	string_t key;
	void* val;
	struct node* next;
} map_node_t;

typedef struct hashmap_t{
	LL size;
	map_node_t* mir[MAXRANGE];
}map_t;


// BKDR Hash Function
extern LL BKDRHash(char *str, int len);
extern map_t* map_init();
extern map_node_t* map_node_new(string_t* key, void* val, map_node_t* next);
extern void map_clear(map_t* mp);
extern void map_insert(map_t* mp, const string_t* str, void* val);
extern void* map_find(map_t* mp, const string_t* str);



#endif