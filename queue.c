#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *q = malloc(sizeof(struct list_head));
    if (!q)
        return NULL;

    INIT_LIST_HEAD(q);

    return q;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    element_t *el, *el_next;

    list_for_each_entry_safe (el, el_next, head, list) {
        free(el->value);
        free(el);
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *el = malloc(sizeof(element_t));
    char *str = strdup(s);

    if (!str || !el) {
        free(el);
        return false;
    }

    el->value = str;
    list_add(&el->list, head);

    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *el = malloc(sizeof(element_t));
    char *str = strdup(s);

    if (!str || !el) {
        free(el);
        return false;
    }

    el->value = str;
    list_add_tail(&el->list, head);

    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    element_t *ele;

    if (!head || list_empty(head))
        return NULL;

    ele = list_first_entry(head, element_t, list);
    if (sp) {
        if (strlen(ele->value) <= bufsize)
            memcpy(sp, ele->value, strlen(ele->value) + 1);
        else {
            memcpy(sp, ele->value, bufsize - 1);
            sp[bufsize - 1] = '\0';
        }
    }

    list_del(head->next);

    return ele;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    element_t *ele;

    if (!head || list_empty(head))
        return NULL;

    ele = list_last_entry(head, element_t, list);
    if (sp) {
        if (strlen(ele->value) <= bufsize)
            memcpy(sp, ele->value, strlen(ele->value) + 1);
        else {
            memcpy(sp, ele->value, bufsize - 1);
            sp[bufsize - 1] = '\0';
        }
    }

    list_del(head->next);

    return ele;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    struct list_head *slow, *fast;

    if (!head || list_empty(head))
        return false;

    slow = head->next;
    fast = head->next;

    while (fast != head && fast->next != head) {
        fast = fast->next->next;
        slow = slow->next;
    }

    list_del(slow);

    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    element_t *el = NULL;
    char *target;

    q_sort(head, 0);

    target = list_entry(head, element_t, list)->value;

    list_for_each_entry (el, head->next, list) {
        if (strcmp(target, el->value) == 0)
            list_del(&el->list);
        else
            target = el->value;
    }

    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    struct list_head *node;

    if (!head || list_empty(head))
        return;
    list_for_each (node, head) {
        list_move(node, node->next);
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    struct list_head *node, *safe;

    if (!head || list_empty(head) || list_is_singular(head))
        return;

    list_for_each_safe (node, safe, head) {
        list_move(node, head);
    }
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    struct list_head *cur = head, *temp = head, *first_prev;
    int cnt = 0, cnt2 = 0;

    if (!head || list_empty(head) || list_is_singular(head))
        return;

    first_prev = cur->prev;
    while (cnt < k) {
        temp = cur->next;
        cur->next = cur->prev;
        cur->prev = temp;
        cnt++;
        if (cnt < k)
            cur = temp;
    }
    cur->prev = first_prev;

    /* traverse to the new last node */
    while (cnt2 < (k - 1)) {
        cur = cur->next;
        cnt2++;
    }
    cur->next = temp;
}

struct list_head *merge_list(struct list_head *l1,
                             struct list_head *l2,
                             struct list_head const *head,
                             bool descend)
{
    LIST_HEAD(tmp_head);
    struct list_head *ptr = &tmp_head, *tmp = &tmp_head;

    while ((l1 != head) && (l2 != head)) {
        if (descend) {
            if (strcmp(list_entry(l1, element_t, list)->value,
                       list_entry(l2, element_t, list)->value) >= 0) {
                ptr->next = l1;
                l1->prev = ptr;
                l1 = l1->next;
            } else {
                ptr->next = l2;
                l2->prev = ptr;
                l2 = l2->next;
            }
        } else {
            if (strcmp(list_entry(l1, element_t, list)->value,
                       list_entry(l2, element_t, list)->value) <= 0) {
                ptr->next = l1;
                l1->prev = ptr;
                l1 = l1->next;
            } else {
                ptr->next = l2;
                l2->prev = ptr;
                l2 = l2->next;
            }
        }
        ptr = ptr->next;
    }

    ptr->next = (l1 != head) ? l1 : l2;
    (ptr->next)->prev = ptr;

    return tmp->next;
}

static struct list_head *mergesort_list(struct list_head *node,
                                        struct list_head *head,
                                        bool descend)
{
    struct list_head *left, *right;

    if (!head || node->next == head)
        return node;

    struct list_head *slow = node;
    struct list_head *fast = slow->next->next;
    struct list_head *mid = NULL;

    while ((fast->next != head) && (fast != head)) {
        slow = slow->next;
        fast = fast->next->next;
    }

    mid = slow->next;
    slow->next = head;

    left = mergesort_list(node, head, descend);
    right = mergesort_list(mid, head, descend);

    return merge_list(left, right, head, descend);
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    head->next = mergesort_list(head->next, head, descend);
    head->next->prev = head;

    struct list_head *tmp = head->next;
    while (tmp->next != head)
        tmp = tmp->next;
    head->prev = tmp;
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    return 0;
}
