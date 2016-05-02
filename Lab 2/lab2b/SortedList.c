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

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 *
 * Note: if (opt_yield & INSERT_YIELD)
 *		call pthread_yield in middle of critical section
 */
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
