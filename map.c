#include "map.h"

LL BKDRHash(char *str, int len)
{
    LL seed = 131LL; // 31 131 1313 13131 131313 etc..
    LL hash = 0LL;

    for(int i = 0; i < len && *str != '\0'; i++)
        hash = hash * seed + *(str + i);
    return (hash & 0x185CB);//%99787
}

map_t* map_init()
{
	map_t* mp = malloc(sizeof(map_t));
	memset(mp, 0, sizeof(map_t));
	return mp;
}

map_node_t* map_node_new(string_t* key, void* val, map_node_t* next)
{
	map_node_t* nd = malloc(sizeof(map_t));
	nd->key = *key;
	nd->val = val;
	nd->next = next;
	return nd;
}

void map_insert(map_t* mp, const string_t* str, void* val)
{
	LL hash = BKDRHash(str->data, str->len);
	map_node_t* nxt = NULL;
	if(mp->mir[hash] != NULL)
		nxt = mp->mir[hash]->next;
	mp->mir[hash] = map_node_new(str, val, nxt);
	mp->size++;
}

extern void map_clear(map_t* mp)
{
	if(mp == NULL)
		return ;
	for (int i = 0; i < MAXRANGE && mp->size > 0; i++)
	{
		if(mp->mir[i] == NULL)
			continue;
		map_node_t* nw = mp->mir[i];
		map_node_t* tmp;
		while(nw != NULL)
		{	
			tmp = nw->next;
			free(nw);
			nw = tmp;
			mp->size -= 1;
		}
	}
}
void* map_find(map_t* mp, const string_t* str)
{
	LL hash = BKDRHash(str->data, str->len);
	map_node_t* nw = mp->mir[hash];
	void* val;
	val = -1;
	while(nw != NULL)
	{
		if(string_cmp(&(nw->key), str) == 0)
		{
			val = nw->val;
			break;
		}
		nw = nw->next;
	}
	return val;
}	
/*
int main()
{
	char a[1000];
	printf("%d\n", 0x185CB);
	map_t* mp = map_init();
	int s = 54;
	printf("54 addr:%p\n", &s);
	scanf("%s", a);
		printf("%d\n", strlen(a));
		a[strlen(a)] = '\0';
		printf("%lld: %lld\n", BKDRHash(a,1), BKDRHash(a,1) % 99787);
		string_t str;
		printf("~\n");
		string_set(&str, a);
		printf("~~\n");
		map_insert(mp, &str, &s);	
		printf("~~~\n");
	int *y = (map_find(mp, &SSTRING("abc")));
	printf("%p", y);
	printf("~~~~~~~~\n");

	map_clear(mp);
}*/