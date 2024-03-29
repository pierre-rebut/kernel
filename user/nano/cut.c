
#include "proto.h"

#include <string.h>

/* Delete the character under the cursor. */
void do_deletion(undo_type action)
{
#ifndef NANO_TINY
	size_t old_amount = 0;
#endif

	openfile->placewewant = xplustabs();

	/* When in the middle of a line, delete the current character. */
	if (openfile->current->data[openfile->current_x] != '\0') {
		int char_len = parse_mbchar(openfile->current->data +
										openfile->current_x, NULL, NULL);
		size_t line_len = strlen(openfile->current->data +
										openfile->current_x);
#ifndef NANO_TINY
		/* If the type of action changed or the cursor moved to a different
		 * line, create a new undo item, otherwise update the existing item. */
		if (action != openfile->last_action ||
					openfile->current->lineno != openfile->current_undo->lineno)
			add_undo(action);
		else
			update_undo(action);

		if (ISSET(SOFTWRAP))
			old_amount = number_of_chunks_in(openfile->current);
#endif
		/* Move the remainder of the line "in", over the current character. */
		charmove(&openfile->current->data[openfile->current_x],
					&openfile->current->data[openfile->current_x + char_len],
					line_len - char_len + 1);
#ifndef NANO_TINY
		/* Adjust the mark if it is after the cursor on the current line. */
		if (openfile->mark == openfile->current &&
								openfile->mark_x > openfile->current_x)
			openfile->mark_x -= char_len;
#endif
	/* Otherwise, when not at end of buffer, join this line with the next. */
	} else if (openfile->current != openfile->filebot) {
		filestruct *joining = openfile->current->next;

		/* If there is a magic line, and we're before it: don't eat it. */
		if (joining == openfile->filebot && openfile->current_x != 0 &&
				!ISSET(NO_NEWLINES)) {
#ifndef NANO_TINY
			if (action == BACK)
				add_undo(BACK);
#endif
			return;
		}

#ifndef NANO_TINY
		add_undo(action);
#endif
		/* Add the contents of the next line to those of the current one. */
		openfile->current->data = charealloc(openfile->current->data,
				strlen(openfile->current->data) + strlen(joining->data) + 1);
		strcat(openfile->current->data, joining->data);

#ifndef NANO_TINY
		/* Adjust the mark if it was on the line that was "eaten". */
		if (openfile->mark == joining) {
			openfile->mark = openfile->current;
			openfile->mark_x += openfile->current_x;
		}
#endif
		unlink_node(joining);
		renumber(openfile->current);

		/* Two lines were joined, so we need to refresh the screen. */
		refresh_needed = true;
	} else
		/* We're at the end-of-file: nothing to do. */
		return;

	/* Adjust the file size, and remember it for a possible redo. */
	openfile->totsize--;
#ifndef NANO_TINY
	openfile->current_undo->newsize = openfile->totsize;

	/* If the number of screen rows that a softwrapped line occupies
	 * has changed, we need a full refresh. */
	if (ISSET(SOFTWRAP) && refresh_needed == false &&
				number_of_chunks_in(openfile->current) != old_amount)
		refresh_needed = true;
#endif

	set_modified();
}

/* Delete the character under the cursor. */
void do_delete(void)
{
#ifndef NANO_TINY
	if (openfile->mark && ISSET(LET_THEM_ZAP))
		zap_text();
	else
#endif
	do_deletion(DEL);
}

/* Backspace over one character.  That is, move the cursor left one
 * character, and then delete the character under the cursor. */
void do_backspace(void)
{
#ifndef NANO_TINY
	if (openfile->mark && ISSET(LET_THEM_ZAP))
		zap_text();
	else
#endif
	if (openfile->current != openfile->fileage || openfile->current_x > 0) {
		do_left();
		do_deletion(BACK);
	}
}

#ifndef NANO_TINY
/* Delete text from the cursor until the first start of a word to
 * the right, or to the left when backward is true. */
