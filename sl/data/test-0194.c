#include "../sl.h"

#include <stdlib.h>

struct node {
    void            *data;
    struct node     *next;
    void            *list;
};

static void insert_node(struct node **pn)
{
    struct node *node = malloc(sizeof *node);
    if (!node)
        abort();

    node->data = malloc(0x100);
    node->next = *pn;
    node->list = pn;

    *pn = node;
    ___sl_plot(NULL);
}

int main()
{
    struct node *list = NULL;
    insert_node(&list);
    insert_node(&list);

    ___sl_plot(NULL, &list);

    while (list) {
        struct node *next = list->next;
        free(list->data);
        free(list);

        list = next;
    }

    return 0;
}