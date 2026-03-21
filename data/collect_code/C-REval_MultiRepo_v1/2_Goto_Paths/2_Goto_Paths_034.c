/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_034 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 39 */
/* NLOC: 142 */
/* Subsystem: tools */
/* Includes
 * #include "builtin.h"
 * #include "util/counts.h"
 * #include "util/debug.h"
 * #include "util/dso.h"
 * #include <subcmd/exec-cmd.h>
 * #include "util/header.h"
 * #include <subcmd/parse-options.h>
 * #include "util/perf_regs.h"
 * #include "util/session.h"
 * #include "util/tool.h"
 * #include "util/map.h"
 * #include "util/srcline.h"
 * #include "util/symbol.h"
 * #include "util/thread.h"
 * #include "util/trace-event.h"
 * #include "util/env.h"
 * #include "util/evlist.h"
 * #include "util/evsel.h"
 * #include "util/evsel_fprintf.h"
 * #include "util/evswitch.h"
 */
/* Context-Before
 * 		script = str;
 * 		ext = strrchr(script, '.');
 * 		if (!ext) {
 * 			fprintf(stderr, "invalid script extension");
 * 			return -1;
 * 		}
 * 		scripting_ops = script_spec__lookup(++ext);
 * 		if (!scripting_ops) {
 * 			fprintf(stderr, "invalid script extension");
 * 			return -1;
 * 		}
 * 	}
 * 
 * 	script_name = find_script(script);
 * 	if (!script_name)
 * 		script_name = strdup(script);
 * 
 * 	return 0;
 * }
 */
