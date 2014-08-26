/*
 * XML DRI client-side driver configuration
 * Copyright (C) 2003 Felix Kuehling
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * FELIX KUEHLING, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */
/**
 * \file xmlconfig.c
 * \brief Driver-independent client-side part of the XML configuration
 * \author Felix Kuehling
 */

#include <string.h>
#include <assert.h>
#include <expat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "main/imports.h"
#include "utils.h"
#include "xmlconfig.h"

#undef GET_PROGRAM_NAME

#if (defined(__GNU_LIBRARY__) || defined(__GLIBC__)) && !defined(__UCLIBC__)
#    if !defined(__GLIBC__) || (__GLIBC__ < 2)
/* These aren't declared in any libc5 header */
extern char *program_invocation_name, *program_invocation_short_name;
#    endif
#    define GET_PROGRAM_NAME() program_invocation_short_name
#elif defined(__CYGWIN__)
#    define GET_PROGRAM_NAME() program_invocation_short_name
#elif defined(__FreeBSD__) && (__FreeBSD__ >= 2)
#    include <osreldate.h>
#    if (__FreeBSD_version >= 440000)
#        include <stdlib.h>
#        define GET_PROGRAM_NAME() getprogname()
#    endif
#elif defined(__NetBSD__) && defined(__NetBSD_Version) && (__NetBSD_Version >= 106000100)
#    include <stdlib.h>
#    define GET_PROGRAM_NAME() getprogname()
#elif defined(__APPLE__)
#    include <stdlib.h>
#    define GET_PROGRAM_NAME() getprogname()
#elif defined(__sun)
/* Solaris has getexecname() which returns the full path - return just
   the basename to match BSD getprogname() */
#    include <stdlib.h>
#    include <libgen.h>

static const char *__getProgramName () {
    static const char *progname;

    if (progname == NULL) {
	const char *e = getexecname();
	if (e != NULL) {
	    /* Have to make a copy since getexecname can return a readonly
	       string, but basename expects to be able to modify its arg. */
	    char *n = strdup(e);
	    if (n != NULL) {
		progname = basename(n);
	    }
	}
    }
    return progname;
}

#    define GET_PROGRAM_NAME() __getProgramName()
#endif

#if !defined(GET_PROGRAM_NAME)
#    if defined(__OpenBSD__) || defined(NetBSD) || defined(__UCLIBC__) || defined(ANDROID)
/* This is a hack. It's said to work on OpenBSD, NetBSD and GNU.
 * Rogelio M.Serrano Jr. reported it's also working with UCLIBC. It's
 * used as a last resort, if there is no documented facility available. */
static const char *__getProgramName () {
    extern const char *__progname;
    char * arg = strrchr(__progname, '/');
    if (arg)
        return arg+1;
    else
        return __progname;
}
#        define GET_PROGRAM_NAME() __getProgramName()
#    else
#        define GET_PROGRAM_NAME() ""
#        warning "Per application configuration won't work with your OS version."
#    endif
#endif

/** \brief Find an option in an option cache with the name as key */
static uint32_t findOption (const driOptionCache *cache, const char *name) {
    uint32_t len = strlen (name);
    uint32_t size = 1 << cache->tableSize, mask = size - 1;
    uint32_t hash = 0;
    uint32_t i, shift;

  /* compute a hash from the variable length name */
    for (i = 0, shift = 0; i < len; ++i, shift = (shift+8) & 31)
	hash += (uint32_t)name[i] << shift;
    hash *= hash;
    hash = (hash >> (16-cache->tableSize/2)) & mask;

  /* this is just the starting point of the linear search for the option */
    for (i = 0; i < size; ++i, hash = (hash+1) & mask) {
      /* if we hit an empty entry then the option is not defined (yet) */
	if (cache->info[hash].name == 0)
	    break;
	else if (!strcmp (name, cache->info[hash].name))
	    break;
    }
  /* this assertion fails if the hash table is full */
    assert (i < size);

    return hash;
}

/** \brief Like strdup but using malloc and with error checking. */
#define XSTRDUP(dest,source) do { \
    uint32_t len = strlen (source); \
    if (!(dest = malloc(len+1))) { \
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__); \
	abort(); \
    } \
    memcpy (dest, source, len+1); \
} while (0)

