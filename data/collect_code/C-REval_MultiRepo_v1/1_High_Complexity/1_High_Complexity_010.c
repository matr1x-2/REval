/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_010 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 37 */
/* NLOC: 165 */
/* Subsystem: scripts */
/* Includes
 * #include <stdarg.h>
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <stdint.h>
 * #include <stdbool.h>
 * #include <string.h>
 * #include <ctype.h>
 * #include <unistd.h>
 * #include <fcntl.h>
 * #include <sys/stat.h>
 * #include <linux/asn1_ber_bytecode.h>
 */
/* Context-Before
 * 	//debug("cmp(%s,%s) = ", token->content, dir);
 * 
 * 	val = memcmp(token->content, dir, clen);
 * 	if (val != 0) {
 * 		//debug("%d [cmp]\n", val);
 * 		return val;
 * 	}
 * 
 * 	if (dlen == token->size) {
 * 		//debug("0\n");
 * 		return 0;
 * 	}
 * 	//debug("%d\n", (int)dlen - (int)token->size);
 * 	return dlen - token->size; /* shorter -> negative */
 * }
 * 
 * /*
 *  * Tokenise an ASN.1 grammar
 *  */
 */
static void tokenise(char *buffer, char *end)
{
	struct token *tokens;
	char *line, *nl, *start, *p, *q;
	unsigned tix, lineno;

	/* Assume we're going to have half as many tokens as we have
	 * characters
	 */
	token_list = tokens = calloc((end - buffer) / 2, sizeof(struct token));
	if (!tokens) {
		perror(NULL);
		exit(1);
	}
	tix = 0;

	lineno = 0;
	while (buffer < end) {
		/* First of all, break out a line */
		lineno++;
		line = buffer;
		nl = memchr(line, '\n', end - buffer);
		if (!nl) {
			buffer = nl = end;
		} else {
			buffer = nl + 1;
			*nl = '\0';
		}

		/* Remove "--" comments */
		p = line;
	next_comment:
		while ((p = memchr(p, '-', nl - p))) {
			if (p[1] == '-') {
				/* Found a comment; see if there's a terminator */
				q = p + 2;
				while ((q = memchr(q, '-', nl - q))) {
					if (q[1] == '-') {
						/* There is - excise the comment */
						q += 2;
						memmove(p, q, nl - q);
						goto next_comment;
					}
					q++;
				}
				*p = '\0';
				nl = p;
				break;
			} else {
				p++;
			}
		}

		p = line;
		while (p < nl) {
			/* Skip white space */
			while (p < nl && isspace(*p))
				*(p++) = 0;
			if (p >= nl)
				break;

			tokens[tix].line = lineno;
			start = p;

			/* Handle string tokens */
			if (isalpha(*p)) {
				const char **dir;

				/* Can be a directive, type name or element
				 * name.  Find the end of the name.
				 */
				q = p + 1;
				while (q < nl && (isalnum(*q) || *q == '-' || *q == '_'))
					q++;
				tokens[tix].size = q - p;
				p = q;

				tokens[tix].content = malloc(tokens[tix].size + 1);
				if (!tokens[tix].content) {
					perror(NULL);
					exit(1);
				}
				memcpy(tokens[tix].content, start, tokens[tix].size);
				tokens[tix].content[tokens[tix].size] = 0;
				
				/* If it begins with a lowercase letter then
				 * it's an element name
				 */
				if (islower(tokens[tix].content[0])) {
					tokens[tix++].token_type = TOKEN_ELEMENT_NAME;
					continue;
				}

				/* Otherwise we need to search the directive
				 * table
				 */
				dir = bsearch(&tokens[tix], directives,
					      sizeof(directives) / sizeof(directives[1]),
					      sizeof(directives[1]),
					      directive_compare);
				if (dir) {
					tokens[tix++].token_type = dir - directives;
					continue;
				}

				tokens[tix++].token_type = TOKEN_TYPE_NAME;
				continue;
			}

			/* Handle numbers */
			if (isdigit(*p)) {
				/* Find the end of the number */
				q = p + 1;
				while (q < nl && (isdigit(*q)))
					q++;
				tokens[tix].size = q - p;
				p = q;
				tokens[tix].content = malloc(tokens[tix].size + 1);
				if (!tokens[tix].content) {
					perror(NULL);
					exit(1);
				}
				memcpy(tokens[tix].content, start, tokens[tix].size);
				tokens[tix].content[tokens[tix].size] = 0;
				tokens[tix++].token_type = TOKEN_NUMBER;
				continue;
			}

			if (nl - p >= 3) {
				if (memcmp(p, "::=", 3) == 0) {
					p += 3;
					tokens[tix].size = 3;
					tokens[tix].content = "::=";
					tokens[tix++].token_type = TOKEN_ASSIGNMENT;
					continue;
				}
			}

			if (nl - p >= 2) {
				if (memcmp(p, "({", 2) == 0) {
					p += 2;
					tokens[tix].size = 2;
					tokens[tix].content = "({";
					tokens[tix++].token_type = TOKEN_OPEN_ACTION;
					continue;
				}
				if (memcmp(p, "})", 2) == 0) {
					p += 2;
					tokens[tix].size = 2;
					tokens[tix].content = "})";
					tokens[tix++].token_type = TOKEN_CLOSE_ACTION;
					continue;
				}
			}

			if (nl - p >= 1) {
				tokens[tix].size = 1;
				switch (*p) {
				case '{':
					p += 1;
					tokens[tix].content = "{";
					tokens[tix++].token_type = TOKEN_OPEN_CURLY;
					continue;
				case '}':
					p += 1;
					tokens[tix].content = "}";
					tokens[tix++].token_type = TOKEN_CLOSE_CURLY;
					continue;
				case '[':
					p += 1;
					tokens[tix].content = "[";
					tokens[tix++].token_type = TOKEN_OPEN_SQUARE;
					continue;
				case ']':
					p += 1;
					tokens[tix].content = "]";
					tokens[tix++].token_type = TOKEN_CLOSE_SQUARE;
					continue;
				case ',':
					p += 1;
					tokens[tix].content = ",";
					tokens[tix++].token_type = TOKEN_COMMA;
					continue;
				default:
					break;
				}
			}

			fprintf(stderr, "%s:%u: Unknown character in grammar: '%c'\n",
				filename, lineno, *p);
			exit(1);
		}
	}

	nr_tokens = tix;
	verbose("Extracted %u tokens\n", nr_tokens);

#if 0
	{
		int n;
		for (n = 0; n < nr_tokens; n++)
			debug("Token %3u: '%s'\n", n, token_list[n].content);
	}
#endif
}
/* Context-After
 * static void build_type_list(void);
 * static void parse(void);
 * static void dump_elements(void);
 * static void render(FILE *out, FILE *hdr);
 * 
 * /*
 *  *
 *  */
 * int main(int argc, char **argv)
 * {
 * 	struct stat st;
 * 	ssize_t readlen;
 * 	FILE *out, *hdr;
 * 	char *buffer, *p;
 * 	char *kbuild_verbose;
 * 	int fd;
 * 
 * 	kbuild_verbose = getenv("KBUILD_VERBOSE");
 * 	if (kbuild_verbose && strchr(kbuild_verbose, '1'))
 */
