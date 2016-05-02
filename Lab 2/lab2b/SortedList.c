#define TRUE 1
#include "SortedList.h"
#include "string.h"


int SortedList_length(SortedList_t *list) {

        /* list head should not be corrupt */
        if(list->prev != NULL || list->key != NULL)
                return -1;

        int len = 0;
        SortedListElement_t *cur_node = list;
        SortedListElement_t *prev_node;

        /* iterate over all nodes, checking for corruption & incrementing len */
        while(cur_node->next != NULL)
        {
                len++;
                prev_node = cur_node;
                cur_node = cur_node->next;

                if(cur_node->prev != prev_node)
                        return -1;

                if(cur_node->key == NULL)
                        return -1;
        }

        return len;
}

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {

        SortedListElement_t *cur_node  = list;
        SortedListElement_t *prev_node = list;

        if(cur_node->next == NULL)
                goto EOL;
        else
                cur_node = cur_node->next;

        int bias;
        /** Iterate until EOL **/
        do {
                bias = strcmp(element->key, cur_node->key);
                /* cur_node > element, insert element to left of cur_node */
                if(bias < 0)
                {
                        prev_node->next = element;
                        element->prev = prev_node;
                        element->next = cur_node;
                        cur_node->prev = element;
                        return;
                }

                /* cur_node <= element, iterate to next node or break if EOL */
                else if (cur_node->next != NULL)
                {
                        prev_node = cur_node;
                        cur_node = cur_node->next;
                }
                else
                        goto EOL;
        } while(TRUE);

        /* Control reaches here if we reach EOL w/o inserting the element */
        /* Insert element to EOL and set element->next = NULL */
EOL:
        cur_node->next = element;
        element->prev = cur_node;
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

        SortedListElement_t *cur_node = list;

        /* If list is empty, return NULL. Else move to first node */
        if(cur_node->next == NULL)
                goto EOL;
        else
                cur_node = cur_node->next;
        do {
                /* If key matches, return the current node */
                if(strcmp(key, cur_node->key) == 0)
                        return cur_node;

                /* Else move to the next node if possible */
                else if(cur_node->next != NULL)
                        cur_node = cur_node->next;

                /* Not possible to go to next node. Reached EOL, return NULL */
                else
                        goto EOL;
        } while(TRUE);

        /* Reached EOL, key not found. Return NULL */
EOL:
        return NULL;
}
