/*
 ccd2cue.c -- convert CCD sheet to CUE sheet;

 Copyright (C) 2010, 2013 Bruno Félix Rezende Ribeiro
 <oitofelix@gnu.org>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file       ccd2cue.c
 * \brief      convert CCD sheet to CUE sheet
 * \author     Bruno Félix Rezende Ribeiro (_oitofelix_)
 *             <oitofelix@gnu.org>
 * \date       2013
 * \version    0.2
 *
 * \copyright [GNU General Public License (version 3 or later)]
 *            (http://www.gnu.org/licenses/gpl.html)
 *
 * ~~~
 * This file is part of GNU ccd2cue.
 *
 * GNU ccd2cue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNU ccd2cue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ~~~
 *
 * \mainpage notitle
 *
 * # Introduction #
 *
 * On the internet there is a gigantic quantity of optical disc image
 * files in numerous formats.  Countless times we need to burn some of
 * them.  Some time ago I needed it, but I came across a file format
 * extremely irritating for a Free Software user like me: a CD layout
 * descriptor file, with the CCD sufix, generated by a proprietary
 * software called CloneCD.  I searched the internet for a way to burn
 * that file on GNU+Linux-Libre system, but I only found a lot of
 * people asking for a solution on a lot of forums, and getting the
 * unanimous answer: no way!  In the first I could not believe that at
 * this point there was no option.  Then, with a little bit of
 * patience and research, I wrote a code to convert that files to a
 * format much more common and accessible, an ad-hoc standard in GNU
 * operating system: the CUE Sheet format.  So I could burn a lot of
 * what I wanted!  I wondered whether it would be useful for others...
 * And here is the result!
 *
 */


#define _GNU_SOURCE

#include <config.h>

/* Assume ANSI C89 headers are available.  */
#include <stddef.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

/* Use POSIX headers. */
#include <argp.h>
#include <sysexits.h>

/* ccd2cue headers. */
#include "i18n.h"
#include "convert.h"
#include "io.h"
#include "file.h"
#include "ccd.h"
#include "cue.h"
#include "cdt.h"
#include "array.h"
#include "errors.h"


/* Forward declarations. */
static error_t parse_opt (int, char *, struct argp_state *);
static void print_version (FILE *, struct argp_state *);
static char * help_filter (int, const char *, void *);


/**
 * Argp `--version` option customization.
 *
 * This hook is called by Argp command line option processing engine to customize
 * arbitrarily sections of `--version` option output.
 *
 * \sa [Argp Global Variables] (https://gnu.org/software/libc/manual/html_node/Argp-Global-Variables.html#Argp-Global-Variables)
 *
 * \attention Maybe it will be necessary (or better) to use this hook
 * to add i18n support to that output.
 *
 */

void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/**
 * Command line options.
 *
 * This structure store the data that Argp will use to construct
 * `--help` option output and option process logic.  The function
 * ::parse_opt is the Argp parse function that will deal with it.
 *
 * The options are organized in groups and each group has a header and
 * a documentation very after the respective options.
 *
 * \sa [Argp Option Vectors] (https://gnu.org/software/libc/manual/html_node/Argp-Option-Vectors.html#Argp-Option-Vectors
 *
 */

static struct argp_option options[] = {
  { NULL, 0, NULL, 0, __("Output:"), 0 },
  { "output", 'o', "cue-file", 0, __("write output to `cue-file'"), 0 },
  { "cd-text", 'c', "cdt-file", 0, __("write CD-Text data to `cdt-file'"), 0 },
  { NULL, 0, NULL, OPTION_DOC, __("While the main output file `cue-file' is always generated, the `cdt-file' is created only when there is CD-Text data.  If `cue-file' is `-', or `--output' is omitted, standard output is used."), 0 },
  { NULL, 0, NULL, 0, __("Conversion:"), 0 },
  { "image", 'i', "img-file", 0, __("reference `img-file' as the image file"), 0 },
  { "absolute-file-name", 'a', NULL, 0, __("use absolute file name deduction"), 0 },
  { NULL, 0, NULL, OPTION_DOC, __("The `img-file' is a reference to a data file required only in burning time and thus its existence is not enforced in conversion stage."), 0 },
  { NULL, 0, NULL, 0, __("Help:"), -1},
  { 0 }
};

/**
 * The Argp parser.
 *
 * This data structure maintain virtually all the information needed
 * to make Argp to parse the command line properly.
 *
 * \sa [Argp Parsers] (https://gnu.org/software/libc/manual/html_node/Argp-Parsers.html#Argp-Parsers)
 *
 */

