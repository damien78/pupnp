/* Regression test for GHSA-25cx-xv8x-j247.
 *
 * ixmlNode_isAncestor() in ixml/src/node.c recurses on firstChild and
 * nextSibling without a depth limit.  It is called by ixmlNode_appendChild()
 * (and insertBefore/replaceChild) to detect hierarchy cycles.  Attempting to
 * append the root element under one of its own leaf descendants triggers a
 * full recursive descent of the 10 000-deep tree before the cycle is
 * detected, causing a stack overflow.  The fix must replace the recursion
 * with an iterative traversal. */

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

	/* Walk to the leaf iteratively to avoid any recursion in the test. */
	IXML_Node *root_elem = ixmlNode_getFirstChild((IXML_Node *)doc);
	IXML_Node *leaf = root_elem;
	IXML_Node *child;
	while ((child = ixmlNode_getFirstChild(leaf)) != NULL)
		leaf = child;

	/* appendChild(leaf, root_elem) calls isAncestor(root_elem, leaf), which
	 * must traverse ~10 000 levels before detecting the cycle.  The call
	 * returns IXML_HIERARCHY_REQUEST_ERR; the crash is in the recursion. */
	ixmlNode_appendChild(leaf, root_elem);

	ixmlDocument_free(doc);
	return 0;
}
