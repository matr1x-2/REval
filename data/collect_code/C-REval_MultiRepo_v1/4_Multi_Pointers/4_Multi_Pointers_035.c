/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_035 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 21 */
/* Subsystem: scripts */
/* Includes
 * #include "dtc.h"
 * #include "srcpos.h"
 */
/* Context-Before
 * 	int i;
 * 
 * 	for (i = 0; i < len; i++) {
 * 		if (! isstring(p[i]))
 * 			nnotstring++;
 * 		if (p[i] == '\0')
 * 			nnul++;
 * 	}
 * 
 * 	if ((p[len-1] == '\0') && (nnotstring == 0) && (nnul <= len - nnul)) {
 * 		if (nnul > 1)
 * 			add_string_markers(prop, offset, len);
 * 		return TYPE_STRING;
 * 	} else if ((len % sizeof(cell_t)) == 0) {
 * 		return TYPE_UINT32;
 * 	}
 * 
 * 	return TYPE_UINT8;
 * }
 */
static void guess_type_markers(struct property *prop)
{
	struct marker **m = &prop->val.markers;
	unsigned int offset = 0;

	for (m = &prop->val.markers; *m; m = &((*m)->next)) {
		if (is_type_marker((*m)->type))
			/* assume the whole property is already marked */
			return;

		if ((*m)->offset > offset) {
			m = add_marker(m, guess_value_type(prop, offset, (*m)->offset - offset),
				       offset, NULL);

			offset = (*m)->offset;
		}

		if ((*m)->type == REF_PHANDLE) {
			m = add_marker(m, TYPE_UINT32, offset, NULL);
			offset += 4;
		}
	}

	if (offset < prop->val.len)
		add_marker(m, guess_value_type(prop, offset, prop->val.len - offset),
			   offset, NULL);
}
/* Context-After
 * static void write_propval(FILE *f, struct property *prop)
 * {
 * 	size_t len = prop->val.len;
 * 	struct marker *m;
 * 	enum markertype emit_type = TYPE_NONE;
 * 	char *srcstr;
 * 
 * 	if (len == 0) {
 * 		fprintf(f, ";");
 * 		if (annotate) {
 * 			srcstr = srcpos_string_first(prop->srcpos, annotate);
 * 			if (srcstr) {
 * 				fprintf(f, " /* %s */", srcstr);
 * 				free(srcstr);
 * 			}
 * 		}
 * 		fprintf(f, "\n");
 * 		return;
 * 	}
 */