struct argp argp = { options, parse_opt, "[ccd-file]", NULL, NULL, help_filter, NULL };

/**
 * Post-Argp, processed command line arguments and correlate, helper,
 * data.
 *
 * An instance of this structure is used by the Argp parser function
 * ::parse_opt, to store the relevant command line options arguments,
 * and the only non-option argument allowed.  When some file name
 * arguments is needed but not supplied, it also stores the deduction
 * for them based on another supplied argument.
 *
 * This structure also store the streams to the input and output
 * files.
 *
 * Refer to the ::options variable to the correlate option argument's
 * constraints.
 *
 * \public
 *
 **/

struct arguments
{
  const char *img_name;		/**< `--image` argument. */
  const char *cdt_name;		/**< `--cd-text` argument. */
  const char *ccd_name;		/**< The only non-option argument; the
				   input CCD sheet file name. */
  const char *cue_name; 	/**< The output file name; the input
				   CUE sheet file name. */
  int abs_fname_flag;		/**< Boolean. True if, and only if,
				   `--absolute-file-name` is
				   supplied. */
  const char *reference_name; 	/**< Reference name to deduce the
				   possible needed but not supplied
				   file names.  \sa
				   make_reference_name */
  FILE *cue_stream; /**< CUE sheet input stream.  Opened by ::parse_opt. */
  FILE *ccd_stream; /**< CCD sheet output stream.  Opened by ::parse_opt. */

};


/**
 * Main entry point;
 *
 * \param[in] argc  Number of command line arguments;
 * \param[in] argv  Vector of individual command line arguments;
 *
 * Program's execution begins here.
 *
 * \sa [Program Arguments] (https://gnu.org/software/libc/manual/html_node/Program-Arguments.html#Program-Arguments)
 */

int
main (int argc, char *argv[])
{
  struct arguments arguments;  /* Structure passed to ::argp to
				  contain the processed command line
				  and some helper data; */

  FILE *cdt_stream;   /* CDT stream that will be opened if there are
			 CDText information on the input CUE sheet */

  struct ccd ccd;   /* CCD structure filled by stream2ccd; */
  struct cue *cue;  /* Pointer to CUE structure filled by ccd2cue; */
  struct cdt cdt;   /* CDT structure filled by ccd2cdt; */

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* Parse command line arguments */
  error_t argp_retval = argp_parse (&argp, argc, argv, 0, 0, &arguments);
  assert (argp_retval == 0);

  /* Try to optimize CCD input and CUE output streams. */
  io_optimize_stream_buffer (arguments.ccd_stream, _IOLBF);
  io_optimize_stream_buffer (arguments.cue_stream, _IOLBF);

  /* Parse the CCD sheet input into a CCD structure. */
  if (stream2ccd (arguments.ccd_stream, &ccd) < 0)
    error_pop (EX_DATAERR, _("cannot parse CCD sheet stream from `%s'"), arguments.ccd_name);

  /* Convert the CCD structure into a CUE structure. */
  cue = ccd2cue (&ccd, arguments.img_name, arguments.cdt_name);
  if (cue == NULL)
    error_pop (EX_SOFTWARE, _("cannot convert `%s' to `%s'"),
	       arguments.ccd_name, arguments.cue_name);

  /* Convert the CD-Text data in the CCD structure into a CDT
     structure.  */
  if (ccd2cdt (&ccd, &cdt) > 0)
    {
      /* Convert the CDT structure into a CD-Text binary file. */
      cdt_stream = fopen (arguments.cdt_name, "w");
      cdt2stream (&cdt, cdt_stream);
      if (fclose (cdt_stream) == EOF)
	error (EX_IOERR, errno, _("cannot close `%s'"), arguments.cdt_name);
    }

  /* Convert the CUE structure into the CUE sheet output. */
  if (cue2stream (cue, arguments.cue_stream) < 0)
    error_pop (EX_SOFTWARE, _("cannot convert `%s' to `%s'"),
	       arguments.ccd_name, arguments.cue_name);

  /* Close the CCD sheet input and the CUE sheet output streams. */
  if (fclose (arguments.ccd_stream) == EOF) error (EX_IOERR, errno,
					 _("cannot close `%s'"), arguments.ccd_name);
  if (fclose (arguments.cue_stream) == EOF) error (EX_IOERR, errno,
					 _("cannot close `%s'"), arguments.cue_name);

  /* Exit with success. */
  return 0;
}

