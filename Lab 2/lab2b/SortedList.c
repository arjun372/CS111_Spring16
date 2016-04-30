
#include "SortedList.h"

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 *
 * Note: if (opt_yield & SEARCH_YIELD)
 *		call pthread_yield in middle of critical section
 */

/** @param SortedList_t *list ... header for the list
  *
  *   @return -1 if the list is corrupted
  *   @return  0 if no-elements in list (except head)
  *   @return  N # elements in list (except head)
  *
  *   if (opt_yield & SEARCH_YIELD) ::
  *                             call pthread_yield in middle of critical section
**/

int SortedList_length(SortedList_t *list) {
  int count = -1;
  return count;
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

}