static int compare (const void *a, const void *b) {
    return strcmp (*(char *const*)a, *(char *const*)b);
}
/** \brief Binary search in a string array. */
static uint32_t bsearchStr (const XML_Char *name,
			  const XML_Char *elems[], uint32_t count) {
    const XML_Char **found;
    found = bsearch (&name, elems, count, sizeof (XML_Char *), compare);
    if (found)
	return found - elems;
    else
	return count;
}

/** \brief Locale-independent integer parser.
 *
 * Works similar to strtol. Leading space is NOT skipped. The input
 * number may have an optional sign. Radix is specified by base. If
 * base is 0 then decimal is assumed unless the input number is
 * prefixed by 0x or 0X for hexadecimal or 0 for octal. After
 * returning tail points to the first character that is not part of
 * the integer number. If no number was found then tail points to the
 * start of the input string. */
static int strToI (const XML_Char *string, const XML_Char **tail, int base) {
    int radix = base == 0 ? 10 : base;
    int result = 0;
    int sign = 1;
    bool numberFound = false;
    const XML_Char *start = string;

    assert (radix >= 2 && radix <= 36);

    if (*string == '-') {
	sign = -1;
	string++;
    } else if (*string == '+')
	string++;
    if (base == 0 && *string == '0') {
	numberFound = true;
	if (*(string+1) == 'x' || *(string+1) == 'X') {
	    radix = 16;
	    string += 2;
	} else {
	    radix = 8;
	    string++;
	}
    }
    do {
	int digit = -1;
	if (radix <= 10) {
	    if (*string >= '0' && *string < '0' + radix)
		digit = *string - '0';
	} else {
	    if (*string >= '0' && *string <= '9')
		digit = *string - '0';
	    else if (*string >= 'a' && *string < 'a' + radix - 10)
		digit = *string - 'a' + 10;
	    else if (*string >= 'A' && *string < 'A' + radix - 10)
		digit = *string - 'A' + 10;
	}
	if (digit != -1) {
	    numberFound = true;
	    result = radix*result + digit;
	    string++;
	} else
	    break;
    } while (true);
    *tail = numberFound ? string : start;
    return sign * result;
}

/** \brief Locale-independent floating-point parser.
 *
 * Works similar to strtod. Leading space is NOT skipped. The input
 * number may have an optional sign. '.' is interpreted as decimal
 * point and may occur at most once. Optionally the number may end in
 * [eE]<exponent>, where <exponent> is an integer as recognized by
 * strToI. In that case the result is number * 10^exponent. After
 * returning tail points to the first character that is not part of
 * the floating point number. If no number was found then tail points
 * to the start of the input string.
 *
 * Uses two passes for maximum accuracy. */
static float strToF (const XML_Char *string, const XML_Char **tail) {
    int nDigits = 0, pointPos, exponent;
    float sign = 1.0f, result = 0.0f, scale;
    const XML_Char *start = string, *numStart;

    /* sign */
    if (*string == '-') {
	sign = -1.0f;
	string++;
    } else if (*string == '+')
	string++;

    /* first pass: determine position of decimal point, number of
     * digits, exponent and the end of the number. */
    numStart = string;
    while (*string >= '0' && *string <= '9') {
	string++;
	nDigits++;
    }
    pointPos = nDigits;
    if (*string == '.') {
	string++;
	while (*string >= '0' && *string <= '9') {
	    string++;
	    nDigits++;
	}
    }
    if (nDigits == 0) {
	/* no digits, no number */
	*tail = start;
	return 0.0f;
    }
    *tail = string;
    if (*string == 'e' || *string == 'E') {
	const XML_Char *expTail;
	exponent = strToI (string+1, &expTail, 10);
	if (expTail == string+1)
	    exponent = 0;
	else
	    *tail = expTail;
    } else
	exponent = 0;
    string = numStart;

    /* scale of the first digit */
    scale = sign * (float)pow (10.0, (double)(pointPos-1 + exponent));

    /* second pass: parse digits */
    do {
	if (*string != '.') {
	    assert (*string >= '0' && *string <= '9');
	    result += scale * (float)(*string - '0');
	    scale *= 0.1f;
	    nDigits--;
	}
	string++;
    } while (nDigits > 0);

    return result;
}