/**
 * \fn static error_t parse_opt (int key, char *arg, struct argp_state *state)
 *
 * Argp parser function
 *
 * This function is registered in ::argp and it comes into play when
 * [argp_parse][Argp] gets called into ::main to process the
 * command line's options and arguments.  One can find a general
 * documentation about its parameters and how a Argp parser function
 * works on general grounds at [Argp Parser Functions].  Here is only
 * the specific documentation.
 *
 * [Argp]: https://gnu.org/software/libc/manual/html_node/Argp.html#Argp
 * [Argp Parser Functions]: https://gnu.org/software/libc/manual/html_node/Argp-Parser-Functions.html#Argp-Parser-Functions
 *
 * The basic purpose of this function is to fill out the main's
 * \ref arguments instance based on options and arguments provided in the
 * command line.  Afterwards we will serve ourselves with the language
 * abuse "arguments structure" to refer to the main function's
 * instance of it. In the touching of `KEY` arguments the following
 * holds:
 *
 * - When `KEY` is `ARGP_KEY_INIT`, namely before any command line
 *   option is parsed, the \ref arguments structure has its pointers
 *   members initialized to NULL and its booleans members to false.
 *
 * - If `--image`, `--cd-text` or `--output` options are supplied, its
 *   arguments are put in the respective fields on \ref arguments
 *   structure, i.e., [img_name], [cdt_name] and [cue_name],
 *   respectively.  If there isn't argument for any of those options
 *   or they are specified more than once an error is raised.
 *
 *   [img_name]: \ref arguments.img_name
 *   [cdt_name]: \ref arguments.cdt_name
 *   [cue_name]: \ref arguments.cue_name
 *
 * - If `--absolute-file-name` is supplied the respective flag in
 *   \ref arguments structure are marked as so, i.e., true.
 *
 * - If `KEY` is `ARGP_KEY_ARG`, namely when a non-option argument is
 *    supplied, that argument is put in [ccd_name] on \ref arguments
 *    structure.  If there is more than one non-option argument an
 *    error is raised.
 *
 *   [ccd_name]: \ref arguments.ccd_name
 *
 * - If `KEY` is `ARGP_KEY_NO_ARGS`, then it means that no non-option
 *   arguments were supplied and thus there isn't any input CCD sheet
 *   specified.  When it happens the standard input is in place.  So,
 *   the [ccd_name] and [ccd_stream] \ref arguments' members are
 *   pointed to a string like "stdin" and stream `stdin`,
 *   respectively.
 *
 *   [ccd_stream]: \ref arguments.ccd_stream
 *
 * - When `KEY` is `ARGP_KEY_END` there is no more command line
 *   arguments to parse, and if there is no provided argument in
 *   command line an error is raised.
 *
 * - When `KEY` is `ARG_KEY_SUCESS`, all options and non-options
 *   arguments were successfully parsed and some procedures are taken
 *   to make sure that \ref arguments structure gets in a consistent
 *   state in order to resume main function's execution properly.  The
 *   following steps are fulfilled:
 *
 *   + If one of the options that has a file name as an argument are
 *     not supplied, the function ::make_reference_name is called with
 *     a supplied name as an argument to derive a name to construct
 *     the names not supplied but possibly necessary.  The preference
 *     of names to use as template to the reference name is sorted as
 *     follow:
 *
 *     1. The CCD sheet name; the only non-option argument; the input;
 *        [ccd_name].
 *
 *     2. The CUE sheet name; the argument of `--output` option; the
 *        output; [cue_name].
 *
 *     3. The disc image name; the argument of `--image` option;
 *        [img_name].
 *
 *     4. The CD-Text info file name; the argument of `--cd-text`; an
 *     output; [cdt_name].
 *
 *     If none of those are provided on command line an error is
 *     raised.
 *
 *   + If the arguments related to input and output &mdash; the former
 *     being the only accepted non-option argument, and the last being
 *     the argument of `--output` &mdash; are a dash "-", the standard
 *     input and output are placed in [ccd_stream] and [cue_stream],
 *     respectively, with strings like "stdin" and "stdout" in
 *     [ccd_name] and [cue_name] members of \ref arguments structure.
 *
 *     [cue_stream]: \ref arguments.cue_stream
 *
 * To get the user's visible consequences of these rules, take a look
 * at the `--help` output or, equivalently, at ::argp and ::options
 * global variables.
 *
 * \sa [Argp]
 * \sa [Argp Parser Functions] (https://gnu.org/software/libc/manual/html_node/Argp-Parser-Functions.html#Argp-Parser-Functions)
 *
 * \since 0.2
 */

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = state->input;
  assert (arguments != NULL);

  switch (key)
    {
      case ARGP_KEY_INIT:	/* Prepare to parse the command line arguments. */
	/* Initialize arguments structure.  */
        arguments->img_name = NULL;
	arguments->cdt_name = NULL;
	arguments->ccd_name = NULL;
	arguments->cue_name = NULL;
	arguments->abs_fname_flag = 0;
	arguments->reference_name = NULL;
	arguments->cue_stream = NULL;
	arguments->ccd_stream = NULL;
	break;
      case 'i': 		/* `--image' option supplied. */
	/* If it was already supplied raise an error. */
	if (arguments->img_name != NULL)
	   argp_error
	     (state, _("more than one disc image file name provided (`--image'): `%s' and `%s'"),
	       arguments->img_name, arg);
	/* Save the argument supplied. */
	arguments->img_name = arg;
        break;
      case 'c':			/* `--cd-text' option supplied. */
	/* If it was already supplied raise an error. */
	if (arguments->cdt_name != NULL)
	  argp_error
	    (state, _("more than one CD-Text file name provided (`--cd-text'): `%s' and `%s'"),
	     arguments->cdt_name, arg);
	/* Save the argument supplied. */
	arguments->cdt_name = arg;
        break;
      case 'o':			/* `--output' option supplied. */
	/* If it was already supplied raise an error. */
	if (arguments->cue_name != NULL)
	  argp_error
	    (state, _("more than one output CUE sheet provided (`--output'): `%s' and `%s'"),
	     arguments->cue_name, arg);
	/* Save the argument supplied. */
	arguments->cue_name = arg;
        break;
      case 'a': 		/* `--absolute-file-name' option supplied. */
        arguments->abs_fname_flag = 1;
	break;
      case ARGP_KEY_ARG: 	/* A non-option argument supplied. */
	/* If more than one argument was supplied raise an error. */
	if (state->arg_num > 0)
	  argp_error (state, _("%s: more than one input CCD sheet provided: `%s' and `%s'"),
	  	      __func__, arguments->ccd_name, arg);
	/* Save the argument supplied. */
	arguments->ccd_name = arg;
	break;
      case ARGP_KEY_NO_ARGS:	/* No non-option arguments supplied. */
	/* Assume that CCD sheet yield from standard input. */
        break;
      case ARGP_KEY_END:	/* There is no more arguments to parse. */
	/* If no arguments were supplied outputs an usage message. */
        if (state->argc == 1) argp_usage (state);
	break;
      case ARGP_KEY_SUCCESS:	/* All supplied command line arguments
				   were successfully processed. */

	/* Make the reference name: */

	/* If CCD sheet file name was supplied use it to make the
	   reference name. */
	if (arguments->ccd_name != NULL && strcmp (arguments->ccd_name, "-"))
	  arguments->reference_name =
	    make_reference_name (arguments->ccd_name, arguments->abs_fname_flag);
	/* Else, if CUE sheet file name was supplied use it to make the
	   reference name. */
	else if (arguments->cue_name != NULL && strcmp (arguments->cue_name, "-"))
	  arguments->reference_name =
	    make_reference_name (arguments->cue_name, arguments->abs_fname_flag);
	/* Else, if disc image file name was supplied use it to make the
	   reference name. */
	else if (arguments->img_name != NULL)
	  arguments->reference_name =
	    make_reference_name (arguments->img_name, arguments->abs_fname_flag);
	/* Else, if CD-Text info file name was supplied use it to make the
	   reference name. */
	else if (arguments->cdt_name != NULL)
	  arguments->reference_name =
	    make_reference_name (arguments->cdt_name, arguments->abs_fname_flag);
	/* If none of the above file names were supplied raise an error. */
	else
	  argp_error (state, _("%s: no image name provided (`--image')"), __func__);

	/* If occurred any failure determining the reference name
	   raise an error. */
	if (arguments->reference_name == NULL) error_fatal_pop
       	  (EX_OSERR, _("cannot process command line arguments"));

	/* Deduce the disc image name if it was not supplied. */
	if (arguments->img_name == NULL)
	  arguments->img_name = concat (arguments->reference_name, ".img", NULL);
	/* If occurred any failure deducing the disc image name
	   raise an error. */
	if (arguments->img_name == NULL)
	  error_fatal_pop (EX_OSERR, _("cannot deduce image file name"));

	/* Deduce the CD-Text info file name if it was not supplied. */
	if (arguments->cdt_name == NULL)
	  arguments->cdt_name = concat (arguments->reference_name, ".cdt", NULL);
	/* If occurred any failure deducing the CD-Text info file name
	   raise an error. */
	if (arguments->cdt_name == NULL)
	  error_fatal_pop (EX_OSERR, _("cannot deduce CD-Text file name"));

	/* If the user specified `-' as CCD sheet's name, mark it to
	   read from standard input. */
	if (arguments->ccd_name == NULL || ! strcmp (arguments->ccd_name, "-"))
	  {
	    arguments->ccd_stream = stdin;
	    arguments->ccd_name = "stdin";
	  }
	/* If not, open the CCD sheet file specified for reading.
	   Raise an error if it is not possible to do so.*/
	else
	  {
	    arguments->ccd_stream = fopen (arguments->ccd_name, "r");
	    if (arguments->ccd_stream == NULL)
	      error_fatal_pop_lib (fopen, EX_NOINPUT,
	      		            _("cannot open CCD sheet `%s'"),
				   arguments->ccd_name);
	  }

	/* If the user specified `-' as CUE sheet's name or not
	   specified its name at all, mark it to write to standard
	   output. */
	if (arguments->cue_name == NULL || ! strcmp (arguments->cue_name, "-"))
	  {
	    arguments->cue_stream = stdout;
	    arguments->cue_name = "stdout";
	  }
	/* If not, open the CUE sheet file specified for writing.
	   Raise an error if it is not possible to do so.*/
	else
	  {
	    arguments->cue_stream = fopen (arguments->cue_name, "w");
	    if (arguments->cue_stream == NULL)
	      error_fatal_pop_lib (fopen, EX_CANTCREAT,
	      		            _("cannot open CUE sheet `%s'"),
				   arguments->cue_name);
	  }

        break;
      default:			/* KEY value not recognized. */
        return ARGP_ERR_UNKNOWN;
    }

  return 0;
}