void do_cutword(bool backward)
{
	/* Remember the current cursor position. */
	filestruct *is_current = openfile->current;
	size_t is_current_x = openfile->current_x;

	/* Remember where the cutbuffer is and then make it seem blank. */
	filestruct *is_cutbuffer = cutbuffer;
	filestruct *is_cutbottom = cutbottom;
	cutbuffer = NULL;
	cutbottom = NULL;

	/* Move the cursor to a word start, to the left or to the right.
	 * If that word is on another line and the cursor was not already
	 * on the edge of the original line, then put the cursor on that
	 * edge instead, so that lines will not be joined unexpectedly. */
	if (backward) {
		do_prev_word(ISSET(WORD_BOUNDS));
		if (openfile->current != is_current) {
			if (is_current_x > 0) {
				openfile->current = is_current;
				openfile->current_x = 0;
			} else
				openfile->current_x = strlen(openfile->current->data);
		}
	} else {
		do_next_word(false, ISSET(WORD_BOUNDS));
		if (openfile->current != is_current &&
							is_current->data[is_current_x] != '\0') {
			openfile->current = is_current;
			openfile->current_x = strlen(is_current->data);
		}
	}

	/* Set the mark at the start of that word. */
	openfile->mark = openfile->current;
	openfile->mark_x = openfile->current_x;

	/* Put the cursor back where it was, so an undo will put it there too. */
	openfile->current = is_current;
	openfile->current_x = is_current_x;

	/* Now kill the marked region and a word is gone. */
	do_cut_text_void();

	/* Discard the cut word and restore the cutbuffer. */
	free_filestruct(cutbuffer);
	cutbuffer = is_cutbuffer;
	cutbottom = is_cutbottom;
}

/* Delete a word leftward. */
void do_cut_prev_word(void)
{
	do_cutword(true);
}

/* Delete a word rightward. */
void do_cut_next_word(void)
{
	if (is_cuttable(openfile->current_x > 0))
		do_cutword(false);
}
#endif /* !NANO_TINY */

/* If we aren't on the last line of the file, move all the text of the
 * current line, plus the newline at the end, into the cutbuffer.  If we
 * are, move all of the text of the current line into the cutbuffer.  In
 * both cases, set the current place we want to the beginning of the
 * current line. */
void cut_line(void)
{
	if (openfile->current != openfile->filebot)
		extract_buffer(&cutbuffer, &cutbottom, openfile->current, 0,
				openfile->current->next, 0);
	else
		extract_buffer(&cutbuffer, &cutbottom, openfile->current, 0,
				openfile->current, strlen(openfile->current->data));
	openfile->placewewant = 0;
}

#ifndef NANO_TINY
/* Move all currently marked text into the cutbuffer, and set the
 * current place we want to where the text used to start. */
void cut_marked(bool *right_side_up)
{
	filestruct *top, *bot;
	size_t top_x, bot_x;

	mark_order((const filestruct **)&top, &top_x,
				(const filestruct **)&bot, &bot_x, right_side_up);

	extract_buffer(&cutbuffer, &cutbottom, top, top_x, bot, bot_x);
	openfile->placewewant = xplustabs();
}

/* If we aren't at the end of the current line, move all the text from
 * the current cursor position to the end of the current line, not
 * counting the newline at the end, into the cutbuffer.  If we are, and
 * we're not on the last line of the file, move the newline at the end
 * into the cutbuffer, and set the current place we want to where the
 * newline used to be. */
void cut_to_eol(void)
{
	size_t data_len = strlen(openfile->current->data);

	if (openfile->current_x < data_len)
		/* If we're not at the end of the line, move all the text from
		 * the current position up to it, not counting the newline at
		 * the end, into the cutbuffer. */
		extract_buffer(&cutbuffer, &cutbottom, openfile->current,
				openfile->current_x, openfile->current, data_len);
	else if (openfile->current != openfile->filebot) {
		/* If we're at the end of the line, and it isn't the last line
		 * of the file, move all the text from the current position up
		 * to the beginning of the next line, i.e. the newline at the
		 * end, into the cutbuffer. */
		extract_buffer(&cutbuffer, &cutbottom, openfile->current,
				openfile->current_x, openfile->current->next, 0);
		openfile->placewewant = xplustabs();
	}
}

/* Move all the text from the current cursor position to the end of the
 * file into the cutbuffer. */
void cut_to_eof(void)
{
	extract_buffer(&cutbuffer, &cutbottom,
				openfile->current, openfile->current_x,
				openfile->filebot, strlen(openfile->filebot->data));
}
#endif /* !NANO_TINY */

/* Move text from the current buffer into the cutbuffer.  If
 * copy_text is true, copy the text back into the buffer afterward.
 * If cut_till_eof is true, move all text from the current cursor
 * position to the end of the file into the cutbuffer.  If append
 * is true (when zapping), always append the cut to the cutbuffer. */