/** \brief Parse a value of a given type. */
static unsigned char parseValue (driOptionValue *v, driOptionType type,
			     const XML_Char *string) {
    const XML_Char *tail = NULL;
  /* skip leading white-space */
    string += strspn (string, " \f\n\r\t\v");
    switch (type) {
      case DRI_BOOL:
	if (!strcmp (string, "false")) {
	    v->_bool = false;
	    tail = string + 5;
	} else if (!strcmp (string, "true")) {
	    v->_bool = true;
	    tail = string + 4;
	}
	else
	    return false;
	break;
      case DRI_ENUM: /* enum is just a special integer */
      case DRI_INT:
	v->_int = strToI (string, &tail, 0);
	break;
      case DRI_FLOAT:
	v->_float = strToF (string, &tail);
	break;
      case DRI_STRING:
	if (v->_string)
	    free (v->_string);
	v->_string = strndup(string, STRING_CONF_MAXLEN);
	return GL_TRUE;
    }

    if (tail == string)
	return false; /* empty string (or containing only white-space) */
  /* skip trailing white space */
    if (*tail)
	tail += strspn (tail, " \f\n\r\t\v");
    if (*tail)
	return false; /* something left over that is not part of value */

    return true;
}

/** \brief Parse a list of ranges of type info->type. */
static unsigned char parseRanges (driOptionInfo *info, const XML_Char *string) {
    XML_Char *cp, *range;
    uint32_t nRanges, i;
    driOptionRange *ranges;

    XSTRDUP (cp, string);
  /* pass 1: determine the number of ranges (number of commas + 1) */
    range = cp;
    for (nRanges = 1; *range; ++range)
	if (*range == ',')
	    ++nRanges;

    if ((ranges = malloc(nRanges*sizeof(driOptionRange))) == NULL) {
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
	abort();
    }

  /* pass 2: parse all ranges into preallocated array */
    range = cp;
    for (i = 0; i < nRanges; ++i) {
	XML_Char *end, *sep;
	assert (range);
	end = strchr (range, ',');
	if (end)
	    *end = '\0';
	sep = strchr (range, ':');
	if (sep) { /* non-empty interval */
	    *sep = '\0';
	    if (!parseValue (&ranges[i].start, info->type, range) ||
		!parseValue (&ranges[i].end, info->type, sep+1))
	        break;
	    if (info->type == DRI_INT &&
		ranges[i].start._int > ranges[i].end._int)
		break;
	    if (info->type == DRI_FLOAT &&
		ranges[i].start._float > ranges[i].end._float)
		break;
	} else { /* empty interval */
	    if (!parseValue (&ranges[i].start, info->type, range))
		break;
	    ranges[i].end = ranges[i].start;
	}
	if (end)
	    range = end+1;
	else
	    range = NULL;
    }
    free(cp);
    if (i < nRanges) {
	free(ranges);
	return false;
    } else
	assert (range == NULL);

    info->nRanges = nRanges;
    info->ranges = ranges;
    return true;
}

/** \brief Check if a value is in one of info->ranges. */
static bool checkValue (const driOptionValue *v, const driOptionInfo *info) {
    uint32_t i;
    assert (info->type != DRI_BOOL); /* should be caught by the parser */
    if (info->nRanges == 0)
	return true;
    switch (info->type) {
      case DRI_ENUM: /* enum is just a special integer */
      case DRI_INT:
	for (i = 0; i < info->nRanges; ++i)
	    if (v->_int >= info->ranges[i].start._int &&
		v->_int <= info->ranges[i].end._int)
		return true;
	break;
      case DRI_FLOAT:
	for (i = 0; i < info->nRanges; ++i)
	    if (v->_float >= info->ranges[i].start._float &&
		v->_float <= info->ranges[i].end._float)
		return true;
	break;
      case DRI_STRING:
	break;
      default:
	assert (0); /* should never happen */
    }
    return false;
}

/**
 * Print message to \c stderr if the \c LIBGL_DEBUG environment variable
 * is set. 
 * 
 * Is called from the drivers.
 * 
 * \param f \c printf like format string.
 */
