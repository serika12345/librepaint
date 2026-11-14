#include <expat_config.h>
#include <expat.h>

#ifndef XML_DTD
#  error "The Krita iOS Expat contract requires DTD support"
#endif

#if ! defined(XML_GE) || XML_GE != 1
#  error "The Krita iOS Expat contract requires general entity support"
#endif

#ifndef XML_NS
#  error "The Krita iOS Expat contract requires namespace support"
#endif

#if ! defined(XML_CONTEXT_BYTES) || XML_CONTEXT_BYTES != 1024
#  error "The Krita iOS Expat contract requires 1024 context bytes"
#endif

#if XML_MAJOR_VERSION != 2 || XML_MINOR_VERSION != 9 || XML_MICRO_VERSION != 0
#  error "Unexpected Expat API version"
#endif

int main(void)
{
    static const char document[] =
        "<!DOCTYPE root [<!ENTITY value 'ok'>]>"
        "<root xmlns='urn:krita'>&value;</root>";
    XML_Parser parser = XML_ParserCreateNS(NULL, '|');

    if (parser == NULL) {
        return 1;
    }

    XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_ALWAYS);
    const enum XML_Status status =
        XML_Parse(parser, document, (int)(sizeof(document) - 1), XML_TRUE);
    XML_ParserFree(parser);
    return status == XML_STATUS_OK ? 0 : 1;
}
