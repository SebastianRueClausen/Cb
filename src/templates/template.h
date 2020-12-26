#ifndef TEMPLATE_DEF_H
#define TEMPLATE_DEF_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#define TEMPLATE_JOIN(a, b) a##b

#define TEMPLATE_COMBINE(a, b) TEMPLATE_JOIN(a, b)

#define TEMPLATE_SIGNATURE(prefix, name) TEMPLATE_COMBINE(prefix, TEMPLATE_COMBINE(_, name))

#endif
