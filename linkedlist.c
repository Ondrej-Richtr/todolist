#include "todolist.h"

//linkedlist definitions
void node_destroy(struct node *n)
{
	if (n == NULL) return;
	
	free(n->val);
	free(n);
}

size_t llist_length(llist *list)
{
	size_t length = 0;
	struct node *n = list->first;
	
	while (n)
	{
		length++;
		n = n->next;
	}
	
	return length;
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

void llist_delete_after(llist* list, struct node *prev)
{	//deletes node after given 'prev' node, if NULL then it deletes the first node
	struct node *deleted = NULL;
	if (!prev) deleted = llist_pop_node_first(list);
	else deleted = prev->next;
	
	if (!deleted) return; //nothing to be done
	
	//fixes correctness for list->last (list->first should be correct from pop)
	if (deleted == list->last) list->last = prev;
	
	if (prev) prev->next = deleted->next;
	node_destroy(deleted);
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

int llist_delete_nth_entry(llist *list, size_t n)
{	//deletes nth entry in linked list, counting from 0
	//returns 0 if entry deleted, 1 if 'n' out of bounds, -1 if wrong input
	if (!list) return -1;
	
	struct node *prev = NULL;
	
	if (n)
	{
		if (!(prev = llist_nth_node(list, n - 1)) || !prev->next) return 1;
	}
	
	llist_delete_after(list, prev);
	
	return 0;
}

void llist_clear(llist *list, int status)
{	//deletes all entries from linked list with given status
	struct node *n = list->first, *next = NULL, *prev = NULL;
	
	while (n != NULL)
	{
		next = n->next;
		
		if (n->val->status == status) llist_delete_after(list, prev);
		else prev = n;
		
		n = next;
	}
}

int llist_disconnect(llist *list, llist *into, size_t start, size_t end)
{	//indexing is from 0, disconnects specified section from 'list' and connects it into
	//EMPTY linked list given by 'into', returns nonzero when error
	if (!list || !into || into->first || into->last) return -1;
	if (start > end) return 1;
	
	struct node *start_node_prev = NULL, *end_node = NULL, *start_node = NULL;
	size_t length = llist_length(list);
	if (end >= length) return 2;
	
	//setting up start-node_prev and end_node (not touching the lists)
	if (!start) start_node_prev = NULL;
	else start_node_prev = llist_nth_node(list, start - 1);
	
	if (length == end + 1) end_node = list->last;
	else end_node = llist_nth_node(list, end);
	if (!end_node)
	{
		//TODO err?
		return 3;
	}
	
	//touching the lists
	if (start_node_prev == NULL)
	{
		start_node = list->first;
		list->first = end_node->next;
	}
	else
	{
		start_node = start_node_prev->next;
		start_node_prev->next = end_node->next;
	}
	
	if (end_node == list->last)
	{
		list->last = start_node_prev;
	}
	end_node->next = NULL;
	into->first = start_node;
	into->last = end_node;
	
	return 0;
}

int llist_move(llist *list, size_t from, size_t to, size_t where)
{	//indexing is from 0, moves range defined by 'from':'to' of nodes
	//into place starting at index 'where' + 1 (for now)
	
	//WORK IN PROGRESS
	if (!list || from > to) return -1;

	if (where >= from && where <= to)
	{
		//inside of the moved list is for now disabled
		return 1;
	}
	
	struct node *where_node_prev = NULL;
	if (where)
	{
		if (where - 1 == to) return 0; //this means that we dont have to do anyhting
		where_node_prev = llist_nth_node(list, where - 1);	
		if (!where_node_prev)
		{
			//TODO err	
			return 2;
		}
	}
	
	size_t moved_start = from, moved_end = to;
	llist moved = { NULL, NULL };
	
	int diserr = llist_disconnect(list, &moved, moved_start, moved_end);
	if (diserr)
	{
		//TODO err
		return 3;
	}
	//moved now shouldnt contain NULL pointers, right?
	//as we always select nonempty section of linkedlist
	if (!where_node_prev)
	{
		moved.last->next = list->first;
		list->first = moved.first;
	}
	else
	{
		moved.last->next = where_node_prev->next;
		where_node_prev->next = moved.first;
	}
	
	if (list->last == where_node_prev) list->last = moved.last;
	
	return 0;
}