static void
__driUtilMessage(const char *f, ...)
{
    va_list args;
    const char *libgl_debug;

    libgl_debug=getenv("LIBGL_DEBUG");
    if (libgl_debug && !strstr(libgl_debug, "quiet")) {
        fprintf(stderr, "libGL: ");
        va_start(args, f);
        vfprintf(stderr, f, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

/** \brief Output a warning message. */
#define XML_WARNING1(msg) do {\
    __driUtilMessage ("Warning in %s line %d, column %d: "msg, data->name, \
                      (int) XML_GetCurrentLineNumber(data->parser), \
                      (int) XML_GetCurrentColumnNumber(data->parser)); \
} while (0)
#define XML_WARNING(msg,args...) do { \
    __driUtilMessage ("Warning in %s line %d, column %d: "msg, data->name, \
                      (int) XML_GetCurrentLineNumber(data->parser), \
                      (int) XML_GetCurrentColumnNumber(data->parser), \
                      args); \
} while (0)
/** \brief Output an error message. */
#define XML_ERROR1(msg) do { \
    __driUtilMessage ("Error in %s line %d, column %d: "msg, data->name, \
                      (int) XML_GetCurrentLineNumber(data->parser), \
                      (int) XML_GetCurrentColumnNumber(data->parser)); \
} while (0)
#define XML_ERROR(msg,args...) do { \
    __driUtilMessage ("Error in %s line %d, column %d: "msg, data->name, \
                      (int) XML_GetCurrentLineNumber(data->parser), \
                      (int) XML_GetCurrentColumnNumber(data->parser), \
                      args); \
} while (0)
/** \brief Output a fatal error message and abort. */
#define XML_FATAL1(msg) do { \
    fprintf (stderr, "Fatal error in %s line %d, column %d: "msg"\n", \
             data->name, \
             (int) XML_GetCurrentLineNumber(data->parser),	\
             (int) XML_GetCurrentColumnNumber(data->parser)); \
    abort();\
} while (0)
#define XML_FATAL(msg,args...) do { \
    fprintf (stderr, "Fatal error in %s line %d, column %d: "msg"\n", \
             data->name, \
             (int) XML_GetCurrentLineNumber(data->parser),	\
             (int) XML_GetCurrentColumnNumber(data->parser),		\
             args); \
    abort();\
} while (0)

/** \brief Parser context for __driConfigOptions. */
struct OptInfoData {
    const char *name;
    XML_Parser parser;
    driOptionCache *cache;
    bool inDriInfo;
    bool inSection;
    bool inDesc;
    bool inOption;
    bool inEnum;
    int curOption;
};

/** \brief Elements in __driConfigOptions. */
enum OptInfoElem {
    OI_DESCRIPTION = 0, OI_DRIINFO, OI_ENUM, OI_OPTION, OI_SECTION, OI_COUNT
};
static const XML_Char *OptInfoElems[] = {
    "description", "driinfo", "enum", "option", "section"
};

/** \brief Parse attributes of an enum element.
 *
 * We're not actually interested in the data. Just make sure this is ok
 * for external configuration tools.
 */
static void parseEnumAttr (struct OptInfoData *data, const XML_Char **attr) {
    uint32_t i;
    const XML_Char *value = NULL, *text = NULL;
    driOptionValue v;
    uint32_t opt = data->curOption;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "value")) value = attr[i+1];
	else if (!strcmp (attr[i], "text")) text = attr[i+1];
	else XML_FATAL("illegal enum attribute: %s.", attr[i]);
    }
    if (!value) XML_FATAL1 ("value attribute missing in enum.");
    if (!text) XML_FATAL1 ("text attribute missing in enum.");
     if (!parseValue (&v, data->cache->info[opt].type, value))
	XML_FATAL ("illegal enum value: %s.", value);
    if (!checkValue (&v, &data->cache->info[opt]))
	XML_FATAL ("enum value out of valid range: %s.", value);
}

/** \brief Parse attributes of a description element.
 *
 * We're not actually interested in the data. Just make sure this is ok
 * for external configuration tools.
 */
static void parseDescAttr (struct OptInfoData *data, const XML_Char **attr) {
    uint32_t i;
    const XML_Char *lang = NULL, *text = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "lang")) lang = attr[i+1];
	else if (!strcmp (attr[i], "text")) text = attr[i+1];
	else XML_FATAL("illegal description attribute: %s.", attr[i]);
    }
    if (!lang) XML_FATAL1 ("lang attribute missing in description.");
    if (!text) XML_FATAL1 ("text attribute missing in description.");
}

