/* Regression test for GHSA-q54q-vx78-v9rq.
 *
 * ixmlNode_compare() in ixml/src/node.c calls strcmp() on nodeValue,
 * namespaceURI, prefix, and localName without NULL guards.  These fields are
 * legitimately NULL for non-namespaced attributes created via
 * ixmlDocument_createAttribute (which zeroes the struct via ixmlAttr_init).
 *
 * When ixmlElement_removeAttributeNode compares two different attribute
 * objects with the same nodeName, the pointer-equality fast path does not
 * fire and strcmp(NULL, NULL) crashes.
 *
 * The fix must replace bare strcmp() calls in ixmlNode_compare() with a
 * NULL-safe wrapper. */

#include "ixml.h"

#include <stdlib.h>

int main(void)
{
	IXML_Document *doc = NULL;
	IXML_Element *el = NULL;
	IXML_Attr *attached = NULL;
	IXML_Attr *lookup = NULL;
	IXML_Attr *removed = NULL;

	ixmlDocument_createDocumentEx(&doc);
	if (!doc)
		return 1;

	el = ixmlDocument_createElement(doc, "E");
	if (!el)
		return 1;

	/* Attach attribute "x"; nodeValue is NULL (zeroed by ixmlAttr_init) */
	attached = ixmlDocument_createAttribute(doc, "x");
	if (!attached)
		return 1;
	ixmlElement_setAttributeNode(el, attached, NULL);

	/* A second, distinct attribute object with the same name */
	lookup = ixmlDocument_createAttribute(doc, "x");
	if (!lookup)
		return 1;

	/* removeAttributeNode calls ixmlNode_compare(attached, lookup).
	 * Different pointers → fast path skipped → strcmp(NULL, NULL) → SEGV.
	 */
	ixmlElement_removeAttributeNode(el, lookup, &removed);

	ixmlAttr_free(lookup);
	if (removed) {
		ixmlAttr_free(removed);
	}
	ixmlElement_free(el);
	ixmlDocument_free(doc);
	return 0;
}
