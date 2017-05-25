#pragma once
#include <stdio.h>
#include <assert.h>

#define DEBUG
#ifdef DEBUG
#define DPRINT(fmt, ...)	\
	fprintf(stderr, "[DEBUG][%s -- line %d]", __FILE__, __LINE__);	\
	fprintf(stderr, fmt, ## __VA_ARGS__);	\
	fprintf(stderr, "\n");
#endif // DEBUG