void do_cut_text(bool copy_text, bool marked, bool cut_till_eof, bool append)
{
#ifndef NANO_TINY
	filestruct *cb_save = NULL;
		/* The current end of the cutbuffer, before we add text to it. */
	size_t cb_save_len = 0;
		/* The length of the string at the current end of the cutbuffer,
		 * before we add text to it. */
	bool using_magicline = !ISSET(NO_NEWLINES);
		/* Whether an automatic newline should be added at end-of-buffer. */
	bool right_side_up = true;
		/* There *is* no region, *or* it is marked forward. */
#endif

	/* If cuts were not continuous, or when cutting a region, clear the slate. */
	if (!append && (!keep_cutbuffer || marked || cut_till_eof)) {
		free_filestruct(cutbuffer);
		cutbuffer = NULL;
		/* After a line cut, future line cuts should add to the cutbuffer. */
		keep_cutbuffer = !marked && !cut_till_eof;
	}

#ifndef NANO_TINY
	if (copy_text) {
		/* If the cutbuffer isn't empty, remember where it currently ends. */
		if (cutbuffer != NULL) {
			cb_save = cutbottom;
			cb_save_len = strlen(cutbottom->data);
		}
		/* Don't add a magic line when moving text to the cutbuffer. */
		SET(NO_NEWLINES);
	}

	if (cut_till_eof) {
		/* Move all text up to the end of the file into the cutbuffer. */
		cut_to_eof();
	} else if (openfile->mark) {
		/* Move the marked text to the cutbuffer, and turn the mark off. */
		cut_marked(&right_side_up);
		openfile->mark = NULL;
	} else if (ISSET(CUT_FROM_CURSOR))
		/* Move all text up to the end of the line into the cutbuffer. */
		cut_to_eol();
	else
#endif
		/* Move the entire line into the cutbuffer. */
		cut_line();

#ifndef NANO_TINY
	if (copy_text) {
		/* Copy the text that is in the cutbuffer (starting at its saved end,
		 * if there is one) back into the current buffer.  This effectively
		 * uncuts the text we just cut. */
		if (cutbuffer != NULL) {
			if (cb_save != NULL) {
				cb_save->data += cb_save_len;
				copy_from_buffer(cb_save);
				cb_save->data -= cb_save_len;
			} else
				copy_from_buffer(cutbuffer);

			/* If the copied region was marked forward, put the new desired
			 * x position at its end; otherwise, leave it at its beginning. */
			if (right_side_up)
				openfile->placewewant = xplustabs();
		}
		/* Restore the magic-line behavior now that we're done fiddling. */
		if (using_magicline)
			UNSET(NO_NEWLINES);
	} else
#endif /* !NANO_TINY */

	set_modified();
	refresh_needed = true;
}

/* Return false when a cut command would not actually cut anything: when
 * on an empty line at EOF, or when the mark covers zero characters, or
 * (when test_cliff is true) when the magic line would be cut. */
bool is_cuttable(bool test_cliff)
{
	if ((openfile->current->next == NULL && openfile->current->data[0] == '\0'
#ifndef NANO_TINY
					&& openfile->mark == NULL) ||
					(openfile->mark == openfile->current &&
					openfile->mark_x == openfile->current_x) ||
					(test_cliff && openfile->current->data[openfile->current_x] == '\0' &&
					((ISSET(NO_NEWLINES) && openfile->current == openfile->filebot) ||
					(!ISSET(NO_NEWLINES) && openfile->current == openfile->filebot->prev))
#endif
					)) {
#ifndef NANO_TINY
		openfile->mark = NULL;
#endif
		statusbar(_("Nothing was cut"));
		return false;
	} else
		return true;
}

/* Move text from the current buffer into the cutbuffer. */
void do_cut_text_void(void)
{
#ifndef NANO_TINY
	if (!is_cuttable(ISSET(CUT_FROM_CURSOR) && openfile->mark == NULL))
		return;

	/* Only add a new undo item when the current item is not a CUT or when
	 * the current cut is not contiguous with the previous cutting. */
	if (openfile->last_action != CUT || openfile->current_undo == NULL ||
			openfile->current_undo->mark_begin_lineno != openfile->current->lineno ||
			!keep_cutbuffer)
		add_undo(CUT);
	do_cut_text(false, (bool) openfile->mark, false, false);
	update_undo(CUT);
#else
	if (is_cuttable(false))
		do_cut_text(false, false, false, false);
#endif
}

