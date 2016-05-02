#define TRUE 1
#include "SortedList.h"
#include "string.h"

int SortedList_length(SortedList_t *list) {

        /* list head should not be corrupt */
        if(list->prev != NULL || list->key != NULL)
                return -1;

        int len = 0;
        SortedListElement_t *curr_node = list;
        SortedListElement_t *prev_node;

        /* iterate over all nodes, checking for corruption & incrementing len */
        while(curr_node->next != NULL)
        {
                len++;
                prev_node = curr_node;
                // Call yield in the middle of iterating over the nodes to really screw things
                if(opt_yield & SEARCH_YIELD)
                        pthread_yield();
                curr_node = curr_node->next;

                if(curr_node->prev != prev_node)
                        return -1;

                if(curr_node->key == NULL)
                        return -1;
        }

        return len;
}

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {

        SortedListElement_t *curr_node  = list;
        SortedListElement_t *prev_node = list;

        if(curr_node->next == NULL)
                goto EOL;
        else
                curr_node = curr_node->next;

        int bias;
        /** Iterate until EOL **/
        do {
                bias = strcmp(element->key, curr_node->key);
                /* curr_node > element, insert element to left of curr_node */
                if(bias < 0)
                {
                        prev_node->next = element;
                        element->prev = prev_node;
                        if(opt_yield & INSERT_YIELD)
                                pthread_yield();
                        element->next = curr_node;
                        curr_node->prev = element;
                        return;
                }

                /* curr_node <= element, iterate to next node or break if EOL */
                else if (curr_node->next != NULL)
                {
                        prev_node = curr_node;
                        curr_node = curr_node->next;
                }
                else
                        goto EOL;
        } while(TRUE);

        /* Control reaches here if we reach EOL w/o inserting the element */
        /* Insert element to EOL and set element->next = NULL */
EOL:
        curr_node->next = element;
        element->prev = curr_node;
        element->next = NULL;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *		call pthread_yield in middle of critical section
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {

        SortedListElement_t *curr_node = list;

        /* If list is empty, return NULL. Else move to first node */
        if(curr_node->next == NULL)
                goto EOL;
        else
                curr_node = curr_node->next;
        do {
                /* If key matches, return the current node */
                if(strcmp(key, curr_node->key) == 0)
                        return curr_node;

                /* Else move to the next node if possible */
                else if(curr_node->next != NULL) {
                        if(opt_yield & SEARCH_YIELD)
                                pthread_yield();
                        curr_node = curr_node->next;
                }

                /* Not possible to go to next node. Reached EOL, return NULL */
                else
                        goto EOL;
        } while(TRUE);

        /* Reached EOL, key not found. Return NULL */
EOL:
        return NULL;
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 * Note: if (opt_yield & DELETE_YIELD)
 *		call pthread_yield in middle of critical section
 */
int SortedList_delete( SortedListElement_t *element) {

        SortedListElement_t *next_node  = NULL;
        SortedListElement_t *prev_node  = NULL;

        /* Check to see if next_node->prev points to curr_node */
        if(element->next != NULL)
        {
                next_node = element->next;
                if(next_node->prev != element)
                        return 1;
        }

        /* Check to see if prev_node->next points to curr_node */
        if(element->prev != NULL)
        {
                prev_node = element->prev;
                if(prev_node->next != element)
                        return 1;
        }

        /* If we survived till here, it means that prev_node OR next_node
         * OR BOTH nodes are either curr_node or expected nodes. But they are
         * definitely not corrupted. Which means they are safe to unlink if
         * they exist.
         */
        if(prev_node != NULL)
                prev_node->next = next_node;

        if(opt_yield & DELETE_YIELD)
                pthread_yield();

        if(next_node != NULL)
                next_node->prev = prev_node;
        return 0;
}
