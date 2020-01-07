/*
 * uri.h
 *
 *  Created on: Sep 8, 2016
 *      Author: kel
 */

#ifndef URI_H_
#define URI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The system include files */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

const char* URIToNativePath(const char* uri);

#ifdef __cplusplus
}
#endif

#endif /* URI_H_ */