/** \brief Parse attributes of an option element. */
static void parseOptInfoAttr (struct OptInfoData *data, const XML_Char **attr) {
    enum OptAttr {OA_DEFAULT = 0, OA_NAME, OA_TYPE, OA_VALID, OA_COUNT};
    static const XML_Char *optAttr[] = {"default", "name", "type", "valid"};
    const XML_Char *attrVal[OA_COUNT] = {NULL, NULL, NULL, NULL};
    const char *defaultVal;
    driOptionCache *cache = data->cache;
    uint32_t opt, i;
    for (i = 0; attr[i]; i += 2) {
	uint32_t attrName = bsearchStr (attr[i], optAttr, OA_COUNT);
	if (attrName >= OA_COUNT)
	    XML_FATAL ("illegal option attribute: %s", attr[i]);
	attrVal[attrName] = attr[i+1];
    }
    if (!attrVal[OA_NAME]) XML_FATAL1 ("name attribute missing in option.");
    if (!attrVal[OA_TYPE]) XML_FATAL1 ("type attribute missing in option.");
    if (!attrVal[OA_DEFAULT]) XML_FATAL1 ("default attribute missing in option.");

    opt = findOption (cache, attrVal[OA_NAME]);
    if (cache->info[opt].name)
	XML_FATAL ("option %s redefined.", attrVal[OA_NAME]);
    data->curOption = opt;

    XSTRDUP (cache->info[opt].name, attrVal[OA_NAME]);

    if (!strcmp (attrVal[OA_TYPE], "bool"))
	cache->info[opt].type = DRI_BOOL;
    else if (!strcmp (attrVal[OA_TYPE], "enum"))
	cache->info[opt].type = DRI_ENUM;
    else if (!strcmp (attrVal[OA_TYPE], "int"))
	cache->info[opt].type = DRI_INT;
    else if (!strcmp (attrVal[OA_TYPE], "float"))
	cache->info[opt].type = DRI_FLOAT;
    else if (!strcmp (attrVal[OA_TYPE], "string"))
	cache->info[opt].type = DRI_STRING;
    else
	XML_FATAL ("illegal type in option: %s.", attrVal[OA_TYPE]);

    defaultVal = getenv (cache->info[opt].name);
    if (defaultVal != NULL) {
      /* don't use XML_WARNING, we want the user to see this! */
	fprintf (stderr,
		 "ATTENTION: default value of option %s overridden by environment.\n",
		 cache->info[opt].name);
    } else
	defaultVal = attrVal[OA_DEFAULT];
    if (!parseValue (&cache->values[opt], cache->info[opt].type, defaultVal))
	XML_FATAL ("illegal default value for %s: %s.", cache->info[opt].name, defaultVal);

    if (attrVal[OA_VALID]) {
	if (cache->info[opt].type == DRI_BOOL)
	    XML_FATAL1 ("boolean option with valid attribute.");
	if (!parseRanges (&cache->info[opt], attrVal[OA_VALID]))
	    XML_FATAL ("illegal valid attribute: %s.", attrVal[OA_VALID]);
	if (!checkValue (&cache->values[opt], &cache->info[opt]))
	    XML_FATAL ("default value out of valid range '%s': %s.",
		       attrVal[OA_VALID], defaultVal);
    } else if (cache->info[opt].type == DRI_ENUM) {
	XML_FATAL1 ("valid attribute missing in option (mandatory for enums).");
    } else {
	cache->info[opt].nRanges = 0;
	cache->info[opt].ranges = NULL;
    }
}

/** \brief Handler for start element events. */
static void optInfoStartElem (void *userData, const XML_Char *name,
			      const XML_Char **attr) {
    struct OptInfoData *data = (struct OptInfoData *)userData;
    enum OptInfoElem elem = bsearchStr (name, OptInfoElems, OI_COUNT);
    switch (elem) {
      case OI_DRIINFO:
	if (data->inDriInfo)
	    XML_FATAL1 ("nested <driinfo> elements.");
	if (attr[0])
	    XML_FATAL1 ("attributes specified on <driinfo> element.");
	data->inDriInfo = true;
	break;
      case OI_SECTION:
	if (!data->inDriInfo)
	    XML_FATAL1 ("<section> must be inside <driinfo>.");
	if (data->inSection)
	    XML_FATAL1 ("nested <section> elements.");
	if (attr[0])
	    XML_FATAL1 ("attributes specified on <section> element.");
	data->inSection = true;
	break;
      case OI_DESCRIPTION:
	if (!data->inSection && !data->inOption)
	    XML_FATAL1 ("<description> must be inside <description> or <option.");
	if (data->inDesc)
	    XML_FATAL1 ("nested <description> elements.");
	data->inDesc = true;
	parseDescAttr (data, attr);
	break;
      case OI_OPTION:
	if (!data->inSection)
	    XML_FATAL1 ("<option> must be inside <section>.");
	if (data->inDesc)
	    XML_FATAL1 ("<option> nested in <description> element.");
	if (data->inOption)
	    XML_FATAL1 ("nested <option> elements.");
	data->inOption = true;
	parseOptInfoAttr (data, attr);
	break;
      case OI_ENUM:
	if (!(data->inOption && data->inDesc))
	    XML_FATAL1 ("<enum> must be inside <option> and <description>.");
	if (data->inEnum)
	    XML_FATAL1 ("nested <enum> elements.");
	data->inEnum = true;
	parseEnumAttr (data, attr);
	break;
      default:
	XML_FATAL ("unknown element: %s.", name);
    }
}

