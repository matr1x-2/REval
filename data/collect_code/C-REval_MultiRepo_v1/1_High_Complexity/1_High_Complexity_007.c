/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_007 */
/* Category: 1_High_Complexity */
/* Repo: musl */
/* Cyclomatic Complexity: 54 */
/* NLOC: 234 */
/* Subsystem: src */
/* Includes
 * #include <stdlib.h>
 * #include <string.h>
 * #include <wchar.h>
 * #include <wctype.h>
 * #include <limits.h>
 * #include <stdint.h>
 * #include <regex.h>
 * #include "tre.h"
 * #include <assert.h>
 */
/* Context-Before
 *   The worst case time required for finding the leftmost and longest
 *   match, or determining that there is no match, is always linearly
 *   dependent on the length of the text being searched.
 * 
 *   This algorithm cannot handle TNFAs with back referencing nodes.
 *   See `tre-match-backtrack.c'.
 * */
 * 
 * typedef struct {
 *   tre_tnfa_transition_t *state;
 *   regoff_t *tags;
 * } tre_tnfa_reach_t;
 * 
 * typedef struct {
 *   regoff_t pos;
 *   regoff_t **tags;
 * } tre_reach_pos_t;
 * 
 * 
 * static reg_errcode_t
 */
tre_tnfa_run_parallel(const tre_tnfa_t *tnfa, const void *string,
		      regoff_t *match_tags, int eflags,
		      regoff_t *match_end_ofs)
{
  /* State variables required by GET_NEXT_WCHAR. */
  tre_char_t prev_c = 0, next_c = 0;
  const char *str_byte = string;
  regoff_t pos = -1;
  regoff_t pos_add_next = 1;
#ifdef TRE_MBSTATE
  mbstate_t mbstate;
#endif /* TRE_MBSTATE */
  int reg_notbol = eflags & REG_NOTBOL;
  int reg_noteol = eflags & REG_NOTEOL;
  int reg_newline = tnfa->cflags & REG_NEWLINE;
  reg_errcode_t ret;

  char *buf;
  tre_tnfa_transition_t *trans_i;
  tre_tnfa_reach_t *reach, *reach_next, *reach_i, *reach_next_i;
  tre_reach_pos_t *reach_pos;
  int *tag_i;
  int num_tags, i;

  regoff_t match_eo = -1;	   /* end offset of match (-1 if no match found yet) */
  int new_match = 0;
  regoff_t *tmp_tags = NULL;
  regoff_t *tmp_iptr;

#ifdef TRE_MBSTATE
  memset(&mbstate, '\0', sizeof(mbstate));
#endif /* TRE_MBSTATE */

  if (!match_tags)
    num_tags = 0;
  else
    num_tags = tnfa->num_tags;

  /* Allocate memory for temporary data required for matching.	This needs to
     be done for every matching operation to be thread safe.  This allocates
     everything in a single large block with calloc(). */
  {
    size_t tbytes, rbytes, pbytes, xbytes, total_bytes;
    char *tmp_buf;

    /* Ensure that tbytes and xbytes*num_states cannot overflow, and that
     * they don't contribute more than 1/8 of SIZE_MAX to total_bytes. */
    if (num_tags > SIZE_MAX/(8 * sizeof(regoff_t) * tnfa->num_states))
      return REG_ESPACE;

    /* Likewise check rbytes. */
    if (tnfa->num_states+1 > SIZE_MAX/(8 * sizeof(*reach_next)))
      return REG_ESPACE;

    /* Likewise check pbytes. */
    if (tnfa->num_states > SIZE_MAX/(8 * sizeof(*reach_pos)))
      return REG_ESPACE;

    /* Compute the length of the block we need. */
    tbytes = sizeof(*tmp_tags) * num_tags;
    rbytes = sizeof(*reach_next) * (tnfa->num_states + 1);
    pbytes = sizeof(*reach_pos) * tnfa->num_states;
    xbytes = sizeof(regoff_t) * num_tags;
    total_bytes =
      (sizeof(long) - 1) * 4 /* for alignment paddings */
      + (rbytes + xbytes * tnfa->num_states) * 2 + tbytes + pbytes;

    /* Allocate the memory. */
    buf = calloc(total_bytes, 1);
    if (buf == NULL)
      return REG_ESPACE;

    /* Get the various pointers within tmp_buf (properly aligned). */
    tmp_tags = (void *)buf;
    tmp_buf = buf + tbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    reach_next = (void *)tmp_buf;
    tmp_buf += rbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    reach = (void *)tmp_buf;
    tmp_buf += rbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    reach_pos = (void *)tmp_buf;
    tmp_buf += pbytes;
    tmp_buf += ALIGN(tmp_buf, long);
    for (i = 0; i < tnfa->num_states; i++)
      {
	reach[i].tags = (void *)tmp_buf;
	tmp_buf += xbytes;
	reach_next[i].tags = (void *)tmp_buf;
	tmp_buf += xbytes;
      }
  }

  for (i = 0; i < tnfa->num_states; i++)
    reach_pos[i].pos = -1;

  GET_NEXT_WCHAR();
  pos = 0;

  reach_next_i = reach_next;
  while (1)
    {
      /* If no match found yet, add the initial states to `reach_next'. */
      if (match_eo < 0)
	{
	  trans_i = tnfa->initial;
	  while (trans_i->state != NULL)
	    {
	      if (reach_pos[trans_i->state_id].pos < pos)
		{
		  if (trans_i->assertions
		      && CHECK_ASSERTIONS(trans_i->assertions))
		    {
		      trans_i++;
		      continue;
		    }

		  reach_next_i->state = trans_i->state;
		  for (i = 0; i < num_tags; i++)
		    reach_next_i->tags[i] = -1;
		  tag_i = trans_i->tags;
		  if (tag_i)
		    while (*tag_i >= 0)
		      {
			if (*tag_i < num_tags)
			  reach_next_i->tags[*tag_i] = pos;
			tag_i++;
		      }
		  if (reach_next_i->state == tnfa->final)
		    {
		      match_eo = pos;
		      new_match = 1;
		      for (i = 0; i < num_tags; i++)
			match_tags[i] = reach_next_i->tags[i];
		    }
		  reach_pos[trans_i->state_id].pos = pos;
		  reach_pos[trans_i->state_id].tags = &reach_next_i->tags;
		  reach_next_i++;
		}
	      trans_i++;
	    }
	  reach_next_i->state = NULL;
	}
      else
	{
	  if (num_tags == 0 || reach_next_i == reach_next)
	    /* We have found a match. */
	    break;
	}

      /* Check for end of string. */
      if (!next_c) break;

      GET_NEXT_WCHAR();

      /* Swap `reach' and `reach_next'. */
      reach_i = reach;
      reach = reach_next;
      reach_next = reach_i;

      /* For each state in `reach', weed out states that don't fulfill the
	 minimal matching conditions. */
      if (tnfa->num_minimals && new_match)
	{
	  new_match = 0;
	  reach_next_i = reach_next;
	  for (reach_i = reach; reach_i->state; reach_i++)
	    {
	      int skip = 0;
	      for (i = 0; tnfa->minimal_tags[i] >= 0; i += 2)
		{
		  int end = tnfa->minimal_tags[i];
		  int start = tnfa->minimal_tags[i + 1];
		  if (end >= num_tags)
		    {
		      skip = 1;
		      break;
		    }
		  else if (reach_i->tags[start] == match_tags[start]
			   && reach_i->tags[end] < match_tags[end])
		    {
		      skip = 1;
		      break;
		    }
		}
	      if (!skip)
		{
		  reach_next_i->state = reach_i->state;
		  tmp_iptr = reach_next_i->tags;
		  reach_next_i->tags = reach_i->tags;
		  reach_i->tags = tmp_iptr;
		  reach_next_i++;
		}
	    }
	  reach_next_i->state = NULL;

	  /* Swap `reach' and `reach_next'. */
	  reach_i = reach;
	  reach = reach_next;
	  reach_next = reach_i;
	}

      /* For each state in `reach' see if there is a transition leaving with
	 the current input symbol to a state not yet in `reach_next', and
	 add the destination states to `reach_next'. */
      reach_next_i = reach_next;
      for (reach_i = reach; reach_i->state; reach_i++)
	{
	  for (trans_i = reach_i->state; trans_i->state; trans_i++)
	    {
	      /* Does this transition match the input symbol? */
	      if (trans_i->code_min <= (tre_cint_t)prev_c &&
		  trans_i->code_max >= (tre_cint_t)prev_c)
		{
		  if (trans_i->assertions
		      && (CHECK_ASSERTIONS(trans_i->assertions)
			  || CHECK_CHAR_CLASSES(trans_i, tnfa, eflags)))
		    {
		      continue;
		    }

		  /* Compute the tags after this transition. */
		  for (i = 0; i < num_tags; i++)
		    tmp_tags[i] = reach_i->tags[i];
		  tag_i = trans_i->tags;
		  if (tag_i != NULL)
		    while (*tag_i >= 0)
		      {
			if (*tag_i < num_tags)
			  tmp_tags[*tag_i] = pos;
			tag_i++;
		      }

		  if (reach_pos[trans_i->state_id].pos < pos)
		    {
		      /* Found an unvisited node. */
		      reach_next_i->state = trans_i->state;
		      tmp_iptr = reach_next_i->tags;
		      reach_next_i->tags = tmp_tags;
		      tmp_tags = tmp_iptr;
		      reach_pos[trans_i->state_id].pos = pos;
		      reach_pos[trans_i->state_id].tags = &reach_next_i->tags;

		      if (reach_next_i->state == tnfa->final
			  && (match_eo == -1
			      || (num_tags > 0
				  && reach_next_i->tags[0] <= match_tags[0])))
			{
			  match_eo = pos;
			  new_match = 1;
			  for (i = 0; i < num_tags; i++)
			    match_tags[i] = reach_next_i->tags[i];
			}
		      reach_next_i++;

		    }
		  else
		    {
		      assert(reach_pos[trans_i->state_id].pos == pos);
		      /* Another path has also reached this state.  We choose
			 the winner by examining the tag values for both
			 paths. */
		      if (tre_tag_order(num_tags, tnfa->tag_directions,
					tmp_tags,
					*reach_pos[trans_i->state_id].tags))
			{
			  /* The new path wins. */
			  tmp_iptr = *reach_pos[trans_i->state_id].tags;
			  *reach_pos[trans_i->state_id].tags = tmp_tags;
			  if (trans_i->state == tnfa->final)
			    {
			      match_eo = pos;
			      new_match = 1;
			      for (i = 0; i < num_tags; i++)
				match_tags[i] = tmp_tags[i];
			    }
			  tmp_tags = tmp_iptr;
			}
		    }
		}
	    }
	}
      reach_next_i->state = NULL;
    }

  *match_end_ofs = match_eo;
  ret = match_eo >= 0 ? REG_OK : REG_NOMATCH;
error_exit:
  xfree(buf);
  return ret;
}
/* Context-After
 * /***********************************************************************
 *  from tre-match-backtrack.c
 * ***********************************************************************/
 * 
 * /*
 *   This matcher is for regexps that use back referencing.  Regexp matching
 *   with back referencing is an NP-complete problem on the number of back
 *   references.  The easiest way to match them is to use a backtracking
 *   routine which basically goes through all possible paths in the TNFA
 *   and chooses the one which results in the best (leftmost and longest)
 *   match.  This can be spectacularly expensive and may run out of stack
 *   space, but there really is no better known generic algorithm.	 Quoting
 *   Henry Spencer from comp.compilers:
 *   <URL: http://compilers.iecc.com/comparch/article/93-03-102>
 * 
 *     POSIX.2 REs require longest match, which is really exciting to
 *     implement since the obsolete ("basic") variant also includes
 */
