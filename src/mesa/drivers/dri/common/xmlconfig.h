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
 * \file xmlconfig.h
 * \brief Driver-independent client-side part of the XML configuration
 * \author Felix Kuehling
 */

#ifndef __XMLCONFIG_H
#define __XMLCONFIG_H

/** \brief Option data types */
typedef enum driOptionType {
    DRI_BOOL, DRI_ENUM, DRI_INT, DRI_FLOAT
} driOptionType;

/** \brief Option value */
typedef union driOptionValue {
    GLboolean _bool; /**< \brief Boolean */
    GLint _int;      /**< \brief Integer or Enum */
    GLfloat _float;  /**< \brief Floating-point */
} driOptionValue;

/** \brief Single range of valid values
 *
 * For empty ranges (a single value) start == end */
typedef struct driOptionRange {
    driOptionValue start; /**< \brief Start */
    driOptionValue end;   /**< \brief End */
} driOptionRange;

/** \brief Information about an option */
typedef struct driOptionInfo {
    char *name;             /**< \brief Name */
    driOptionType type;     /**< \brief Type */
    driOptionRange *ranges; /**< \brief Array of ranges */
    GLuint nRanges;         /**< \brief Number of ranges */
} driOptionInfo;

/** \brief Option cache
 *
 * \li One in <driver>Screen caching option info and the default values
 * \li One in each <driver>Context with the actual values for that context */
typedef struct driOptionCache {
    driOptionInfo *info;
  /**< \brief Array of option infos
   *
   * Points to the same array in the screen and all contexts */
    driOptionValue *values;	
  /**< \brief Array of option values
   *
   * \li Default values in screen
   * \li Actual values in contexts 
   */
    GLuint tableSize;
  /**< \brief Size of the arrays
   *
   * Depending on the hash function this may differ from __driNConfigOptions.
   * In the current implementation it's not actually a size but log2(size).
   * The value is the same in the screen and all contexts. */
} driOptionCache;

/** \brief XML document describing available options
 *
 * This must be defined in a driver-specific soure file. xmlpool.h
 * defines helper macros and common options. */
extern const char __driConfigOptions[];
/** \brief The number of options supported by a driver
 *
 * This is used to choose an appropriate hash table size. So any value
 * larger than the actual number of options will work. */
extern const GLuint __driNConfigOptions;

/** \brief Parse XML option info from __driConfigOptions
 *
 * To be called in <driver>CreateScreen */
void driParseOptionInfo (driOptionCache *info);
/** \brief Initialize option cache from info and parse configuration files
 *
 * To be called in <driver>CreateContext. screenNum and driverName select
 * device sections. */
void driParseConfigFiles (driOptionCache *cache, driOptionCache *info,
			  GLint screenNum, const char *driverName);
/** \brief Destroy option info
 *
 * To be called in <driver>DestroyScreen */
void driDestroyOptionInfo (driOptionCache *info);
/** \brief Destroy option cache
 *
 * To be called in <driver>DestroyContext */
void driDestroyOptionCache (driOptionCache *cache);

/** \brief Check if there exists a certain option */
GLboolean driCheckOption (const driOptionCache *cache, const char *name,
			  driOptionType type);

/** \brief Query a boolean option value */
GLboolean driQueryOptionb (const driOptionCache *cache, const char *name);
/** \brief Query an integer option value */
GLint driQueryOptioni (const driOptionCache *cache, const char *name);
/** \brief Query a floating-point option value */
GLfloat driQueryOptionf (const driOptionCache *cache, const char *name);

#endif