/** \brief Handler for end element events. */
static void optInfoEndElem (void *userData, const XML_Char *name) {
    struct OptInfoData *data = (struct OptInfoData *)userData;
    enum OptInfoElem elem = bsearchStr (name, OptInfoElems, OI_COUNT);
    switch (elem) {
      case OI_DRIINFO:
	data->inDriInfo = false;
	break;
      case OI_SECTION:
	data->inSection = false;
	break;
      case OI_DESCRIPTION:
	data->inDesc = false;
	break;
      case OI_OPTION:
	data->inOption = false;
	break;
      case OI_ENUM:
	data->inEnum = false;
	break;
      default:
	assert (0); /* should have been caught by StartElem */
    }
}

void driParseOptionInfo (driOptionCache *info, const char *configOptions) {
    XML_Parser p;
    int status;
    struct OptInfoData userData;
    struct OptInfoData *data = &userData;

    /* Make the hash table big enough to fit more than the maximum number of
     * config options we've ever seen in a driver.
     */
    info->tableSize = 6;
    info->info = calloc(1 << info->tableSize, sizeof (driOptionInfo));
    info->values = calloc(1 << info->tableSize, sizeof (driOptionValue));
    if (info->info == NULL || info->values == NULL) {
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
	abort();
    }

    p = XML_ParserCreate ("UTF-8"); /* always UTF-8 */
    XML_SetElementHandler (p, optInfoStartElem, optInfoEndElem);
    XML_SetUserData (p, data);

    userData.name = "__driConfigOptions";
    userData.parser = p;
    userData.cache = info;
    userData.inDriInfo = false;
    userData.inSection = false;
    userData.inDesc = false;
    userData.inOption = false;
    userData.inEnum = false;
    userData.curOption = -1;

    status = XML_Parse (p, configOptions, strlen (configOptions), 1);
    if (!status)
	XML_FATAL ("%s.", XML_ErrorString(XML_GetErrorCode(p)));

    XML_ParserFree (p);
}

/** \brief Parser context for configuration files. */
struct OptConfData {
    const char *name;
    XML_Parser parser;
    driOptionCache *cache;
    int screenNum;
    const char *driverName, *execName;
    uint32_t ignoringDevice;
    uint32_t ignoringApp;
    uint32_t inDriConf;
    uint32_t inDevice;
    uint32_t inApp;
    uint32_t inOption;
};

/** \brief Elements in configuration files. */
enum OptConfElem {
    OC_APPLICATION = 0, OC_DEVICE, OC_DRICONF, OC_OPTION, OC_COUNT
};
static const XML_Char *OptConfElems[] = {
    "application", "device", "driconf", "option"
};

/** \brief Parse attributes of a device element. */
static void parseDeviceAttr (struct OptConfData *data, const XML_Char **attr) {
    uint32_t i;
    const XML_Char *driver = NULL, *screen = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "driver")) driver = attr[i+1];
	else if (!strcmp (attr[i], "screen")) screen = attr[i+1];
	else XML_WARNING("unknown device attribute: %s.", attr[i]);
    }
    if (driver && strcmp (driver, data->driverName))
	data->ignoringDevice = data->inDevice;
    else if (screen) {
	driOptionValue screenNum;
	if (!parseValue (&screenNum, DRI_INT, screen))
	    XML_WARNING("illegal screen number: %s.", screen);
	else if (screenNum._int != data->screenNum)
	    data->ignoringDevice = data->inDevice;
    }
}

/** \brief Parse attributes of an application element. */
static void parseAppAttr (struct OptConfData *data, const XML_Char **attr) {
    uint32_t i;
    const XML_Char *exec = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "name")) /* not needed here */;
	else if (!strcmp (attr[i], "executable")) exec = attr[i+1];
	else XML_WARNING("unknown application attribute: %s.", attr[i]);
    }
    if (exec && strcmp (exec, data->execName))
	data->ignoringApp = data->inApp;
}

