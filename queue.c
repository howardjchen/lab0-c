#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

//#define SORT_BY_KERNEL_API true

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
    element_t *el, *safe;

    if (!head)
        return;

    list_for_each_entry_safe (el, safe, head, list)
        q_release_element(el);

    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *el = malloc(sizeof(element_t));
    if (!el)
        return false;

    el->value = strdup(s);
    if (!el->value) {
        free(el);
        return false;
    }

    list_add(&el->list, head);

    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *el = malloc(sizeof(element_t));
    if (!el)
        return false;

    el->value = strdup(s);
    if (!el->value) {
        free(el);
        return false;
    }

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
        size_t len = strlen(ele->value);
        if (len <= bufsize)
            memcpy(sp, ele->value, len + 1);
        else {
            memcpy(sp, ele->value, bufsize - 1);
            sp[bufsize - 1] = '\0';
        }
    }

    list_del(&ele->list);

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
        size_t len = strlen(ele->value);
        if (len <= bufsize)
            memcpy(sp, ele->value, len + 1);
        else {
            memcpy(sp, ele->value, bufsize - 1);
            sp[bufsize - 1] = '\0';
        }
    }

    list_del(&ele->list);

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
    q_release_element(list_entry(slow, element_t, list));

    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    element_t *el, *safe;
    char *target = NULL;

    if (!head || list_empty(head) || list_is_singular(head))
        return false;

    list_for_each_entry_safe (el, safe, head, list) {
        if (el->list.next != head) {
            if (strcmp(el->value, safe->value) == 0) {
                if (target)
                    free(target);
                target = strdup(el->value);
            }
        }

        /* Delete duplicate node and element */
        if (target) {
            if (strcmp(el->value, target) == 0) {
                list_del(&el->list);
                q_release_element(el);
            }
        }
    }

    /* Free the target if last node was deleted */
    if (target)
        free(target);

    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    struct list_head *node;

    if (!head || list_empty(head))
        return;
    list_for_each (node, head) {
        if (node->next != head)
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
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    struct list_head *tmp, *node, *safe;
    int reverse_num = q_size(head) / k;
    int cnt = 0;

    tmp = head;
    list_for_each_safe (node, safe, head) {
        if (reverse_num) {
            if (cnt == k) {
                tmp = node->prev;
                cnt = 0;
                reverse_num--;
            }
            list_move(node, tmp);
            cnt++;
        }
    }
}

#if SORT_BY_KERNEL_API
typedef unsigned char u8;
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
typedef int
    __attribute__((nonnull(2, 3))) (*list_cmp_func_t)(void *,
                                                      const struct list_head *,
                                                      const struct list_head *);

int sort_comp(void *p, const struct list_head *a, const struct list_head *b)
{
    // cppcheck-suppress nullPointer
    return strcmp(list_entry(a, element_t, list)->value,
                  // cppcheck-suppress nullPointer
                  list_entry(b, element_t, list)->value);
}

/*
 * Returns a list organized in an intermediate format suited
 * to chaining of merge() calls: null-terminated, no reserved or
 * sentinel head node, "prev" links not maintained.
 */
