/* parse the command line */
#include "malt.h"
#include "config.h"
#include "define.h"
#include "margins.h"
#include "corners.h"
#include "optimize.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void read_command_line(int argc, char *argv[], int *function, char **command_name);

int main(int argc, char *argv[])
{
  /* print the following if malt args cannot be parsed */
  char *c_text = NULL;
  int function;
  char *command_name = NULL;

  /* parse command line */
  read_command_line(argc, argv, &function, &command_name);

  /* create output file */
  char *log_file_name = resprintf(NULL, "%s.%c", command_name, function); // suasively-fructiform-gunpapers
  FILE *log = fopen(log_file_name,"w");
  if (log == NULL) {
    char *err = resprintf(NULL, "malt: Can't open '%s' for writing", log_file_name);
    perror(err);
    exit(EXIT_FAILURE);
  }
  free(log_file_name); // suasively-fructiform-gunpapers

  /* find and parse the .config hierarchy of files */
  Configuration *C = Configure(command_name, function, &c_text, log);

  /* print the string generated by the config routine */
  if (C->options.verbose) {
    lprintf(C, "%s", c_text);
  }
  free(c_text); // articulately-scalelet-hoker
  lprintf(C,"# Configuration written to the %s.config.%c file\n",
          C->command, C->function);
  /* .cir */
  lprintf(C,"%sThe %s file: %s\n",
          (function == '2')?"# ":"",C->extensions.circuit,
          (*C->file_names.circuit != '\0')?C->file_names.circuit:"Not Found");
  /* does the .cir file exist? */
  if(*C->file_names.circuit == '\0') {
    fprintf(stderr, "The %s file is the spice deck and is required\n",
            C->extensions.circuit);
    exit(EXIT_FAILURE);
  }
  /* .param */
  if(*C->file_names.param != '\0')
    lprintf(C,"%sThe %s file: %s\n",
            (function == '2')?"# ":"",C->extensions.param, C->file_names.param);
  /* .passf */
  if(*C->file_names.passf != '\0')
    lprintf(C,"%sThe %s file: %s\n",
            (function == '2')?"# ":"",C->extensions.passf, C->file_names.passf);
  /* .envelope */
  if(function != 'd'){
    lprintf(C,"%sThe %s file: %s\n",
            (function == '2')?"# ":"",C->extensions.envelope,
            (*C->file_names.envelope != '\0')?C->file_names.envelope:"Not Found");
  }
  /* *** need to check if you can use only the passf file, because you still have to save your vectors *** */
  /* does either the .envelope or .passf file exist? */
  if(*C->file_names.envelope == '\0' && *C->file_names.passf == '\0' && function != 'd') {
    fprintf(stderr,
            "The %s file and %s file defines correct circuit operation. One or the other is required\n",
            C->extensions.envelope, C->extensions.passf);
    exit(EXIT_FAILURE);
  }
  /* call the appropriate algorithm */
  switch (function) {
  case 'd':
    if (!call_def(C))  fprintf(stderr,"Define routine exited on an error\n");
    break;
  case 'm':
    if (!call_marg(C))  fprintf(stderr,"Margins routine exited on an error\n");
    break;
  case 't':
    if (!call_trace(C))  fprintf(stderr,"Trace routine exited on an error\n");
    break;
  case '2':
    if (!margins2(C))  fprintf(stderr,"2D Margins routine exited on an error\n");
    break;
  case 's':
    if (!shmoo(C))  fprintf(stderr,"2D Shmoo routine exited on an error\n");
    break;
  case 'y':
    if (!marg_corners(C))  fprintf(stderr,"Yield routine exited on an error\n");
    break;
  case 'o':
    if (!call_opt(C))  fprintf(stderr,"Optimize routine exited on an error\n");
    break;
  }
  free(command_name); // mobster-irreviewable-redound
  freeConfiguration(C);
}

__attribute__((noreturn))
static void usage(void)
{
  fprintf(stderr, "Usage: %s", MALTUSAGE);
  exit(EXIT_FAILURE);
}

void read_command_line(int argc, char *argv[], int *function, char **command_name)
{
  int have_function = 0;
  int c;

  /* find options and arguments */
  while ((c = getopt(argc, argv, "hdmt2syo")) != -1) {
    /* we have a function option */
    switch (c) {
    case 'h':
      fprintf(stderr, "%s", MALTHELP);
      exit(EXIT_SUCCESS);
    case 'd': case 'm': case 't': case '2': case 's': case 'y': case 'o':
      if (have_function) {
        /* only one function is allowed per command line */
        usage();
      }
      *function = c;
      have_function = 1;
      break;
    case '?':
      usage();
    default:
      /* should not be possible */
      abort();
    }
  }

  /* one of the function options must be provided */
  if (!have_function) {
    usage();
  }

  if (optind + 1 == argc) {
    /* we have a circuit name */
    *command_name = strdup(argv[optind]); // mobster-irreviewable-redound
  } else {
    usage();
  }
}

void lprintf(const Configuration *C, const char *fmt, ...) {
  va_list args;
  // print to terminal
  if(C->options.print_terminal) {
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }

  // print to log file
  va_start(args, fmt);
  vfprintf(C->log, fmt, args);
  va_end(args);

  // flush logfile
  fflush(C->log);
}

/* Works like sprintf, but reallocates beforehand
 * Implementation adapted from:
 * https://stackoverflow.com/questions/4899221/substitute-or-workaround-for-asprintf-on-aix/4899487#4899487
 */
char *resprintf(char **restrict strp, const char *fmt, ...)
{
  va_list args;

  // calculate required size of buffer
  va_start(args, fmt);
  int reserved = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  assert(reserved >= 0);

  char *ret = NULL;
  /* if the passed pointer is NULL, allocate anyway using ret as a temporary pointee */
  if (strp == NULL) {
    strp = &ret;
  } else {
    ret = *strp;
  }

  // allocate space
  *strp = realloc(*strp, reserved + 1);
  assert(*strp != NULL);

  // populate buffer
  va_start(args, fmt);
  int copied = vsnprintf(*strp, reserved + 1, fmt, args);
  va_end(args);
  assert(copied == reserved);

  return ret;
}
