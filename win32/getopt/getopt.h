/**
 * getopt.h
 * A getopt reimplementation for Windows.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WIN32_GETOPT_H
#define _WIN32_GETOPT_H

#ifdef __cplusplus
extern "C" {
#endif

extern int opterr;   /* if error message should be printed */
extern int optind;   /* index into parent argv vector */
extern int optopt;   /* character checked for validity */
extern int optreset; /* reset getopt  */
extern char *optarg; /* argument associated with option */

int getopt(int nargc, char *const nargv[], const char *ostr);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32_GETOPT_H */