/** \brief Parse attributes of an option element. */
static void parseOptConfAttr (struct OptConfData *data, const XML_Char **attr) {
    uint32_t i;
    const XML_Char *name = NULL, *value = NULL;
    for (i = 0; attr[i]; i += 2) {
	if (!strcmp (attr[i], "name")) name = attr[i+1];
	else if (!strcmp (attr[i], "value")) value = attr[i+1];
	else XML_WARNING("unknown option attribute: %s.", attr[i]);
    }
    if (!name) XML_WARNING1 ("name attribute missing in option.");
    if (!value) XML_WARNING1 ("value attribute missing in option.");
    if (name && value) {
	driOptionCache *cache = data->cache;
	uint32_t opt = findOption (cache, name);
	if (cache->info[opt].name == NULL)
            /* don't use XML_WARNING, drirc defines options for all drivers,
             * but not all drivers support them */
            return;
	else if (getenv (cache->info[opt].name))
	  /* don't use XML_WARNING, we want the user to see this! */
	    fprintf (stderr, "ATTENTION: option value of option %s ignored.\n",
		     cache->info[opt].name);
	else if (!parseValue (&cache->values[opt], cache->info[opt].type, value))
	    XML_WARNING ("illegal option value: %s.", value);
    }
}

/** \brief Handler for start element events. */
static void optConfStartElem (void *userData, const XML_Char *name,
			      const XML_Char **attr) {
    struct OptConfData *data = (struct OptConfData *)userData;
    enum OptConfElem elem = bsearchStr (name, OptConfElems, OC_COUNT);
    switch (elem) {
      case OC_DRICONF:
	if (data->inDriConf)
	    XML_WARNING1 ("nested <driconf> elements.");
	if (attr[0])
	    XML_WARNING1 ("attributes specified on <driconf> element.");
	data->inDriConf++;
	break;
      case OC_DEVICE:
	if (!data->inDriConf)
	    XML_WARNING1 ("<device> should be inside <driconf>.");
	if (data->inDevice)
	    XML_WARNING1 ("nested <device> elements.");
	data->inDevice++;
	if (!data->ignoringDevice && !data->ignoringApp)
	    parseDeviceAttr (data, attr);
	break;
      case OC_APPLICATION:
	if (!data->inDevice)
	    XML_WARNING1 ("<application> should be inside <device>.");
	if (data->inApp)
	    XML_WARNING1 ("nested <application> elements.");
	data->inApp++;
	if (!data->ignoringDevice && !data->ignoringApp)
	    parseAppAttr (data, attr);
	break;
      case OC_OPTION:
	if (!data->inApp)
	    XML_WARNING1 ("<option> should be inside <application>.");
	if (data->inOption)
	    XML_WARNING1 ("nested <option> elements.");
	data->inOption++;
	if (!data->ignoringDevice && !data->ignoringApp)
	    parseOptConfAttr (data, attr);
	break;
      default:
	XML_WARNING ("unknown element: %s.", name);
    }
}

/** \brief Handler for end element events. */
static void optConfEndElem (void *userData, const XML_Char *name) {
    struct OptConfData *data = (struct OptConfData *)userData;
    enum OptConfElem elem = bsearchStr (name, OptConfElems, OC_COUNT);
    switch (elem) {
      case OC_DRICONF:
	data->inDriConf--;
	break;
      case OC_DEVICE:
	if (data->inDevice-- == data->ignoringDevice)
	    data->ignoringDevice = 0;
	break;
      case OC_APPLICATION:
	if (data->inApp-- == data->ignoringApp)
	    data->ignoringApp = 0;
	break;
      case OC_OPTION:
	data->inOption--;
	break;
      default:
	/* unknown element, warning was produced on start tag */;
    }
}

/** \brief Initialize an option cache based on info */
static void initOptionCache (driOptionCache *cache, const driOptionCache *info) {
    GLuint i, size = 1 << info->tableSize;
    cache->info = info->info;
    cache->tableSize = info->tableSize;
    cache->values = malloc((1<<info->tableSize) * sizeof (driOptionValue));
    if (cache->values == NULL) {
	fprintf (stderr, "%s: %d: out of memory.\n", __FILE__, __LINE__);
	abort();
    }
    memcpy (cache->values, info->values,
	    (1<<info->tableSize) * sizeof (driOptionValue));
    for (i = 0; i < size; ++i) {
	if (cache->info[i].type == DRI_STRING)
	    XSTRDUP(cache->values[i]._string, info->values[i]._string);
    }
}

