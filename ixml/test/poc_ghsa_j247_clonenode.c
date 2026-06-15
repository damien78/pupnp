/* Regression test for GHSA-25cx-xv8x-j247.
 *
 * ixmlNode_cloneNodeTreeRecursive() in ixml/src/node.c recurses on firstChild
 * and nextSibling without a depth limit.  A deeply nested document triggers a
 * stack overflow when ixmlNode_cloneNode() is called with deep=1.  The fix
 * must replace the recursion with an iterative traversal. */

#include "ixml.h"

#include <stdlib.h>
#include <string.h>

#define NESTING_DEPTH 10000

int main(void);

int main(void)
{
	size_t len = (size_t)3 * NESTING_DEPTH + (size_t)4 * NESTING_DEPTH;
	char *buf = malloc(len + 1);
	if (!buf)
		return 1;
	char *p = buf;
	for (int i = 0; i < NESTING_DEPTH; i++) {
		memcpy(p, "<a>", 3);
		p += 3;
	}
	for (int i = 0; i < NESTING_DEPTH; i++) {
		memcpy(p, "</a>", 4);
		p += 4;
	}
	*p = '\0';

	IXML_Document *doc = NULL;
	int rc = ixmlParseBufferEx(buf, &doc);
	free(buf);
	if (rc != IXML_SUCCESS || !doc)
		return 1;

	/* Deep clone recurses into ixmlNode_cloneNodeTreeRecursive for each
	 * level of the 10 000-deep element tree. */
	IXML_Node *clone = ixmlNode_cloneNode((IXML_Node *)doc, 1);
	ixmlDocument_free((IXML_Document *)clone);

	ixmlDocument_free(doc);
	return 0;
}
