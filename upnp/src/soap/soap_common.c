
#include "config.h"
#if EXCLUDE_SOAP == 0

#include "httpparser.h"
#include "sock.h"
#include "soaplib.h"

const char *ContentTypeHeader =
    "Content-Type: text/xml; charset=\"utf-8\"\r\n";

#endif /* EXCLUDE_SOAP */

