#include "../sl.h"

#include <linux/stddef.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define prefetch(x) ((void) 0)
#define typeof(x) __typeof__(x)

struct list_head {
	struct list_head *next, *prev;
};

struct s {
    struct list_head h0;
    struct { } dummy;
    struct list_head h1;
    char a;
    struct list_head h2;
    char *b;
    struct list_head h3;
    int c[7];
    struct list_head h4;
    void (*f)(void);
    struct list_head h5;
};

int main()
{
#define PRINT_OFF(what) \
    printf("%s = %d\n", #what, (int)what)

    PRINT_OFF(offsetof(struct s, h0));
    PRINT_OFF(offsetof(struct s, h1));
    PRINT_OFF(offsetof(struct s, h2));
    PRINT_OFF(offsetof(struct s, h3));
    PRINT_OFF(offsetof(struct s, h4));
    PRINT_OFF(offsetof(struct s, h5));

    struct s s;
    struct list_head *h3 = &s.h3;
    struct s *ps = ((char *)h3 - offsetof(struct s, h3));
    if (ps != &s)
        free(ps);
}