/* Print version and copyright information.  */

static void
print_version (FILE *stream, struct argp_state *state)
{
  xfprintf (stream, "%s (%s) %s\n", PACKAGE, PACKAGE_NAME, VERSION);
  xputc ('\n', stream);
  xfprintf(stream,
  "Copyright (C) %s Bruno Félix Rezende Ribeiro <%s>\n\n",
	   "2010, 2013", "oitofelix@gnu.org");
  fputs (_("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"), stream);
  xputc ('\n', stream);
  fputs (_("Written by Bruno Félix Rezende Ribeiro."), stream);
  xputc ('\n', stream);

  exit (EXIT_SUCCESS);
}


static char *
help_filter (int key, const char *text, void *input)
{
  char *newtext;

  switch (key)
    {
      case ARGP_KEY_HELP_PRE_DOC:
	asprintf(&newtext, "%s\n\n%s",
		 _("Convert CCD sheet to CUE sheet."),
		 _("The input file, referred as `ccd-file', must exist.  If `ccd-file' is \
`-', or omitted, standard input is used.  It is necessary to supply at \
least one file name, in an option or non-option argument, in order to \
deduce the remaining file names needed, and only one file name of each \
type can be supplied.  The following options are accepted:"));
	break;
      case ARGP_KEY_HELP_EXTRA:
        asprintf(&newtext,
		 "%s\n\n"	/* Examples: */
		 "%s\n\n"	/* The most... */
		 "  %s\n\n"	/* ccd2cue -o... */
		 "%s\n\n"	/* Burning: */
		 "%s\n\n"	/* If you burned... */
		 "%s <%s>\n"	/* Report bugs at: <%s>*/
		 "%s <%s>\n"	/* Report translation bugs to: <%s> */,
		 _("Examples:"),
		 _("The most ordinary use case is when you have a CCD set of files and \
just want to generate a CUE sheet file in order to burn or otherwise \
access the data inside the image file.  Supposing your CCD sheet file \
is called gnu.ccd, you are done with the command:"),
		 _("ccd2cue -o gnu.cue gnu.ccd"),
		 _("Burning:"),
		 _("If you are willing to burn or already burned a CD from a CUE sheet \
produced by this program and all audio tracks became only senseless \
static noise, you may need to ask your burning software to swap the \
byte order of all samples sent to the CD-recorder when writing audio \
tracks.  For instance, that can be accomplished with the `--swap' \
option when using the cdrdao program.  Be warned that at least for \
mixed-mode discs, by author's own experience, the rule is to use that \
option; if you fail to specify it when burning, you almost certainly \
will pointless waste your CD."),
		 _("Report bugs at:"),
		 PACKAGE_BUGREPORT,
		 _("Report translation bugs to:"),
		 "http://translationproject.org/team/");
        break;
    }

  return newtext;
}