/** \brief Parse the named configuration file */
static void parseOneConfigFile (XML_Parser p) {
#define BUF_SIZE 0x1000
    struct OptConfData *data = (struct OptConfData *)XML_GetUserData (p);
    int status;
    int fd;

    if ((fd = open (data->name, O_RDONLY)) == -1) {
	__driUtilMessage ("Can't open configuration file %s: %s.",
			  data->name, strerror (errno));
	return;
    }

    while (1) {
	int bytesRead;
	void *buffer = XML_GetBuffer (p, BUF_SIZE);
	if (!buffer) {
	    __driUtilMessage ("Can't allocate parser buffer.");
	    break;
	}
	bytesRead = read (fd, buffer, BUF_SIZE);
	if (bytesRead == -1) {
	    __driUtilMessage ("Error reading from configuration file %s: %s.",
			      data->name, strerror (errno));
	    break;
	}
	status = XML_ParseBuffer (p, bytesRead, bytesRead == 0);
	if (!status) {
	    XML_ERROR ("%s.", XML_ErrorString(XML_GetErrorCode(p)));
	    break;
	}
	if (bytesRead == 0)
	    break;
    }

    close (fd);
#undef BUF_SIZE
}

void driParseConfigFiles (driOptionCache *cache, const driOptionCache *info,
			  int screenNum, const char *driverName) {
    char *filenames[2] = {"/etc/drirc", NULL};
    char *home;
    uint32_t i;
    struct OptConfData userData;

    initOptionCache (cache, info);

    userData.cache = cache;
    userData.screenNum = screenNum;
    userData.driverName = driverName;
    userData.execName = GET_PROGRAM_NAME();

    if ((home = getenv ("HOME"))) {
	uint32_t len = strlen (home);
	filenames[1] = malloc(len + 7+1);
	if (filenames[1] == NULL)
	    __driUtilMessage ("Can't allocate memory for %s/.drirc.", home);
	else {
	    memcpy (filenames[1], home, len);
	    memcpy (filenames[1] + len, "/.drirc", 7+1);
	}
    }

    for (i = 0; i < 2; ++i) {
	XML_Parser p;
	if (filenames[i] == NULL)
	    continue;

	p = XML_ParserCreate (NULL); /* use encoding specified by file */
	XML_SetElementHandler (p, optConfStartElem, optConfEndElem);
	XML_SetUserData (p, &userData);
	userData.parser = p;
	userData.name = filenames[i];
	userData.ignoringDevice = 0;
	userData.ignoringApp = 0;
	userData.inDriConf = 0;
	userData.inDevice = 0;
	userData.inApp = 0;
	userData.inOption = 0;

	parseOneConfigFile (p);
	XML_ParserFree (p);
    }

    free(filenames[1]);
}

void driDestroyOptionInfo (driOptionCache *info) {
    driDestroyOptionCache (info);
    if (info->info) {
	uint32_t i, size = 1 << info->tableSize;
	for (i = 0; i < size; ++i) {
	    if (info->info[i].name) {
		free(info->info[i].name);
		free(info->info[i].ranges);
	    }
	}
	free(info->info);
    }
}

void driDestroyOptionCache (driOptionCache *cache) {
    if (cache->info) {
	GLuint i, size = 1 << cache->tableSize;
	for (i = 0; i < size; ++i) {
	    if (cache->info[i].type == DRI_STRING)
		free(cache->values[i]._string);
	}
    }
    free(cache->values);
}

unsigned char driCheckOption (const driOptionCache *cache, const char *name,
			  driOptionType type) {
    uint32_t i = findOption (cache, name);
    return cache->info[i].name != NULL && cache->info[i].type == type;
}

unsigned char driQueryOptionb (const driOptionCache *cache, const char *name) {
    uint32_t i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_BOOL);
    return cache->values[i]._bool;
}

int driQueryOptioni (const driOptionCache *cache, const char *name) {
    uint32_t i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_INT || cache->info[i].type == DRI_ENUM);
    return cache->values[i]._int;
}

float driQueryOptionf (const driOptionCache *cache, const char *name) {
    uint32_t i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_FLOAT);
    return cache->values[i]._float;
}

char *driQueryOptionstr (const driOptionCache *cache, const char *name) {
    GLuint i = findOption (cache, name);
  /* make sure the option is defined and has the correct type */
    assert (cache->info[i].name != NULL);
    assert (cache->info[i].type == DRI_STRING);
    return cache->values[i]._string;
}
