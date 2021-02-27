#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
queue_t *q_new()
{
    queue_t *q = malloc(sizeof(queue_t));

    /* Return NULL if malloc fail */
    if (!q)
        return NULL;

    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
    list_ele_t *node, *pre_node;

    if (!q)
        return;

    /* Free each element and node */
    pre_node = q->head;
    while (pre_node != NULL) {
        node = pre_node->next;
        free(pre_node->value);
        free(pre_node);
        pre_node = node;
    }

    /* Free queue structure */
    free(q);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
    list_ele_t *newh;
    char *news;

    /* Return false is q is NULL */
    if (!q)
        return false;

    /* Allocate space for new node */
    newh = malloc(sizeof(list_ele_t));
    if (!newh)
        return false;

    /* Allocate space for string */
    news = malloc(strlen(s) + 1);
    if (!news) {
        free(newh);
        return false;
    }

    /* Copy string */
    memcpy(news, s, strlen(s) + 1);
    newh->value = news;

    /* Attach new head */
    newh->next = q->head;
    q->head = newh;

    if (q->size == 0)
        q->tail = q->head;

    q->size += 1;

    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    list_ele_t *newt;
    char *news;

    /* Return false is q is NULL */
    if (!q)
        return false;

    /* Allocate space for new node */
    newt = malloc(sizeof(list_ele_t));
    if (!newt)
        return false;

    /* Allocate space for string */
    news = malloc(strlen(s) + 1);
    if (!news) {
        free(newt);
        return false;
    }

    /* Copy string */
    memcpy(news, s, strlen(s) + 1);
    newt->value = news;
    newt->next = NULL;

    if (q->size > 0)
        q->tail->next = newt;
    else
        q->head = newt;

    q->tail = newt;
    q->size += 1;

    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return true if successful.
 * Return false if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 * The space used by the list element and the string should be freed.
 */
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    list_ele_t *rmh;

    if (!q || !q->head)
        return false;

    memcpy(sp, q->head->value, bufsize);
    rmh = q->head;
    q->head = q->head->next;
    free(rmh->value);
    free(rmh);
    q->size -= 1;

    return true;
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    return q->size;
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    list_ele_t *pre_node, *next_node, *current;

    if (!q || !q->head || q->size == 1)
        return;

    pre_node = q->head;
    current = q->head->next;
    next_node = current->next;
    current->next = pre_node;
    q->head->next = NULL;
    q->tail = q->head;

    while (current != NULL) {
        pre_node = current;
        current = next_node;
        if (current) {
            next_node = current->next;
            current->next = pre_node;
        }
    }
    q->head = pre_node;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
list_ele_t *merge(list_ele_t *l1, list_ele_t *l2)
{
    // merge with recursive
    if (!l2)
        return l1;
    if (!l1)
        return l2;

    if (strcasecmp(l1->value, l2->value) <= 0) {
        l1->next = merge(l1->next, l2);
        return l1;
    } else {
        l2->next = merge(l1, l2->next);
        return l2;
    }
}

list_ele_t *mergeSortList(list_ele_t *head)
{
    if (!head || !head->next)
        return head;

    list_ele_t *fast = head->next;
    list_ele_t *slow = head;

    // split list
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    fast = slow->next;
    slow->next = NULL;

    // sort each list
    list_ele_t *l1 = mergeSortList(head);
    list_ele_t *l2 = mergeSortList(fast);

    // merge sorted l1 and sorted l2
    return merge(l1, l2);
}

void q_sort(queue_t *q)
{
    list_ele_t *node;

    if (!q || !q->head)
        return;

    q->head = mergeSortList(q->head);

    node = q->head;
    while (node->next) {
        node = node->next;
    }
    q->tail = node;
}