#ifndef NANO_TINY
/* Move text from the current buffer into the cutbuffer, and copy it
 * back into the buffer afterward.  If the mark is set or the cursor
 * was moved, blow away previous contents of the cutbuffer. */
void do_copy_text(void)
{
	static filestruct *next_contiguous_line = NULL;
	bool mark_is_set = (openfile->mark != NULL);

	/* Remember the current viewport and cursor position. */
	ssize_t is_edittop_lineno = openfile->edittop->lineno;
	size_t is_firstcolumn = openfile->firstcolumn;
	ssize_t is_current_lineno = openfile->current->lineno;
	size_t is_current_x = openfile->current_x;

	if (mark_is_set || openfile->current != next_contiguous_line)
		keep_cutbuffer = false;

	do_cut_text(true, mark_is_set, false, false);

	/* If the mark was set, blow away the cutbuffer on the next copy. */
	next_contiguous_line = (mark_is_set ? NULL : openfile->current);

	/* If the mark was set, restore the viewport and cursor position. */
	if (mark_is_set) {
		openfile->edittop = fsfromline(is_edittop_lineno);
		openfile->firstcolumn = is_firstcolumn;
		openfile->current = fsfromline(is_current_lineno);
		openfile->current_x = is_current_x;
	}
}

/* Cut from the current cursor position to the end of the file. */
void do_cut_till_eof(void)
{
	if ((openfile->current == openfile->filebot && openfile->current->data[0] == '\0') ||
				(!ISSET(NO_NEWLINES) && openfile->current->next == openfile->filebot &&
				openfile->current->data[openfile->current_x] == '\0')) {
		statusbar(_("Nothing was cut"));
		return;
	}

	add_undo(CUT_TO_EOF);
	do_cut_text(false, false, true, false);
	update_undo(CUT_TO_EOF);
}

/* Erase text (current line or marked region), sending it into oblivion. */
void zap_text(void)
{
	/* Remember the current cutbuffer so it can be restored after the zap. */
	filestruct *was_cutbuffer = cutbuffer;
	filestruct *was_cutbottom = cutbottom;

	if (!is_cuttable(ISSET(CUT_FROM_CURSOR) && openfile->mark == NULL))
		return;

	/* Add a new undo item only when the current item is not a ZAP or when
	 * the current zap is not contiguous with the previous zapping. */
	if (openfile->last_action != ZAP || openfile->current_undo == NULL ||
			openfile->current_undo->mark_begin_lineno != openfile->current->lineno ||
			openfile->current_undo->xflags & (MARK_WAS_SET|WAS_MARKED_FORWARD))
		add_undo(ZAP);

	/* Use the cutbuffer from the ZAP undo item, so the cut can be undone. */
	cutbuffer = openfile->current_undo->cutbuffer;
	cutbottom = openfile->current_undo->cutbottom;

	do_cut_text(false, (bool) openfile->mark, false, true);

	update_undo(ZAP);

	cutbuffer = was_cutbuffer;
	cutbottom = was_cutbottom;
}
#endif /* !NANO_TINY */

/* Copy text from the cutbuffer into the current buffer. */
void do_uncut_text(void)
{
	ssize_t was_lineno = openfile->current->lineno;
		/* The line number where we started the paste. */
	size_t was_leftedge = 0;
		/* The leftedge where we started the paste. */

	if (cutbuffer == NULL) {
		statusbar(_("The cutbuffer is empty"));
		return;
	}

#ifndef NANO_TINY
	add_undo(PASTE);

	if (ISSET(SOFTWRAP))
		was_leftedge = leftedge_for(xplustabs(), openfile->current);
#endif

	/* Add a copy of the text in the cutbuffer to the current buffer
	 * at the current cursor position. */
	copy_from_buffer(cutbuffer);

#ifndef NANO_TINY
	update_undo(PASTE);
#endif

	/* If we pasted less than a screenful, don't center the cursor. */
	if (less_than_a_screenful(was_lineno, was_leftedge))
		focusing = false;

	/* Set the desired x position to where the pasted text ends. */
	openfile->placewewant = xplustabs();

	set_modified();
	refresh_needed = true;
}
