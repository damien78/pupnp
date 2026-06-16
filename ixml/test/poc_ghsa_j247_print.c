/* Regression test for GHSA-25cx-xv8x-j247.
 *
 * ixmlPrintDomTreeRecursive() in ixml/src/ixml.c recurses on firstChild and
 * nextSibling without a depth limit.  A deeply nested document triggers a
 * stack overflow when serialised via ixmlPrintDocument() or ixmlPrintNode().
 * The fix must replace the recursion with an iterative traversal. */

#include "ixml.h"

#include <stdlib.h>
#include <string.h>

/* 10 000 levels is enough to crash the old recursive implementation on
 * devices with a small thread stack, and completes in well under a second
 * with the iterative replacement. */
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

	/* ixmlPrintDocument calls ixmlPrintDomTree -> ixmlPrintDomTreeRecursive
	 */
	DOMString s = ixmlPrintDocument(doc);
	ixmlFreeDOMString(s);

	ixmlDocument_free(doc);
	return 0;
}