__attribute__((nonnull(2, 3, 4))) static struct list_head *
merge(void *priv, list_cmp_func_t cmp, struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

/*
 * Combine final list merge with restoration of standard doubly-linked
 * list structure.  This approach duplicates code from merge(), but
 * runs faster than the tidier alternatives of either a separate final
 * prev-link restoration pass, or maintaining the prev links
 * throughout.
 */
__attribute__((nonnull(2, 3, 4, 5))) static void merge_final(
    void *priv,
    list_cmp_func_t cmp,
    struct list_head *head,
    struct list_head *a,
    struct list_head *b)
{
    struct list_head *tail = head;
    u8 count = 0;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    tail->next = b;
    do {
        /*
         * If the merge is highly unbalanced (e.g. the input is
         * already sorted), this loop may run many iterations.
         * Continue callbacks to the client even though no
         * element comparison is needed, so the client's cmp()
         * routine can invoke cond_resched() periodically.
         */
        if (unlikely(!++count))
            cmp(priv, b, b);
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* And the final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
/**
 * list_sort - sort a list
 * @priv: private data, opaque to list_sort(), passed to @cmp
 * @head: the list to sort
 * @cmp: the elements comparison function
 *
 * The comparison function @cmp must return > 0 if @a should sort after
 * @b ("@a > @b" if you want an ascending sort), and <= 0 if @a should
 * sort before @b *or* their original order should be preserved.  It is
 * always called with the element that came first in the input in @a,
 * and list_sort is a stable sort, so it is not necessary to distinguish
 * the @a < @b and @a == @b cases.
 *
 * This is compatible with two styles of @cmp function:
 * - The traditional style which returns <0 / =0 / >0, or
 * - Returning a boolean 0/1.
 * The latter offers a chance to save a few cycles in the comparison
 * (which is used by e.g. plug_ctx_cmp() in block/blk-mq.c).
 *
 * A good way to write a multi-word comparison is::
 *
 *	if (a->high != b->high)
 *		return a->high > b->high;
 *	if (a->middle != b->middle)
 *		return a->middle > b->middle;
 *	return a->low > b->low;
 *
 *
 * This mergesort is as eager as possible while always performing at least
 * 2:1 balanced merges.  Given two pending sublists of size 2^k, they are
 * merged to a size-2^(k+1) list as soon as we have 2^k following elements.
 *
 * Thus, it will avoid cache thrashing as long as 3*2^k elements can
 * fit into the cache.  Not quite as good as a fully-eager bottom-up
 * mergesort, but it does use 0.2*n fewer comparisons, so is faster in
 * the common case that everything fits into L1.
 *
 *
 * The merging is controlled by "count", the number of elements in the
 * pending lists.  This is beautifully simple code, but rather subtle.
 *
 * Each time we increment "count", we set one bit (bit k) and clear
 * bits k-1 .. 0.  Each time this happens (except the very first time
 * for each bit, when count increments to 2^k), we merge two lists of
 * size 2^k into one list of size 2^(k+1).
 *
 * This merge happens exactly when the count reaches an odd multiple of
 * 2^k, which is when we have 2^k elements pending in smaller lists,
 * so it's safe to merge away two lists of size 2^k.
 *
 * After this happens twice, we have created two lists of size 2^(k+1),
 * which will be merged into a list of size 2^(k+2) before we create
 * a third list of size 2^(k+1), so there are never more than two pending.
 *
 * The number of pending lists of size 2^k is determined by the
 * state of bit k of "count" plus two extra pieces of information:
 *
 * - The state of bit k-1 (when k == 0, consider bit -1 always set), and
 * - Whether the higher-order bits are zero or non-zero (i.e.
 *   is count >= 2^(k+1)).
 *
 * There are six states we distinguish.  "x" represents some arbitrary
 * bits, and "y" represents some arbitrary non-zero bits:
 * 0:  00x: 0 pending of size 2^k;           x pending of sizes < 2^k
 * 1:  01x: 0 pending of size 2^k; 2^(k-1) + x pending of sizes < 2^k
 * 2: x10x: 0 pending of size 2^k; 2^k     + x pending of sizes < 2^k
 * 3: x11x: 1 pending of size 2^k; 2^(k-1) + x pending of sizes < 2^k
 * 4: y00x: 1 pending of size 2^k; 2^k     + x pending of sizes < 2^k
 * 5: y01x: 2 pending of size 2^k; 2^(k-1) + x pending of sizes < 2^k
 * (merge and loop back to state 2)
 *
 * We gain lists of size 2^k in the 2->3 and 4->5 transitions (because
 * bit k-1 is set while the more significant bits are non-zero) and
 * merge them away in the 5->2 transition.  Note in particular that just
 * before the 5->2 transition, all lower-order bits are 11 (state 3),
 * so there is one list of each smaller size.
 *
 * When we reach the end of the input, we merge all the pending
 * lists, from smallest to largest.  If you work through cases 2 to
 * 5 above, you can see that the number of elements we merge with a list
 * of size 2^k varies from 2^(k-1) (cases 3 and 5 when x == 0) to
 * 2^(k+1) - 1 (second merge of case 5 when x == 2^(k-1) - 1).
 */
__attribute__((nonnull(2, 3))) void list_sort(void *priv,
                                              struct list_head *head,
                                              list_cmp_func_t cmp)
{
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0; /* Count of pending */

    if (list == head->prev) /* Zero or one elements */
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    /*
     * Data structure invariants:
     * - All lists are singly linked and null-terminated; prev
     *   pointers are not maintained.
     * - pending is a prev-linked "list of lists" of sorted
     *   sublists awaiting further merging.
     * - Each of the sorted sublists is power-of-two in size.
     * - Sublists are sorted by size and age, smallest & newest at front.
     * - There are zero to two sublists of each size.
     * - A pair of pending sublists are merged as soon as the number
     *   of following pending elements equals their size (i.e.
     *   each time count reaches an odd multiple of that size).
     *   That ensures each later final merge will be at worst 2:1.
     * - Each round consists of:
     *   - Merging the two sublists selected by the highest bit
     *     which flips when count is incremented, and
     *   - Adding an element from the input as a size-1 sublist.
     */
    do {
        size_t bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;

            a = merge(priv, cmp, b, a);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        /* Move one element from input list to pending */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);

    /* End of input; merge together all the pending lists. */
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;
        list = merge(priv, cmp, pending, list);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    merge_final(priv, cmp, head, pending, list);
}

#else

struct list_head *merge_list(struct list_head *l1,
                             struct list_head *l2,
                             struct list_head *head,
                             bool descend,
                             size_t cnt)
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

    if (cnt == 1) {
        while (ptr->next != head)
            ptr = ptr->next;
        head->prev = ptr;
    }

    return tmp->next;
}

static struct list_head *mergesort_list(struct list_head *node,
                                        struct list_head *head,
                                        bool descend,
                                        size_t cnt)
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

    cnt++;
    left = mergesort_list(node, head, descend, cnt);
    right = mergesort_list(mid, head, descend, cnt);

    return merge_list(left, right, head, descend, cnt);
}
#endif

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;


#if defined(SORT_BY_KERNEL_API)
    list_sort(NULL, head, sort_comp);
#else

    head->next = mergesort_list(head->next, head, descend, 0);
    head->next->prev = head;

#endif
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    element_t *cur = NULL, *target, *prev = NULL;
    struct list_head *pos = NULL;

    if (!head || list_empty(head) || list_is_singular(head))
        return 0;

    list_for_each_entry (cur, head, list) {
        /* Release the element in the next round */
        if (prev) {
            q_release_element(prev);
            prev = NULL;
        }

        /* Check right side and find if there is greater value */
        if (cur->list.next)
            pos = cur->list.next;
        for (; pos != head; pos = pos->next) {
            target = list_entry(pos, element_t, list);
            if (strcmp(cur->value, target->value) > 0) {
                list_del(&cur->list);
                prev = cur;
                break;
            }
        }
    }

    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    element_t *cur = NULL, *target, *prev = NULL;
    struct list_head *pos = NULL;

    if (!head || list_empty(head) || list_is_singular(head))
        return 0;

    list_for_each_entry (cur, head, list) {
        /* Release the element in the next round */
        if (prev) {
            q_release_element(prev);
            prev = NULL;
        }

        /* Check right side and find if there is greater value */
        if (cur->list.next)
            pos = cur->list.next;
        for (; pos != head; pos = pos->next) {
            target = list_entry(pos, element_t, list);
            if (strcmp(cur->value, target->value) < 0) {
                list_del(&cur->list);
                prev = cur;
                break;
            }
        }
    }

    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return q_size(list_first_entry(head, queue_contex_t, chain)->q);

    queue_contex_t *first, *target = NULL;
    first = list_first_entry(head, queue_contex_t, chain);

    /* For move each target's queue to first context''s queue */
    list_for_each_entry (target, head->next, chain) {
        if (target->id == first->id)
            break;
        list_splice_tail_init(target->q, first->q);
    }
    q_sort(first->q, descend);
    head = first->q;

    return q_size(head);
}
