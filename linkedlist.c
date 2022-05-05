#include "todolist.h"

//linkedlist definitions
void node_destroy(struct node *n)
{
	if (n == NULL) return;
	
	free(n->val);
	free(n);
}

void llist_add_node_end(llist *list, struct node *n)
{
	if (list->last == NULL) list->first = n;
	else list->last->next = n;
	
	list->last = n;
	n->next = NULL;
}

void llist_add_node_first(llist *list, struct node *n)
{
	n->next = list->first;
	
	if (list->last == NULL) list->last = n;
	list->first = n;
}

struct node* llist_pop_node_first(llist *list)
{
	if (list->first == NULL) return NULL;
	
	struct node *out = list->first;
	if (list->last == out) list->last = NULL;
	
	list->first = out->next;
	
	return out;
}

int llist_add_end(llist *list, todo_entry_t *val)
{
	if (list == NULL) return 0;
	
	struct node *n = malloc(sizeof(struct node));
	
	if (n == NULL) return 0;
	
	n->next = NULL;
	n->val = val;
	llist_add_node_end(list, n);
	
	return 1;
}

int llist_add_first(llist *list, todo_entry_t *val)
{
	if (list == NULL) return 0;
	
	struct node *n = malloc(sizeof(struct node));
	
	if (n == NULL) return 0;
	
	n->next = NULL;
	n->val = val;
	llist_add_node_first(list, n);
	
	return 1;
}

void llist_destroy_contents(llist *list)	//destroys contents deeply
{
	struct node *n = NULL;
	
	while ((n = llist_pop_node_first(list)) != NULL)
	{
		node_destroy(n);
	}
}

//functions for CLI
struct node *llist_nth_node(llist *list, size_t n)
{	//returns pointer to nth node in linked list, indexing from zero
	//if there is no such node then it returns NULL
	if (!list) return NULL;
	
	struct node *current = list->first;
	
	while (n && current) //n is not zero and current is not NULL
	{
		n--;
		current = current->next;
	}
	
	return current;
}

todo_entry_t *llist_nth_entry(llist *list, size_t n)
{	//returns pointer to nth entry linked list, indexing from zero
	//if there is no such entry then it returns NULL
	struct node *entry_node = llist_nth_node(list, n);
	if (entry_node) return entry_node->val;
	return NULL;
}