static int parse_output_fields(const struct option *opt __maybe_unused,
			    const char *arg, int unset __maybe_unused)
{
	char *tok, *strtok_saveptr = NULL;
	int i, imax = ARRAY_SIZE(all_output_options);
	int j;
	int rc = 0;
	char *str = strdup(arg);
	int type = -1;
	enum { DEFAULT, SET, ADD, REMOVE } change = DEFAULT;

	if (!str)
		return -ENOMEM;

	/* first word can state for which event type the user is specifying
	 * the fields. If no type exists, the specified fields apply to all
	 * event types found in the file minus the invalid fields for a type.
	 */
	tok = strchr(str, ':');
	if (tok) {
		*tok = '\0';
		tok++;
		if (!strcmp(str, "hw"))
			type = PERF_TYPE_HARDWARE;
		else if (!strcmp(str, "sw"))
			type = PERF_TYPE_SOFTWARE;
		else if (!strcmp(str, "trace"))
			type = PERF_TYPE_TRACEPOINT;
		else if (!strcmp(str, "raw"))
			type = PERF_TYPE_RAW;
		else if (!strcmp(str, "break"))
			type = PERF_TYPE_BREAKPOINT;
		else if (!strcmp(str, "synth"))
			type = OUTPUT_TYPE_SYNTH;
		else {
			fprintf(stderr, "Invalid event type in field string.\n");
			rc = -EINVAL;
			goto out;
		}

		if (output[type].user_set)
			pr_warning("Overriding previous field request for %s events.\n",
				   event_type(type));

		/* Don't override defaults for +- */
		if (strchr(tok, '+') || strchr(tok, '-'))
			goto parse;

		output[type].fields = 0;
		output[type].user_set = true;
		output[type].wildcard_set = false;

	} else {
		tok = str;
		if (strlen(str) == 0) {
			fprintf(stderr,
				"Cannot set fields to 'none' for all event types.\n");
			rc = -EINVAL;
			goto out;
		}

		/* Don't override defaults for +- */
		if (strchr(str, '+') || strchr(str, '-'))
			goto parse;

		if (output_set_by_user())
			pr_warning("Overriding previous field request for all events.\n");

		for (j = 0; j < OUTPUT_TYPE_MAX; ++j) {
			output[j].fields = 0;
			output[j].user_set = true;
			output[j].wildcard_set = true;
		}
	}

parse:
	for (tok = strtok_r(tok, ",", &strtok_saveptr); tok; tok = strtok_r(NULL, ",", &strtok_saveptr)) {
		if (*tok == '+') {
			if (change == SET)
				goto out_badmix;
			change = ADD;
			tok++;
		} else if (*tok == '-') {
			if (change == SET)
				goto out_badmix;
			change = REMOVE;
			tok++;
		} else {
			if (change != SET && change != DEFAULT)
				goto out_badmix;
			change = SET;
		}

		for (i = 0; i < imax; ++i) {
			if (strcmp(tok, all_output_options[i].str) == 0)
				break;
		}
		if (i == imax && strcmp(tok, "flags") == 0) {
			print_flags = change != REMOVE;
			continue;
		}
		if (i == imax) {
			fprintf(stderr, "Invalid field requested.\n");
			rc = -EINVAL;
			goto out;
		}
#ifndef HAVE_LIBCAPSTONE_SUPPORT
		if (change != REMOVE && strcmp(tok, "disasm") == 0) {
			fprintf(stderr, "Field \"disasm\" requires perf to be built with libcapstone support.\n");
			rc = -EINVAL;
			goto out;
		}
#endif

		if (type == -1) {
			/* add user option to all events types for
			 * which it is valid
			 */
			for (j = 0; j < OUTPUT_TYPE_MAX; ++j) {
				if (output[j].invalid_fields & all_output_options[i].field) {
					pr_warning("\'%s\' not valid for %s events. Ignoring.\n",
						   all_output_options[i].str, event_type(j));
				} else {
					if (change == REMOVE) {
						output[j].fields &= ~all_output_options[i].field;
						output[j].user_set_fields &= ~all_output_options[i].field;
						output[j].user_unset_fields |= all_output_options[i].field;
					} else {
						output[j].fields |= all_output_options[i].field;
						output[j].user_set_fields |= all_output_options[i].field;
						output[j].user_unset_fields &= ~all_output_options[i].field;
					}
					output[j].user_set = true;
					output[j].wildcard_set = true;
				}
			}
		} else {
			if (output[type].invalid_fields & all_output_options[i].field) {
				fprintf(stderr, "\'%s\' not valid for %s events.\n",
					 all_output_options[i].str, event_type(type));

				rc = -EINVAL;
				goto out;
			}
			if (change == REMOVE)
				output[type].fields &= ~all_output_options[i].field;
			else
				output[type].fields |= all_output_options[i].field;
			output[type].user_set = true;
			output[type].wildcard_set = true;
		}
	}

	if (type >= 0) {
		if (output[type].fields == 0) {
			pr_debug("No fields requested for %s type. "
				 "Events will not be displayed.\n", event_type(type));
		}
	}
	goto out;

out_badmix:
	fprintf(stderr, "Cannot mix +-field with overridden fields\n");
	rc = -EINVAL;
out:
	free(str);
	return rc;
}
/* Context-After
 * #define for_each_lang(scripts_path, scripts_dir, lang_dirent)		\
 * 	while ((lang_dirent = readdir(scripts_dir)) != NULL)		\
 * 		if ((lang_dirent->d_type == DT_DIR ||			\
 * 		     (lang_dirent->d_type == DT_UNKNOWN &&		\
 * 		      is_directory(scripts_path, lang_dirent))) &&	\
 * 		    (strcmp(lang_dirent->d_name, ".")) &&		\
 * 		    (strcmp(lang_dirent->d_name, "..")))
 * 
 * #define for_each_script(lang_path, lang_dir, script_dirent)		\
 * 	while ((script_dirent = readdir(lang_dir)) != NULL)		\
 * 		if (script_dirent->d_type != DT_DIR &&			\
 * 		    (script_dirent->d_type != DT_UNKNOWN ||		\
 * 		     !is_directory(lang_path, script_dirent)))
 * 
 * 
 * #define RECORD_SUFFIX			"-record"
 * #define REPORT_SUFFIX			"-report"
 * 
 * struct script_desc {
 */
