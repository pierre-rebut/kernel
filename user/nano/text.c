/**************************************************************************
 *   text.c  --  This file is part of GNU nano.                           *
 *                                                                        *
 *   Copyright (C) 1999-2011, 2013-2018 Free Software Foundation, Inc.    *
 *   Copyright (C) 2014-2015 Mark Majeres                                 *
 *   Copyright (C) 2016 Mike Scalora                                      *
 *   Copyright (C) 2016 Sumedh Pendurkar                                  *
 *   Copyright (C) 2018 Marco Diego Aurélio Mesquita                      *
 *   Copyright (C) 2015-2018 Benno Schulenberg                            *
 *                                                                        *
 *   GNU nano is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published    *
 *   by the Free Software Foundation, either version 3 of the License,    *
 *   or (at your option) any later version.                               *
 *                                                                        *
 *   GNU nano is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include "proto.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef NANO_TINY
static pid_t pid_of_command = -1;
		/* The PID of the forked process -- needed when wanting to abort it. */
#endif
#ifdef ENABLE_WRAPPING
static bool prepend_wrap = false;
		/* Should we prepend wrapped text to the next line? */
#endif

#ifdef ENABLE_WORDCOMPLETION
static int pletion_x = 0;
		/* The x position in pletion_line of the last found completion. */
static completion_word *list_of_completions;
		/* A linked list of the completions that have been attempted. */
#endif

#ifndef NANO_TINY
/* Toggle the mark. */
void do_mark(void)
{
	if (!openfile->mark) {
		openfile->mark = openfile->current;
		openfile->mark_x = openfile->current_x;
		statusbar(_("Mark Set"));
		openfile->kind_of_mark = HARDMARK;
	} else {
		openfile->mark = NULL;
		statusbar(_("Mark Unset"));
		refresh_needed = true;
	}
}
#endif /* !NANO_TINY */

#if defined(ENABLE_COLOR) || defined(ENABLE_SPELLER)
/* Return an error message about invoking the given name. */
char *invocation_error(const char *name)
{
	char *message, *invoke_error = _("Error invoking \"%s\"");

	message = charalloc(strlen(invoke_error) + strlen(name) + 1);
	sprintf(message, invoke_error, name);
	return message;
}
#endif

/* Insert a tab.  If the TABS_TO_SPACES flag is set, insert the number
 * of spaces that a tab would normally take up. */
void do_tab(void)
{
#ifndef NANO_TINY
	if (ISSET(TABS_TO_SPACES)) {
		char *spaces = charalloc(tabsize + 1);
		size_t length = tabsize - (xplustabs() % tabsize);

		charset(spaces, ' ', length);
		spaces[length] = '\0';

		do_output(spaces, length, true);

		free(spaces);
	} else
#endif
		do_output((char *)"\t", 1, true);
}

#ifndef NANO_TINY
/* Add an indent to the given line. */
void indent_a_line(filestruct *line, char *indentation)
{
	size_t length = strlen(line->data);
	size_t indent_len = strlen(indentation);

	/* If the requested indentation is empty, don't change the line. */
	if (indent_len == 0)
		return;

	/* Add the fabricated indentation to the beginning of the line. */
	line->data = charealloc(line->data, length + indent_len + 1);
	charmove(line->data + indent_len, line->data, length + 1);
	strncpy(line->data, indentation, indent_len);

	openfile->totsize += indent_len;

	/* Compensate for the change in the current line. */
	if (line == openfile->mark && openfile->mark_x > 0)
		openfile->mark_x += indent_len;
	if (line == openfile->current && openfile->current_x > 0) {
		openfile->current_x += indent_len;
		openfile->placewewant = xplustabs();
	}
}

/* Indent the current line (or the marked lines) by tabsize columns.
 * This inserts either a tab character or a tab's worth of spaces,
 * depending on whether --tabstospaces is in effect. */
void do_indent(void)
{
	char *indentation;
	filestruct *top, *bot, *line;

	/* Use either all the marked lines or just the current line. */
	get_range((const filestruct **)&top, (const filestruct **)&bot);

	/* Skip any leading empty lines. */
	while (top != bot->next && top->data[0] == '\0')
		top = top->next;

	/* If all lines are empty, there is nothing to do. */
	if (top == bot->next)
		return;

	indentation = charalloc(tabsize + 1);

	/* Set the indentation to either a bunch of spaces or a single tab. */
	if (ISSET(TABS_TO_SPACES)) {
		charset(indentation, ' ', tabsize);
		indentation[tabsize] = '\0';
	} else {
		indentation[0] = '\t';
		indentation[1] = '\0';
	}

	add_undo(INDENT);

	/* Go through each of the lines, adding an indent to the non-empty ones,
	 * and recording whatever was added in the undo item. */
	for (line = top; line != bot->next; line = line->next) {
		char *real_indent = (line->data[0] == '\0') ? "" : indentation;

		indent_a_line(line, real_indent);
		update_multiline_undo(line->lineno, real_indent);
	}

	free(indentation);

	set_modified();
	refresh_needed = true;
	shift_held = true;
}

/* Return the number of bytes of whitespace at the start of the given text,
 * but at most a tab's worth. */
size_t length_of_white(const char *text)
{
	size_t bytes_of_white = 0;

	while (true) {
		if (*text == '\t')
			return ++bytes_of_white;

		if (*text != ' ')
			return bytes_of_white;

		if (++bytes_of_white == tabsize)
			return tabsize;

		text++;
	}
}

/* Adjust the positions of mark and cursor when they are on the given line. */
void compensate_leftward(filestruct *line, size_t leftshift)
{
	if (line == openfile->mark) {
		if (openfile->mark_x < leftshift)
			openfile->mark_x = 0;
		else
			openfile->mark_x -= leftshift;
	}

	if (line == openfile->current) {
		if (openfile->current_x < leftshift)
			openfile->current_x = 0;
		else
			openfile->current_x -= leftshift;
		openfile->placewewant = xplustabs();
	}
}

/* Remove an indent from the given line. */
void unindent_a_line(filestruct *line, size_t indent_len)
{
	size_t length = strlen(line->data);

	/* If the indent is empty, don't change the line. */
	if (indent_len == 0)
		return;

	/* Remove the first tab's worth of whitespace from this line. */
	charmove(line->data, line->data + indent_len, length - indent_len + 1);

	openfile->totsize -= indent_len;

	/* Adjust the positions of mark and cursor, when they are affected. */
	compensate_leftward(line, indent_len);
}

/* Unindent the current line (or the marked lines) by tabsize columns.
 * The removed indent can be a mixture of spaces plus at most one tab. */
void do_unindent(void)
{
	filestruct *top, *bot, *line;

	/* Use either all the marked lines or just the current line. */
	get_range((const filestruct **)&top, (const filestruct **)&bot);

	/* Skip any leading lines that cannot be unindented. */
	while (top != bot->next && length_of_white(top->data) == 0)
		top = top->next;

	/* If none of the lines can be unindented, there is nothing to do. */
	if (top == bot->next)
		return;

	add_undo(UNINDENT);

	/* Go through each of the lines, removing their leading indent where
	 * possible, and saving the removed whitespace in the undo item. */
	for (line = top; line != bot->next; line = line->next) {
		size_t indent_len = length_of_white(line->data);
		char *indentation = mallocstrncpy(NULL, line->data, indent_len + 1);

		indentation[indent_len] = '\0';

		unindent_a_line(line, indent_len);
		update_multiline_undo(line->lineno, indentation);

		free(indentation);
	}

	set_modified();
	refresh_needed = true;
	shift_held = true;
}

/* Perform an undo or redo for an indent or unindent action. */
void handle_indent_action(undo *u, bool undoing, bool add_indent)
{
	undo_group *group = u->grouping;
	filestruct *line = fsfromline(group->top_line);

	if (group->next != NULL)
		statusline(ALERT, "Multiple groups -- please report a bug");

	/* When redoing, reposition the cursor and let the indenter adjust it. */
	if (!undoing)
		goto_line_posx(u->lineno, u->begin);

	/* For each line in the group, add or remove the individual indent. */
	while (line && line->lineno <= group->bottom_line) {
		char *blanks = group->indentations[line->lineno - group->top_line];

		if (undoing ^ add_indent)
			indent_a_line(line, blanks);
		else
			unindent_a_line(line, strlen(blanks));

		line = line->next;
	}

	/* When undoing, reposition the cursor to the recorded location. */
	if (undoing)
		goto_line_posx(u->lineno, u->begin);

	refresh_needed = true;
}
#endif /* !NANO_TINY */

/* Test whether the string is empty or consists of only blanks. */
bool white_string(const char *s)
{
	while (*s != '\0' && (is_blank_mbchar(s) || *s == '\r'))
		s += move_mbright(s, 0);

	return !*s;
}

#ifdef ENABLE_COMMENT
/* Test whether the given line can be uncommented, or add or remove a comment,
 * depending on action.  Return true if the line is uncommentable, or when
 * anything was added or removed; false otherwise. */
bool comment_line(undo_type action, filestruct *line, const char *comment_seq)
{
	size_t comment_seq_len = strlen(comment_seq);
	const char *post_seq = strchr(comment_seq, '|');
		/* The postfix, if this is a bracketing type comment sequence. */
	size_t pre_len = post_seq ? post_seq++ - comment_seq : comment_seq_len;
		/* Length of prefix. */
	size_t post_len = post_seq ? comment_seq_len - pre_len - 1 : 0;
		/* Length of postfix. */
	size_t line_len = strlen(line->data);

	if (!ISSET(NO_NEWLINES) && line == openfile->filebot)
		return false;

	if (action == COMMENT) {
		/* Make room for the comment sequence(s), move the text right and
		 * copy them in. */
		line->data = charealloc(line->data, line_len + pre_len + post_len + 1);
		charmove(line->data + pre_len, line->data, line_len + 1);
		charmove(line->data, comment_seq, pre_len);
		if (post_len > 0)
			charmove(line->data + pre_len + line_len, post_seq, post_len + 1);

		openfile->totsize += pre_len + post_len;

		/* If needed, adjust the position of the mark and of the cursor. */
		if (line == openfile->mark && openfile->mark_x > 0)
			openfile->mark_x += pre_len;
		if (line == openfile->current && openfile->current_x > 0) {
			openfile->current_x += pre_len;
			openfile->placewewant = xplustabs();
		}

		return true;
	}

	/* If the line is commented, report it as uncommentable, or uncomment it. */
	if (strncmp(line->data, comment_seq, pre_len) == 0 && (post_len == 0 ||
				strcmp(line->data + line_len - post_len, post_seq) == 0)) {

		if (action == PREFLIGHT)
			return true;

		/* Erase the comment prefix by moving the non-comment part. */
		charmove(line->data, line->data + pre_len, line_len - pre_len);
		/* Truncate the postfix if there was one. */
		line->data[line_len - pre_len - post_len] = '\0';

		openfile->totsize -= pre_len + post_len;

		/* Adjust the positions of mark and cursor, when needed. */
		compensate_leftward(line, pre_len);

		return true;
	}

	return false;
}

/* Comment or uncomment the current line or the marked lines. */
void do_comment(void)
{
	const char *comment_seq = GENERAL_COMMENT_CHARACTER;
	undo_type action = UNCOMMENT;
	filestruct *top, *bot, *line;
	bool empty, all_empty = true;

#ifdef ENABLE_COLOR
	if (openfile->syntax)
		comment_seq = openfile->syntax->comment;

	if (*comment_seq == '\0') {
		statusbar(_("Commenting is not supported for this file type"));
		return;
	}
#endif

	/* Determine which lines to work on. */
	get_range((const filestruct **)&top, (const filestruct **)&bot);

	/* If only the magic line is selected, don't do anything. */
	if (top == bot && bot == openfile->filebot && !ISSET(NO_NEWLINES)) {
		statusbar(_("Cannot comment past end of file"));
		return;
	}

	/* Figure out whether to comment or uncomment the selected line or lines. */
	for (line = top; line != bot->next; line = line->next) {
		empty = white_string(line->data);

		/* If this line is not blank and not commented, we comment all. */
		if (!empty && !comment_line(PREFLIGHT, line, comment_seq)) {
			action = COMMENT;
			break;
		}
		all_empty = all_empty && empty;
	}

	/* If all selected lines are blank, we comment them. */
	action = all_empty ? COMMENT : action;

	add_undo(action);

	/* Store the comment sequence used for the operation, because it could
	 * change when the file name changes; we need to know what it was. */
	openfile->current_undo->strdata = mallocstrcpy(NULL, comment_seq);

	/* Comment/uncomment each of the selected lines when possible, and
	 * store undo data when a line changed. */
	for (line = top; line != bot->next; line = line->next) {
		if (comment_line(action, line, comment_seq))
			update_multiline_undo(line->lineno, "");
	}

	set_modified();
	refresh_needed = true;
	shift_held = true;
}

/* Perform an undo or redo for a comment or uncomment action. */
void handle_comment_action(undo *u, bool undoing, bool add_comment)
{
	undo_group *group = u->grouping;

	/* When redoing, reposition the cursor and let the commenter adjust it. */
	if (!undoing)
		goto_line_posx(u->lineno, u->begin);

	while (group) {
		filestruct *f = fsfromline(group->top_line);

		while (f && f->lineno <= group->bottom_line) {
			comment_line(undoing ^ add_comment ?
								COMMENT : UNCOMMENT, f, u->strdata);
			f = f->next;
		}

		group = group->next;
	}

	/* When undoing, reposition the cursor to the recorded location. */
	if (undoing)
		goto_line_posx(u->lineno, u->begin);

	refresh_needed = true;
}
#endif /* ENABLE_COMMENT */

#ifndef NANO_TINY
#define redo_paste undo_cut
#define undo_paste redo_cut

/* Undo a cut, or redo an uncut. */
void undo_cut(undo *u)
{
	/* Get to where we need to uncut from. */
	if (u->xflags & WAS_WHOLE_LINE)
		goto_line_posx(u->mark_begin_lineno, 0);
	else
		goto_line_posx(u->mark_begin_lineno, u->mark_begin_x);

	/* If nothing was actually cut, positioning the cursor was enough. */
	if (!u->cutbuffer)
		return;

	copy_from_buffer(u->cutbuffer);

	/* If the final line was originally cut, remove the extra magic line. */
	if ((u->xflags & WAS_FINAL_LINE) && !ISSET(NO_NEWLINES) &&
			openfile->current != openfile->filebot)
		remove_magicline();

	if (!(u->xflags & WAS_MARKED_FORWARD) && u->type != PASTE)
		goto_line_posx(u->mark_begin_lineno, u->mark_begin_x);
}

/* Redo a cut, or undo an uncut. */
void redo_cut(undo *u)
{
	filestruct *oldcutbuffer = cutbuffer, *oldcutbottom = cutbottom;

	goto_line_posx(u->lineno, u->begin);

	/* If nothing was actually cut, positioning the cursor was enough. */
	if (!u->cutbuffer)
		return;

	cutbuffer = NULL;
	cutbottom = NULL;

	openfile->mark = fsfromline(u->mark_begin_lineno);
	openfile->mark_x = (u->xflags & WAS_WHOLE_LINE) ? 0 : u->mark_begin_x;

	do_cut_text(false, true, false, u->type == ZAP);

	free_filestruct(cutbuffer);
	cutbuffer = oldcutbuffer;
	cutbottom = oldcutbottom;
}

/* Undo the last thing(s) we did. */
void do_undo(void)
{
	undo *u = openfile->current_undo;
	filestruct *f = NULL, *t = NULL;
	filestruct *oldcutbuffer, *oldcutbottom;
	char *data, *undidmsg = NULL;
	size_t from_x, to_x;

	if (!u) {
		statusbar(_("Nothing in undo buffer!"));
		return;
	}

	if (u->type <= REPLACE) {
		f = fsfromline(u->mark_begin_lineno);
		if (f == NULL)
			return;
	}

	openfile->current_x = u->begin;
	switch (u->type) {
	case ADD:
		/* TRANSLATORS: The next thirteen strings describe actions
		 * that are undone or redone.  They are all nouns, not verbs. */
		undidmsg = _("text add");
		if ((u->xflags & WAS_FINAL_LINE) && !ISSET(NO_NEWLINES))
			remove_magicline();
		data = charalloc(strlen(f->data) - strlen(u->strdata) + 1);
		strncpy(data, f->data, u->begin);
		strcpy(&data[u->begin], &f->data[u->begin + strlen(u->strdata)]);
		free(f->data);
		f->data = data;
		goto_line_posx(u->lineno, u->begin);
		break;
	case ENTER:
		if (f->next == NULL) {
			statusline(ALERT, "Missing break line -- please report a bug");
			break;
		}
		undidmsg = _("line break");
		from_x = (u->begin == 0) ? 0 : u->mark_begin_x;
		to_x = (u->begin == 0) ? u->mark_begin_x : u->begin;
		f->data = charealloc(f->data, strlen(f->data) +
								strlen(&u->strdata[from_x]) + 1);
		strcat(f->data, &u->strdata[from_x]);
		unlink_node(f->next);
		renumber(f);
		goto_line_posx(u->lineno, to_x);
		break;
	case BACK:
	case DEL:
		undidmsg = _("text delete");
		data = charalloc(strlen(f->data) + strlen(u->strdata) + 1);
		strncpy(data, f->data, u->begin);
		strcpy(&data[u->begin], u->strdata);
		strcpy(&data[u->begin + strlen(u->strdata)], &f->data[u->begin]);
		free(f->data);
		f->data = data;
		goto_line_posx(u->mark_begin_lineno, u->mark_begin_x);
		break;
	case JOIN:
		undidmsg = _("line join");
		/* When the join was done by a Backspace at the tail of the file,
		 * and the nonewlines flag isn't set, do not re-add a newline that
		 * wasn't actually deleted; just position the cursor. */
		if ((u->xflags & WAS_FINAL_BACKSPACE) && !ISSET(NO_NEWLINES)) {
			goto_line_posx(openfile->filebot->lineno, 0);
			break;
		}
		t = make_new_node(f);
		t->data = mallocstrcpy(NULL, u->strdata);
		data = mallocstrncpy(NULL, f->data, u->mark_begin_x + 1);
		data[u->mark_begin_x] = '\0';
		free(f->data);
		f->data = data;
		splice_node(f, t);
		renumber(t);
		goto_line_posx(u->lineno, u->begin);
		break;
	case REPLACE:
		undidmsg = _("text replace");
		goto_line_posx(u->lineno, u->begin);
		data = u->strdata;
		u->strdata = f->data;
		f->data = data;
		break;
#ifdef ENABLE_WRAPPING
	case SPLIT_END:
		goto_line_posx(u->lineno, u->begin);
		openfile->current_undo = openfile->current_undo->next;
		while (openfile->current_undo->type != SPLIT_BEGIN)
			do_undo();
		u = openfile->current_undo;
	case SPLIT_BEGIN:
		undidmsg = _("text add");
		break;
#endif
	case ZAP:
		undidmsg = _("erasure");
		undo_cut(u);
		break;
	case CUT_TO_EOF:
	case CUT:
		undidmsg = _("text cut");
		undo_cut(u);
		break;
	case PASTE:
		undidmsg = _("text uncut");
		undo_paste(u);
		break;
	case INSERT:
		undidmsg = _("text insert");
		oldcutbuffer = cutbuffer;
		oldcutbottom = cutbottom;
		cutbuffer = NULL;
		cutbottom = NULL;
		openfile->mark = fsfromline(u->mark_begin_lineno);
		openfile->mark_x = u->mark_begin_x;
		goto_line_posx(u->lineno, u->begin);
		cut_marked(NULL);
		free_filestruct(u->cutbuffer);
		u->cutbuffer = cutbuffer;
		u->cutbottom = cutbottom;
		cutbuffer = oldcutbuffer;
		cutbottom = oldcutbottom;
		break;
	case COUPLE_BEGIN:
		undidmsg = u->strdata;
		goto_line_posx(u->lineno, u->begin);
		break;
	case COUPLE_END:
		openfile->current_undo = openfile->current_undo->next;
		do_undo();
		do_undo();
		do_undo();
		return;
	case INDENT:
		handle_indent_action(u, true, true);
		undidmsg = _("indent");
		break;
	case UNINDENT:
		handle_indent_action(u, true, false);
		undidmsg = _("unindent");
		break;
#ifdef ENABLE_COMMENT
	case COMMENT:
		handle_comment_action(u, true, true);
		undidmsg = _("comment");
		break;
	case UNCOMMENT:
		handle_comment_action(u, true, false);
		undidmsg = _("uncomment");
		break;
#endif
	default:
		statusline(ALERT, "Wrong undo type -- please report a bug");
		break;
	}

	if (undidmsg && !pletion_line)
		statusline(HUSH, _("Undid action (%s)"), undidmsg);

	openfile->current_undo = openfile->current_undo->next;
	openfile->last_action = OTHER;
	openfile->mark = NULL;
	openfile->placewewant = xplustabs();

	openfile->totsize = u->wassize;

	/* When at the point where the file was last saved, unset "Modified". */
	if (openfile->current_undo == openfile->last_saved) {
		openfile->modified = false;
		titlebar(NULL);
	} else
		set_modified();
}

/* Redo the last thing(s) we undid. */
void do_redo(void)
{
	filestruct *f = NULL, *shoveline;
	char *data, *redidmsg = NULL;
	undo *u = openfile->undotop;

	if (u == NULL || u == openfile->current_undo) {
		statusbar(_("Nothing to re-do!"));
		return;
	}

	/* Get the previous undo item. */
	while (u != NULL && u->next != openfile->current_undo)
		u = u->next;

	if (u == NULL) {
		statusline(ALERT, "Bad undo stack -- please report a bug");
		return;
	}

	if (u->type <= REPLACE) {
		f = fsfromline(u->mark_begin_lineno);
		if (f == NULL)
			return;
	}

	switch (u->type) {
	case ADD:
		redidmsg = _("text add");
		if ((u->xflags & WAS_FINAL_LINE) && !ISSET(NO_NEWLINES))
			new_magicline();
		data = charalloc(strlen(f->data) + strlen(u->strdata) + 1);
		strncpy(data, f->data, u->begin);
		strcpy(&data[u->begin], u->strdata);
		strcpy(&data[u->begin + strlen(u->strdata)], &f->data[u->begin]);
		free(f->data);
		f->data = data;
		goto_line_posx(u->mark_begin_lineno, u->mark_begin_x);
		break;
	case ENTER:
		redidmsg = _("line break");
		shoveline = make_new_node(f);
		shoveline->data = mallocstrcpy(NULL, u->strdata);
		data = mallocstrncpy(NULL, f->data, u->begin + 1);
		data[u->begin] = '\0';
		free(f->data);
		f->data = data;
		splice_node(f, shoveline);
		renumber(shoveline);
		goto_line_posx(u->lineno + 1, u->mark_begin_x);
		break;
	case BACK:
	case DEL:
		redidmsg = _("text delete");
		data = charalloc(strlen(f->data) + strlen(u->strdata) + 1);
		strncpy(data, f->data, u->begin);
		strcpy(&data[u->begin], &f->data[u->begin + strlen(u->strdata)]);
		free(f->data);
		f->data = data;
		goto_line_posx(u->lineno, u->begin);
		break;
	case JOIN:
		if (f->next == NULL) {
			statusline(ALERT, "Missing join line -- please report a bug");
			break;
		}
		redidmsg = _("line join");
		/* When the join was done by a Backspace at the tail of the file,
		 * and the nonewlines flag isn't set, do not join anything, as
		 * nothing was actually deleted; just position the cursor. */
		if ((u->xflags & WAS_FINAL_BACKSPACE) && !ISSET(NO_NEWLINES)) {
			goto_line_posx(u->mark_begin_lineno, u->mark_begin_x);
			break;
		}
		f->data = charealloc(f->data, strlen(f->data) + strlen(u->strdata) + 1);
		strcat(f->data, u->strdata);
		unlink_node(f->next);
		renumber(f);
		goto_line_posx(u->mark_begin_lineno, u->mark_begin_x);
		break;
	case REPLACE:
		redidmsg = _("text replace");
		data = u->strdata;
		u->strdata = f->data;
		f->data = data;
		goto_line_posx(u->lineno, u->begin);
		break;
#ifdef ENABLE_WRAPPING
	case SPLIT_BEGIN:
		goto_line_posx(u->lineno, u->begin);
		openfile->current_undo = u;
		while (openfile->current_undo->type != SPLIT_END)
			do_redo();
		u = openfile->current_undo;
		goto_line_posx(u->lineno, u->begin);
	case SPLIT_END:
		redidmsg = _("text add");
		break;
#endif
	case ZAP:
		redidmsg = _("erasure");
		redo_cut(u);
		break;
	case CUT_TO_EOF:
	case CUT:
		redidmsg = _("text cut");
		redo_cut(u);
		break;
	case PASTE:
		redidmsg = _("text uncut");
		redo_paste(u);
		break;
	case INSERT:
		redidmsg = _("text insert");
		goto_line_posx(u->lineno, u->begin);
		copy_from_buffer(u->cutbuffer);
		free_filestruct(u->cutbuffer);
		u->cutbuffer = NULL;
		break;
	case COUPLE_BEGIN:
		openfile->current_undo = u;
		do_redo();
		do_redo();
		do_redo();
		return;
	case COUPLE_END:
		redidmsg = u->strdata;
		goto_line_posx(u->lineno, u->begin);
		break;
	case INDENT:
		handle_indent_action(u, false, true);
		redidmsg = _("indent");
		break;
	case UNINDENT:
		handle_indent_action(u, false, false);
		redidmsg = _("unindent");
		break;
#ifdef ENABLE_COMMENT
	case COMMENT:
		handle_comment_action(u, false, true);
		redidmsg = _("comment");
		break;
	case UNCOMMENT:
		handle_comment_action(u, false, false);
		redidmsg = _("uncomment");
		break;
#endif
	default:
		statusline(ALERT, "Wrong redo type -- please report a bug");
		break;
	}

	if (redidmsg)
		statusline(HUSH, _("Redid action (%s)"), redidmsg);

	openfile->current_undo = u;
	openfile->last_action = OTHER;
	openfile->mark = NULL;
	openfile->placewewant = xplustabs();

	openfile->totsize = u->newsize;

	/* When at the point where the file was last saved, unset "Modified". */
	if (openfile->current_undo == openfile->last_saved) {
		openfile->modified = false;
		titlebar(NULL);
	} else
		set_modified();
}
#endif /* !NANO_TINY */

/* Break the current line at the cursor position. */
void do_enter(void)
{
	filestruct *newnode = make_new_node(openfile->current);
	size_t extra = 0;
#ifndef NANO_TINY
	filestruct *sampleline = openfile->current;
	bool allblanks = false;

	if (ISSET(AUTOINDENT)) {
#ifdef ENABLE_JUSTIFY
		/* When doing automatic long-line wrapping and the next line is
		 * in this same paragraph, use its indentation as the model. */
		if (!ISSET(NO_WRAP) && sampleline->next != NULL &&
					inpar(sampleline->next) && !begpar(sampleline->next, 0))
			sampleline = sampleline->next;
#endif
		extra = indent_length(sampleline->data);

		/* When breaking in the indentation, limit the automatic one. */
		if (extra > openfile->current_x)
			extra = openfile->current_x;
		else if (extra == openfile->current_x)
			allblanks = true;
	}
#endif /* NANO_TINY */
	newnode->data = charalloc(strlen(openfile->current->data +
										openfile->current_x) + extra + 1);
	strcpy(&newnode->data[extra], openfile->current->data +
										openfile->current_x);
#ifndef NANO_TINY
	if (ISSET(AUTOINDENT)) {
		/* Copy the whitespace from the sample line to the new one. */
		strncpy(newnode->data, sampleline->data, extra);
		/* If there were only blanks before the cursor, trim them. */
		if (allblanks)
			openfile->current_x = 0;
	}
#endif

	null_at(&openfile->current->data, openfile->current_x);

#ifndef NANO_TINY
	add_undo(ENTER);

	/* Adjust the mark if it was on the current line after the cursor. */
	if (openfile->mark == openfile->current &&
				openfile->mark_x > openfile->current_x) {
		openfile->mark = newnode;
		openfile->mark_x += extra - openfile->current_x;
	}
#endif

	/* Insert the newly created line after the current one and renumber. */
	splice_node(openfile->current, newnode);
	renumber(newnode);

	/* Put the cursor on the new line, after any automatic whitespace. */
	openfile->current = newnode;
	openfile->current_x = extra;
	openfile->placewewant = xplustabs();

	openfile->totsize++;
	set_modified();

#ifndef NANO_TINY
	if (ISSET(AUTOINDENT) && !allblanks)
		openfile->totsize += extra;
	update_undo(ENTER);
#endif

	refresh_needed = true;
	focusing = false;
}

#ifndef NANO_TINY
/* Send an unconditional kill signal to the running external command. */
RETSIGTYPE cancel_the_command(int signal)
{
	(void) signal;
	kill(pid_of_command);
}

/* Send the text that starts at the given line to file descriptor fd. */
void send_data(const filestruct *line, int fd)
{
	FILE *tube = fdopen(fd); // todo check "w"

	if (tube == NULL)
		return;

	/* Send each line, except a final empty line. */
	while (line != NULL && (line->next != NULL || line->data[0] != '\0')) {
		fprintf(tube, "%s%s", line->data, line->next == NULL ? "" : "\n");
		line = line->next;
	}

	fclose(tube);
}

/* Execute the given command in a shell.  Return true on success. */
bool execute_command(const char *command)
{
	/* int from_fd[2], to_fd[2];
		// The pipes through which text will written and read.
	const bool should_pipe = (command[0] == '|');
	FILE *stream;
	const char *shellenv;
	struct sigaction oldaction, newaction;
		// Original and temporary handlers for SIGINT.
	bool setup_failed = false;
		// Whether setting up the temporary SIGINT handler failed.

	// Create a pipe to read the command's output from, and, if needed,
	// a pipe to feed the command's input through.
	if (pipe(from_fd) == -1 || (should_pipe && pipe(to_fd) == -1)) {
		statusbar(_("Could not create pipe"));
		return false;
	}

	// Check which shell to use.  If none is specified, use /bin/sh
	shellenv = getenv("SHELL");
	if (shellenv == NULL)
		shellenv = (char *) "/bin/sh";

	// Fork a child process to run the command in.
	if ((pid_of_command = fork()) == 0) {
		// Child: close the unused read end of the output pipe.
		close(from_fd[0]);

		// Connect the write end of the output pipe to the process' output streams.
		dup2(from_fd[1], fileno(stdout));
		dup2(from_fd[1], fileno(stderr));

		// If the parent sends text, connect the read end of the
		// feeding pipe to the child's input stream.
		if (should_pipe) {
			dup2(to_fd[0], fileno(stdin));
			close(to_fd[1]);
		}

		// Run the given command inside the preferred shell.
		execl(shellenv, tail(shellenv), "-c", should_pipe ? &command[1] : command, NULL);

		// If the exec call returns, there was an error.
		exit(1);
	}

	// Parent: close the unused write end of the pipe.
	close(from_fd[1]);

	if (pid_of_command == -1) {
		statusbar(_("Could not fork"));
		close(from_fd[0]);
		return false;
	}

	// If the command starts with "|", pipe buffer or region to the command.
	if (should_pipe) {
		filestruct *was_cutbuffer = cutbuffer;
		cutbuffer = NULL;

#ifdef ENABLE_MULTIBUFFER
		if (ISSET(MULTIBUFFER)) {
			switch_to_prev_buffer();
			if (openfile->mark)
				do_cut_text(true, true, false, false);
		} else
#endif
		{
			add_undo(COUPLE_BEGIN);
			openfile->undotop->strdata = mallocstrcpy(NULL, _("filtering"));
			if (openfile->mark == NULL) {
				openfile->current = openfile->fileage;
				openfile->current_x = 0;
			}
			add_undo(CUT);
			do_cut_text(false, openfile->mark, openfile->mark == NULL, false);
			update_undo(CUT);
		}

		if (fork() == 0) {
			close(to_fd[0]);
			send_data(cutbuffer != NULL ? cutbuffer : openfile->fileage, to_fd[1]);
			close(to_fd[1]);
			exit(0);
		}

		close(to_fd[0]);
		close(to_fd[1]);

#ifdef ENABLE_MULTIBUFFER
		if (ISSET(MULTIBUFFER))
			switch_to_next_buffer();
#endif
		free_filestruct(cutbuffer);
		cutbuffer = was_cutbuffer;
	}

	// Re-enable interpretation of the special control keys so that we get
	// SIGINT when Ctrl-C is pressed.
	enable_signals();

	// Set things up so that Ctrl-C will terminate the forked process.
	if (sigaction(SIGINT, NULL, &newaction) == -1) {
		setup_failed = true;
		nperror("sigaction");
	} else {
		newaction.sa_handler = cancel_the_command;
		if (sigaction(SIGINT, &newaction, &oldaction) == -1) {
			setup_failed = true;
			nperror("sigaction");
		}
	}

	stream = fdopen(from_fd[0]); // todo check rb
	if (stream == NULL)
		statusline(ALERT, _("Failed to open pipe: %s"), strerror(errno));
	else
		read_file(stream, 0, "pipe", true);

	if (should_pipe && !ISSET(MULTIBUFFER)) {
		add_undo(COUPLE_END);
		openfile->undotop->strdata = mallocstrcpy(NULL, _("filtering"));
	}

	// Wait for the external command (and possibly data sender) to terminate.
	if (wait(NULL) == -1)
		nperror("wait");
	if (should_pipe && (wait(NULL) == -1))
		nperror("wait");

	// If it was changed, restore the handler for SIGINT.
	if (!setup_failed && sigaction(SIGINT, &oldaction, NULL) == -1)
		nperror("sigaction");

	// Restore the terminal to its desired state, and disable
	 // interpretation of the special control keys again.
	terminal_init();
	 */

	return true;
}

/* Discard undo items that are newer than the given one, or all if NULL.
 * When keep is true, do not touch the last_saved pointer. */
void discard_until(const undo *thisitem, openfilestruct *thefile, bool keep)
{
	undo *dropit = thefile->undotop;
	undo_group *group;

	while (dropit != NULL && dropit != thisitem) {
		thefile->undotop = dropit->next;
		free(dropit->strdata);
		free_filestruct(dropit->cutbuffer);
		group = dropit->grouping;
		while (group != NULL) {
			undo_group *next = group->next;
			free_chararray(group->indentations,
								group->bottom_line - group->top_line);
			free(group);
			group = next;
		}
		free(dropit);
		dropit = thefile->undotop;
	}

	/* Adjust the pointer to the top of the undo stack. */
	thefile->current_undo = (undo *)thisitem;

	/* Prevent a chain of editing actions from continuing. */
	thefile->last_action = OTHER;

	/* When requested, record that the undo stack was chopped, and
	 * that thus there is no point at which the file was last saved. */
	if (!keep)
		thefile->last_saved = (undo *)0xbeeb;
}

/* Add a new undo item of the given type to the top of the current pile. */
void add_undo(undo_type action)
{
	undo *u = openfile->current_undo;
		/* The thing we did previously. */

	/* Blow away newer undo items if we add somewhere in the middle. */
	discard_until(u, openfile, true);

	/* Allocate and initialize a new undo item. */
	u = (undo *) nmalloc(sizeof(undo));
	u->type = action;
	u->strdata = NULL;
	u->cutbuffer = NULL;
	u->cutbottom = NULL;
	u->lineno = openfile->current->lineno;
	u->begin = openfile->current_x;
	u->mark_begin_lineno = openfile->current->lineno;
	u->mark_begin_x = openfile->current_x;
	u->wassize = openfile->totsize;
	u->newsize = openfile->totsize;
	u->xflags = 0;
	u->grouping = NULL;

#ifdef ENABLE_WRAPPING
	/* If some action caused automatic long-line wrapping, insert the
	 * SPLIT_BEGIN item underneath that action's undo item.  Otherwise,
	 * just add the new item to the top of the undo stack. */
	if (u->type == SPLIT_BEGIN) {
		u->next = openfile->undotop->next;
		openfile->undotop->next = u;
	} else
#endif
	{
		u->next = openfile->undotop;
		openfile->undotop = u;
		openfile->current_undo = u;
	}

	/* Record the info needed to be able to undo each possible action. */
	switch (u->type) {
	case ADD:
		/* If a new magic line will be added, an undo should remove it. */
		if (openfile->current == openfile->filebot)
			u->xflags |= WAS_FINAL_LINE;
		u->wassize--;
		break;
	case ENTER:
		break;
	case BACK:
		/* If the next line is the magic line, don't ever undo this
		 * backspace, as it won't actually have deleted anything. */
		if (openfile->current->next == openfile->filebot &&
						openfile->current->data[0] != '\0')
			u->xflags |= WAS_FINAL_BACKSPACE;
	case DEL:
		/* When not at the end of a line, store the deleted character,
		 * else purposely fall into the line-joining code. */
		if (openfile->current->data[openfile->current_x] != '\0') {
			char *char_buf = charalloc(MAXCHARLEN + 1);
			int char_len = parse_mbchar(&openfile->current->data[u->begin],
												char_buf, NULL);
			char_buf[char_len] = '\0';
			u->strdata = char_buf;
			if (u->type == BACK)
				u->mark_begin_x += char_len;
			break;
		}
	case JOIN:
		if (openfile->current->next) {
			if (u->type == BACK) {
				u->lineno = openfile->current->next->lineno;
				u->begin = 0;
			}
			u->strdata = mallocstrcpy(NULL, openfile->current->next->data);
		}
		action = u->type = JOIN;
		break;
	case REPLACE:
		u->strdata = mallocstrcpy(NULL, openfile->current->data);
		break;
#ifdef ENABLE_WRAPPING
	case SPLIT_BEGIN:
		action = openfile->undotop->type;
		break;
	case SPLIT_END:
		break;
#endif
	case CUT_TO_EOF:
		u->xflags |= WAS_FINAL_LINE;
		break;
	case ZAP:
	case CUT:
		if (openfile->mark) {
			u->mark_begin_lineno = openfile->mark->lineno;
			u->mark_begin_x = openfile->mark_x;
			u->xflags |= MARK_WAS_SET;
			if (openfile->current == openfile->filebot ||
						openfile->mark == openfile->filebot)
				u->xflags |= WAS_FINAL_LINE;
		} else if (!ISSET(CUT_FROM_CURSOR)) {
			/* The entire line is being cut regardless of the cursor position. */
			u->begin = 0;
			u->xflags |= WAS_WHOLE_LINE;
		}
		break;
	case PASTE:
		u->cutbuffer = copy_filestruct(cutbuffer);
		u->lineno += cutbottom->lineno - cutbuffer->lineno;
		break;
	case INSERT:
	case COUPLE_BEGIN:
	case COUPLE_END:
		break;
	case INDENT:
	case UNINDENT:
		break;
#ifdef ENABLE_COMMENT
	case COMMENT:
	case UNCOMMENT:
		break;
#endif
	default:
		statusline(ALERT, "Wrong undo adding type -- please report a bug");
		break;
	}

	openfile->last_action = action;
}

/* Update a multiline undo item.  This should be called once for each line
 * affected by a multiple-line-altering feature.  The indentation that is
 * added or removed is saved separately for each line in the undo item. */
void update_multiline_undo(ssize_t lineno, char *indentation)
{
	undo *u = openfile->current_undo;

	/* If there already is a group and the current line is contiguous with it,
	 * extend the group; otherwise, create a new group. */
	if (u->grouping && u->grouping->bottom_line + 1 == lineno) {
		size_t number_of_lines;

		u->grouping->bottom_line++;

		number_of_lines = u->grouping->bottom_line - u->grouping->top_line + 1;
		u->grouping->indentations = (char **)nrealloc(u->grouping->indentations,
										number_of_lines * sizeof(char *));
		u->grouping->indentations[number_of_lines - 1] = mallocstrcpy(NULL,
																indentation);
	} else {
		undo_group *born = (undo_group *)nmalloc(sizeof(undo_group));

		born->next = u->grouping;
		u->grouping = born;
		born->top_line = lineno;
		born->bottom_line = lineno;

		u->grouping->indentations = (char **)nmalloc(sizeof(char *));
		u->grouping->indentations[0] = mallocstrcpy(NULL, indentation);
	}

	/* Store the file size after the change, to be used when redoing. */
	u->newsize = openfile->totsize;
}

/* Update an undo item with (among other things) the file size and
 * cursor position after the given action. */
void update_undo(undo_type action)
{
	undo *u = openfile->undotop;
	char *char_buf;
	int char_len;

	u->newsize = openfile->totsize;

	switch (u->type) {
	case ADD:
		char_buf = charalloc(MAXCHARLEN);
		char_len = parse_mbchar(&openfile->current->data[u->mark_begin_x],
								char_buf, NULL);
		u->strdata = addstrings(u->strdata, u->strdata ? strlen(u->strdata) : 0,
								char_buf, char_len);
		u->mark_begin_lineno = openfile->current->lineno;
		u->mark_begin_x = openfile->current_x;
		break;
	case ENTER:
		u->strdata = mallocstrcpy(NULL, openfile->current->data);
		u->mark_begin_x = openfile->current_x;
		break;
	case BACK:
	case DEL:
		char_buf = charalloc(MAXCHARLEN);
		char_len = parse_mbchar(&openfile->current->data[openfile->current_x],
								char_buf, NULL);
		if (openfile->current_x == u->begin) {
			/* They deleted more: add removed character after earlier stuff. */
			u->strdata = addstrings(u->strdata, strlen(u->strdata), char_buf, char_len);
			u->mark_begin_x = openfile->current_x;
		} else if (openfile->current_x == u->begin - char_len) {
			/* They backspaced further: add removed character before earlier. */
			u->strdata = addstrings(char_buf, char_len, u->strdata, strlen(u->strdata));
			u->begin = openfile->current_x;
		} else {
			/* They deleted *elsewhere* on the line: start a new undo item. */
			free(char_buf);
			add_undo(u->type);
			return;
		}
		break;
	case JOIN:
		break;
	case REPLACE:
	case PASTE:
		u->lineno = openfile->current->lineno;
		u->begin = openfile->current_x;
		break;
#ifdef ENABLE_WRAPPING
	case SPLIT_BEGIN:
	case SPLIT_END:
		break;
#endif
	case ZAP:
	case CUT_TO_EOF:
	case CUT:
		if (!cutbuffer)
			break;
		if (u->type == ZAP)
			u->cutbuffer = cutbuffer;
		else {
			free_filestruct(u->cutbuffer);
			u->cutbuffer = copy_filestruct(cutbuffer);
		}
		if (u->xflags & MARK_WAS_SET) {
			/* If the "marking" operation was from right-->left or
			 * bottom-->top, then swap the mark points. */
			if ((u->lineno == u->mark_begin_lineno && u->begin < u->mark_begin_x)
						|| u->lineno < u->mark_begin_lineno) {
				ssize_t line = u->lineno;
				size_t x_loc = u->begin;

				u->begin = u->mark_begin_x;
				u->mark_begin_x = x_loc;

				u->lineno = u->mark_begin_lineno;
				u->mark_begin_lineno = line;
			} else
				u->xflags |= WAS_MARKED_FORWARD;
		} else {
			/* Compute the end of the cut for the undo, using our copy. */
			u->cutbottom = u->cutbuffer;
			while (u->cutbottom->next != NULL)
				u->cutbottom = u->cutbottom->next;
			u->lineno = u->mark_begin_lineno + u->cutbottom->lineno -
										u->cutbuffer->lineno;
			if (ISSET(CUT_FROM_CURSOR) || u->type == CUT_TO_EOF) {
				u->begin = strlen(u->cutbottom->data);
				if (u->lineno == u->mark_begin_lineno)
					u->begin += u->mark_begin_x;
			} else if (openfile->current == openfile->filebot &&
						ISSET(NO_NEWLINES))
				u->begin = strlen(u->cutbottom->data);
		}
		break;
	case INSERT:
		u->mark_begin_lineno = openfile->current->lineno;
		u->mark_begin_x = openfile->current_x;
	case COUPLE_BEGIN:
		break;
	case COUPLE_END:
		u->lineno = openfile->current->lineno;
		u->begin = openfile->current_x;
		break;
	default:
		statusline(ALERT, "Wrong undo update type -- please report a bug");
		break;
	}
}
#endif /* !NANO_TINY */

#ifdef ENABLE_WRAPPING
/* Unset the prepend_wrap flag.  We need to do this as soon as we do
 * something other than type text. */
void wrap_reset(void)
{
	prepend_wrap = false;
}

/* Try wrapping the given line.  Return true if wrapped, false otherwise. */
bool do_wrap(filestruct *line)
{
	size_t line_len = strlen(line->data);
		/* The length of the line we wrap. */
	ssize_t wrap_loc;
		/* The index of line->data where we wrap. */
	const char *remainder;
		/* The text after the wrap point. */
	size_t rest_length;
		/* The length of the remainder. */

	size_t old_x = openfile->current_x;
	filestruct *old_line = openfile->current;

	/* There are three steps.  First, we decide where to wrap.  Then, we
	 * create the new wrap line.  Finally, we clean up. */

	/* Step 1, finding where to wrap.  We are going to add a new line
	 * after a blank character.  In this step, we call break_line() to
	 * get the location of the last blank we can break the line at, and
	 * set wrap_loc to the location of the character after it, so that
	 * the blank is preserved at the end of the line.
	 *
	 * If there is no legal wrap point, or we reach the last character
	 * of the line while trying to find one, we should return without
	 * wrapping.  Note that if autoindent is turned on, we don't break
	 * at the end of it! */

	/* Find the last blank where we can break the line. */
	wrap_loc = break_line(line->data, wrap_at, false);

	/* If we couldn't break the line, or we've reached the end of it, we
	 * don't wrap. */
	if (wrap_loc == -1 || line->data[wrap_loc] == '\0')
		return false;

	/* Otherwise, move forward to the character just after the blank. */
	wrap_loc += move_mbright(line->data + wrap_loc, 0);

	/* If we've reached the end of the line, we don't wrap. */
	if (line->data[wrap_loc] == '\0')
		return false;

#ifndef NANO_TINY
	/* If autoindent is turned on, and we're on the character just after
	 * the indentation, we don't wrap. */
	if (ISSET(AUTOINDENT) && wrap_loc == indent_length(line->data))
		return false;

	add_undo(SPLIT_BEGIN);
#endif

	openfile->current = line;

	/* Step 2, making the new wrap line.  It will consist of indentation
	 * followed by the text after the wrap point, optionally followed by
	 * a space (if the text after the wrap point doesn't end in a blank)
	 * and the text of the next line, if they can fit without wrapping,
	 * the next line exists, and the prepend_wrap flag is set. */

	/* The remainder is the text that will be wrapped to the next line. */
	remainder = line->data + wrap_loc;
	rest_length = line_len - wrap_loc;

	/* We prepend the wrapped text to the next line, if the prepend_wrap
	 * flag is set, there is a next line, and prepending would not make
	 * the line too long. */
	if (prepend_wrap && line != openfile->filebot) {
		const char *tail = remainder + move_mbleft(remainder, rest_length);

		/* Go to the end of the line. */
		openfile->current_x = line_len;

		/* If the remainder doesn't end in a blank, add a space. */
		if (!is_blank_mbchar(tail)) {
#ifndef NANO_TINY
			add_undo(ADD);
#endif
			line->data = charealloc(line->data, line_len + 2);
			line->data[line_len] = ' ';
			line->data[line_len + 1] = '\0';
			remainder = line->data + wrap_loc;
			rest_length++;
			openfile->totsize++;
			openfile->current_x++;
#ifndef NANO_TINY
			update_undo(ADD);
#endif
		}

		if (rest_length + strlen(line->next->data) <= wrap_at) {
			/* Delete the LF to join the two lines. */
			do_delete();
			/* Delete any leading blanks from the joined-on line. */
			while (is_blank_mbchar(&line->data[openfile->current_x]))
				do_delete();
		}
	}

	/* Go to the wrap location. */
	openfile->current_x = wrap_loc;

	/* When requested, snip trailing blanks off the wrapped line. */
	if (ISSET(TRIM_BLANKS)) {
		size_t tail_x = move_mbleft(line->data, wrap_loc);
		size_t typed_x = move_mbleft(line->data, old_x);

		while (tail_x != typed_x && is_blank_mbchar(line->data + tail_x)) {
			openfile->current_x = tail_x;
			do_delete();
			tail_x = move_mbleft(line->data, tail_x);
		}
	}

	/* Now split the line. */
	do_enter();

	if (old_x < wrap_loc) {
		openfile->current_x = old_x;
		openfile->current = old_line;
		prepend_wrap = true;
	} else {
		openfile->current_x += (old_x - wrap_loc);
		prepend_wrap = false;
	}

	openfile->placewewant = xplustabs();

#ifndef NANO_TINY
	add_undo(SPLIT_END);
#endif

	return true;
}
#endif /* ENABLE_WRAPPING */

#if defined(ENABLE_HELP) || defined(ENABLED_WRAPORJUSTIFY)
/* We are trying to break a chunk off line.  We find the last blank such
 * that the display length to there is at most (goal + 1).  If there is
 * no such blank, then we find the first blank.  We then take the last
 * blank in that group of blanks.  The terminating '\0' counts as a
 * blank, as does a '\n' if snap_at_nl is true. */
ssize_t break_line(const char *line, ssize_t goal, bool snap_at_nl)
{
	ssize_t lastblank = -1;
		/* The index of the last blank we found. */
	ssize_t index = 0;
		/* The index of the character we are looking at. */
	size_t column = 0;
		/* The column position that corresponds with index. */
	int char_len = 0;
		/* The length of the current character, in bytes. */

	/* Find the last blank that does not overshoot the target column. */
	while (*line != '\0' && ((ssize_t)column <= goal)) {
		if (is_blank_mbchar(line) || (snap_at_nl && *line == '\n')) {
			lastblank = index;

			if (*line == '\n')
				break;
		}

		char_len = parse_mbchar(line, NULL, &column);
		line += char_len;
		index += char_len;
	}

	/* If the whole line displays shorter than goal, we're done. */
	if ((ssize_t)column <= goal)
		return index;

#ifdef ENABLE_HELP
	/* If we're wrapping a help text and no blank was found, or was
	 * found only as the first character, force a line break. */
	if (snap_at_nl && lastblank < 1)
		return (index - char_len);
#endif

	/* If no blank was found within the goal width, seek one after it. */
	if (lastblank < 0) {
		while (*line != '\0') {
			if (is_blank_mbchar(line))
				lastblank = index;
			else if (lastblank > 0)
				return lastblank;

			char_len = parse_mbchar(line, NULL, NULL);
			line += char_len;
			index += char_len;
		}

		return -1;
	}

	/* Move the pointer back to the last blank, and then step beyond it. */
	line = line - index + lastblank;
	char_len = parse_mbchar(line, NULL, NULL);
	line += char_len;

	/* Skip any consecutive blanks after the last blank. */
	while (*line != '\0' && is_blank_mbchar(line)) {
		lastblank += char_len;
		char_len = parse_mbchar(line, NULL, NULL);
		line += char_len;
	}

	return lastblank;
}
#endif /* ENABLE_HELP || ENABLED_WRAPORJUSTIFY */

#if !defined(NANO_TINY) || defined(ENABLE_JUSTIFY)
/* The "indentation" of a line is the whitespace between the quote part
 * and the non-whitespace of the line. */
size_t indent_length(const char *line)
{
	size_t len = 0;
	char onechar[MAXCHARLEN];
	int charlen;

	while (*line != '\0') {
		charlen = parse_mbchar(line, onechar, NULL);

		if (!is_blank_mbchar(onechar))
			break;

		line += charlen;
		len += charlen;
	}

	return len;
}
#endif /* !NANO_TINY || ENABLE_JUSTIFY */

#ifdef ENABLE_JUSTIFY
/* justify_format() replaces blanks with spaces and multiple spaces by 1
 * (except it maintains up to 2 after a character in punct optionally
 * followed by a character in brackets, and removes all from the end).
 *
 * justify_format() might make paragraph->data shorter, and change the
 * actual pointer with null_at().
 *
 * justify_format() will not look at the first skip characters of
 * paragraph.  skip should be at most strlen(paragraph->data).  The
 * character at paragraph[skip + 1] must not be blank. */
void justify_format(filestruct *paragraph, size_t skip)
{
	char *end, *new_end, *new_paragraph_data;
	size_t shift = 0;

	end = paragraph->data + skip;
	new_paragraph_data = charalloc(strlen(paragraph->data) + 1);
	strncpy(new_paragraph_data, paragraph->data, skip);
	new_end = new_paragraph_data + skip;

	while (*end != '\0') {
		int end_len;

		/* If this character is blank, change it to a space if
		 * necessary, and skip over all blanks after it. */
		if (is_blank_mbchar(end)) {
			end_len = parse_mbchar(end, NULL, NULL);

			*new_end = ' ';
			new_end++;
			end += end_len;

			while (*end != '\0' && is_blank_mbchar(end)) {
				end_len = parse_mbchar(end, NULL, NULL);

				end += end_len;
				shift += end_len;
			}
		/* If this character is punctuation optionally followed by a
		 * bracket and then followed by blanks, change no more than two
		 * of the blanks to spaces if necessary, and skip over all
		 * blanks after them. */
		} else if (mbstrchr(punct, end) != NULL) {
			end_len = parse_mbchar(end, NULL, NULL);

			while (end_len > 0) {
				*new_end = *end;
				new_end++;
				end++;
				end_len--;
			}

			if (*end != '\0' && mbstrchr(brackets, end) != NULL) {
				end_len = parse_mbchar(end, NULL, NULL);

				while (end_len > 0) {
					*new_end = *end;
					new_end++;
					end++;
					end_len--;
				}
			}

			if (*end != '\0' && is_blank_mbchar(end)) {
				end_len = parse_mbchar(end, NULL, NULL);

				*new_end = ' ';
				new_end++;
				end += end_len;
			}

			if (*end != '\0' && is_blank_mbchar(end)) {
				end_len = parse_mbchar(end, NULL, NULL);

				*new_end = ' ';
				new_end++;
				end += end_len;
			}

			while (*end != '\0' && is_blank_mbchar(end)) {
				end_len = parse_mbchar(end, NULL, NULL);

				end += end_len;
				shift += end_len;
			}
		/* If this character is neither blank nor punctuation, leave it
		 * unchanged. */
		} else {
			end_len = parse_mbchar(end, NULL, NULL);

			while (end_len > 0) {
				*new_end = *end;
				new_end++;
				end++;
				end_len--;
			}
		}
	}

	*new_end = *end;

	/* If there are spaces at the end of the line, remove them. */
	while (new_end > new_paragraph_data + skip && *(new_end - 1) == ' ') {
		new_end--;
		shift++;
	}

	if (shift > 0) {
		null_at(&new_paragraph_data, new_end - new_paragraph_data);
		free(paragraph->data);
		paragraph->data = new_paragraph_data;
	} else
		free(new_paragraph_data);
}

/* The "quote part" of a line is the largest initial substring matching
 * the quote string.  This function returns the length of the quote part
 * of the given line. */
size_t quote_length(const char *line)
{
	regmatch_t matches;
	int rc = regexec(&quotereg, line, 1, &matches, 0);

	if (rc == REG_NOMATCH || matches.rm_so == (regoff_t)-1)
		return 0;
	/* matches.rm_so should be 0, since the quote string should start
	 * with the caret ^. */
	return matches.rm_eo;
}

/* The maximum depth of recursion.  This must be an even number. */
#define RECURSION_LIMIT 222

/* Return true when the given line is the beginning of a paragraph (BOP). */
bool begpar(const filestruct *const line, int depth)
{
	size_t quote_len, indent_len, prev_dent_len;

	/* If this is the very first line of the buffer, it counts as a BOP
	 * even when it contains no text. */
	if (line == openfile->fileage)
		return true;

	/* If recursion is going too deep, just say it's not a BOP. */
	if (depth > RECURSION_LIMIT)
		return false;

	quote_len = quote_length(line->data);
	indent_len = indent_length(line->data + quote_len);

	/* If this line contains no text, it is not a BOP. */
	if (line->data[quote_len + indent_len] == '\0')
		return false;

	/* If the quote part of the preceding line differs, this is a BOP. */
	if (quote_len != quote_length(line->prev->data) ||
					strncmp(line->data, line->prev->data, quote_len) != 0)
		return true;

	prev_dent_len = indent_length(line->prev->data + quote_len);

	/* If the preceding line contains no text, this is a BOP. */
	if (line->prev->data[quote_len + prev_dent_len] == '\0')
		return true;

	/* If the indentation of the preceding line equals the indentation
	 * of this line, this is not a BOP. */
	if (prev_dent_len == indent_len && strncmp(line->prev->data + quote_len,
									line->data + quote_len, indent_len) == 0)
		return false;

	/* Otherwise, this is a BOP if the preceding line is not. */
	return !begpar(line->prev, depth + 1);
}

/* Return true when the given line is part of a paragraph: when it
 * contains something more than quoting and leading whitespace. */
bool inpar(const filestruct *const line)
{
	size_t quote_len = quote_length(line->data);
	size_t indent_len = indent_length(line->data + quote_len);

	return (line->data[quote_len + indent_len] != '\0');
}

/* Determine the beginning, length, and quoting of the first found paragraph.
 * Return true if we found a paragraph, and false otherwise.  Furthermore,
 * return in firstline the first line of the paragraph, in *quotelen the
 * length of the quoting, and in *parlen the length of the paragraph. */
bool find_paragraph(filestruct **firstline,
					size_t *const quotelen, size_t *const parlen)
{
	filestruct *line = *firstline;
		/* The line of the current paragraph we're searching in. */

	/* When not currently in a paragraph, move forward to a line that is. */
	while (!inpar(line) && line->next != NULL)
		line = line->next;

	*firstline = line;

	/* Move down to the last line of the paragraph. */
	do_para_end(&line);

	/* When not in a paragraph now, there aren't any paragraphs left. */
	if (!inpar(line))
		return false;

	/* We found a paragraph; determine length of quoting and number of lines. */
	*quotelen = quote_length((*firstline)->data);
	*parlen = line->lineno - (*firstline)->lineno + 1;

	return true;
}

/* Wrap all lines of the paragraph (that starts at *line, consists of
 * par_len lines, and has quote_len bytes of quoting) so they all fit
 * within the wrap_at target width. */
void justify_paragraph(filestruct **line, size_t quote_len, size_t par_len)
{
	filestruct *sampleline;
		/* The line from which the indentation is copied -- either
		 * the first and only or the second line of the paragraph. */
	size_t lead_len;
		/* Length of the quote part plus the indentation part. */
	ssize_t break_pos;
		/* The x-coordinate where the current line is to be broken. */
	char *lead_string;
		/* The quote+indent stuff that is copied from the sample line. */

	/* The sample line is either the only line or the second line. */
	sampleline = (par_len == 1 ? *line : (*line)->next);

	/* Copy the leading part (quoting + indentation) of the sample line. */
	lead_len = quote_len + indent_length(sampleline->data + quote_len);
	lead_string = mallocstrncpy(NULL, sampleline->data, lead_len + 1);
	lead_string[lead_len] = '\0';

	/* Now first tack all the lines of the paragraph together, skipping
	 * the quoting and indentation on all lines after the first. */
	while (par_len > 1) {
		filestruct *next_line = (*line)->next;
		size_t line_len = strlen((*line)->data);
		size_t next_line_len = strlen(next_line->data);

		lead_len = quote_len + indent_length(next_line->data + quote_len);

		/* We're just about to tack the next line onto this one.  If
		 * this line isn't empty, make sure it ends in a space. */
		if (line_len > 0 && (*line)->data[line_len - 1] != ' ') {
			(*line)->data = charealloc((*line)->data, line_len + 2);
			(*line)->data[line_len++] = ' ';
			(*line)->data[line_len] = '\0';
		}

		(*line)->data = charealloc((*line)->data,
									line_len + next_line_len - lead_len + 1);
		strcat((*line)->data, next_line->data + lead_len);

		unlink_node(next_line);
		par_len--;
	}

	/* Change all blank characters to spaces and remove excess spaces. */
	justify_format(*line, quote_len + indent_length((*line)->data + quote_len));

	/* Now break this long line into pieces that each fit with wrap_at columns. */
	while (strlenpt((*line)->data) > wrap_at) {
		size_t line_len = strlen((*line)->data);

		/* Find a point in the line where it can be broken. */
		break_pos = break_line((*line)->data + lead_len,
						wrap_at - strnlenpt((*line)->data, lead_len), false);

		/* If we can't break the line, or don't need to, we're done. */
		if (break_pos == -1 || break_pos + lead_len == line_len)
			break;

		/* Adjust the breaking position for the leading part and
		 * move it beyond the found whitespace character. */
		break_pos += lead_len + 1;

		/* Insert a new line after the current one, and copy the leading part
		 * plus the text after the breaking point into it. */
		splice_node(*line, make_new_node(*line));
		(*line)->next->data = charalloc(lead_len + line_len - break_pos + 1);
		strncpy((*line)->next->data, lead_string, lead_len);
		strcpy((*line)->next->data + lead_len, (*line)->data + break_pos);

		/* When requested, snip all trailing blanks. */
		if (ISSET(TRIM_BLANKS)) {
			while (break_pos > 0 &&
						is_blank_mbchar(&(*line)->data[break_pos - 1]))
				break_pos--;
		}

		/* Now actually break the current line, and go to the next. */
		null_at(&(*line)->data, break_pos);
		*line = (*line)->next;
	}

	free(lead_string);

	/* When possible, go to the line after the rewrapped paragraph. */
	if ((*line)->next != NULL)
		*line = (*line)->next;
}

/* Justify the current paragraph, and justify the entire file when
 * full_justify is true. */
void do_justify(bool full_justify)
{
	size_t quote_len;
		/* Length of the quote part of the current paragraph. */
	size_t par_len;
		/* Number of lines in the current paragraph. */
	filestruct *first_par_line = NULL;
		/* Will be the first line of the justified paragraph(s), if any. */
	filestruct *last_par_line = NULL;
		/* Will be the line after the last line of the justified
		 * paragraph(s), if any. */
	size_t x_for_last;
		/* The x position until where to extract the last paragraph line. */

	filestruct *was_cutbuffer = cutbuffer;
		/* The old cutbuffer, so we can justify in the current cutbuffer. */
	filestruct *was_cutbottom = cutbottom;
		/* The old cutbottom, so we can justify in the current cutbuffer. */
	size_t jus_len;
		/* The number of lines we're storing in the current cutbuffer. */
	filestruct *jusline;
		/* The line that we're justifying in the current cutbuffer. */

#ifndef NANO_TINY
	/* Stash the cursor position, to be stored in the undo item. */
	ssize_t was_lineno = openfile->current->lineno;
	size_t was_current_x = openfile->current_x;
#endif

	/* When justifying the entire buffer, start at the top.  Otherwise, when
	 * in a paragraph but not at its beginning, move back to its first line. */
	if (full_justify)
		openfile->current = openfile->fileage;
	else if (inpar(openfile->current) && !begpar(openfile->current, 0))
		do_para_begin(&openfile->current);

	/* Find the first line of the paragraph(s) to be justified.  If the
	 * search fails, there is nothing to justify, and we will be on the
	 * last line of the file, so put the cursor at the end of it. */
	if (!find_paragraph(&openfile->current, &quote_len, &par_len)) {
		openfile->current_x = strlen(openfile->filebot->data);
		refresh_needed = true;
		return;
	}

	/* We cannot (yet) justify a marked region, so turn the mark off. */
	openfile->mark = NULL;

	/* Prepare to put the text we want to justify in the cutbuffer. */
	cutbuffer = NULL;
	cutbottom = NULL;

	/* Start out at the first line of the paragraph. */
	first_par_line = openfile->current;
	last_par_line = openfile->current;

	/* Set the number of lines to be pulled into the cutbuffer. */
	if (full_justify) {
		jus_len = openfile->filebot->lineno;
	} else
		jus_len = par_len;

	/* Move down to the last line to be extracted. */
	for (; jus_len > 1; jus_len--)
		last_par_line = last_par_line->next;

	/* When possible, step one line further; otherwise, to line's end. */
	if (last_par_line->next != NULL) {
		last_par_line = last_par_line->next;
		x_for_last = 0;
	} else
		x_for_last = strlen(last_par_line->data);

#ifndef NANO_TINY
	add_undo(COUPLE_BEGIN);
	openfile->undotop->strdata = mallocstrcpy(NULL, _("justify"));

	/* Store the original cursor position, in case we unjustify. */
	openfile->undotop->lineno = was_lineno;
	openfile->undotop->begin = was_current_x;

	add_undo(CUT);
#endif
	/* Do the equivalent of a marked cut. */
	extract_buffer(&cutbuffer, &cutbottom, first_par_line, 0, last_par_line,
																x_for_last);
#ifndef NANO_TINY
	update_undo(CUT);
#endif

	/* Prepare to justify the text we just put in the cutbuffer. */
	jusline = cutbuffer;

	/* Justify the current paragraph. */
	justify_paragraph(&jusline, quote_len, par_len);

	/* When justifying the entire buffer, find and justify all paragraphs. */
	if (full_justify) {
		while (find_paragraph(&jusline, &quote_len, &par_len)) {
			justify_paragraph(&jusline, quote_len, par_len);

			if (jusline->next == NULL)
				break;
		}
	}

#ifndef NANO_TINY
	add_undo(PASTE);
#endif
	/* Do the equivalent of a paste of the justified text. */
	ingraft_buffer(cutbuffer);
#ifndef NANO_TINY
	update_undo(PASTE);

	add_undo(COUPLE_END);
	openfile->undotop->strdata = mallocstrcpy(NULL, _("justify"));
#endif

	/* We're done justifying.  Restore the old cutbuffer. */
	cutbuffer = was_cutbuffer;
	cutbottom = was_cutbottom;

	/* Show what we justified on the status bar. */
	if (full_justify)
		statusbar(_("Justified file"));
	else
		statusbar(_("Justified paragraph"));

	/* Set the desired screen column (always zero, except at EOF). */
	openfile->placewewant = xplustabs();

	set_modified();
	refresh_needed = true;
}

/* Justify the current paragraph. */
void do_justify_void(void)
{
	do_justify(false);
}

/* Justify the entire file. */
void do_full_justify(void)
{
	do_justify(true);
}
#endif /* ENABLE_JUSTIFY */

#if defined(ENABLE_SPELLER) || defined (ENABLE_COLOR)
/* Set up an argument list for executing the given command. */
void construct_argument_list(char ***arguments, char *command, char *filename)
{
	char *copy_of_command = mallocstrcpy(NULL, command);
	char *element = strtok(copy_of_command, " ");
	int count = 2;

	while (element != NULL) {
		*arguments = (char **)nrealloc(*arguments, ++count * sizeof(char *));
		(*arguments)[count - 3] = element;
		element = strtok(NULL, " ");
	}

	(*arguments)[count - 2] = filename;
	(*arguments)[count - 1] = NULL;
}
#endif

#ifdef ENABLE_SPELLER
/* Let the user edit the misspelled word.  Return false if the user cancels. */
bool fix_spello(const char *word)
{
	char *save_search;
	size_t firstcolumn_save = openfile->firstcolumn;
	size_t current_x_save = openfile->current_x;
	filestruct *edittop_save = openfile->edittop;
	filestruct *current_save = openfile->current;
		/* Save where we are. */
	bool proceed = false;
		/* The return value of this function. */
	bool result;
		/* The return value of searching for a misspelled word. */
#ifndef NANO_TINY
	bool right_side_up = false;
		/* true if (mark_begin, mark_begin_x) is the top of the mark,
		 * false if (current, current_x) is. */
	filestruct *top, *bot;
	size_t top_x, bot_x;
#endif

	/* Save the current search string, then set it to the misspelled word. */
	save_search = last_search;
	last_search = mallocstrcpy(NULL, word);

#ifndef NANO_TINY
	/* If the mark is on, start at the beginning of the marked region. */
	if (openfile->mark) {
		mark_order((const filestruct **)&top, &top_x,
						(const filestruct **)&bot, &bot_x, &right_side_up);
		/* If the region is marked normally, swap the end points, so that
		 * (current, current_x) (where searching starts) is at the top. */
		if (right_side_up) {
			openfile->current = top;
			openfile->current_x = top_x;
			openfile->mark = bot;
			openfile->mark_x = bot_x;
		}
	} else
#endif
	/* Otherwise, start from the top of the file. */
	{
		openfile->current = openfile->fileage;
		openfile->current_x = 0;
	}

	/* Find the first whole occurrence of word. */
	result = findnextstr(word, true, INREGION, NULL, false, NULL, 0);

	/* If the word isn't found, alert the user; if it is, allow correction. */
	if (result == 0) {
		statusline(ALERT, _("Unfindable word: %s"), word);
		lastmessage = HUSH;
		proceed = true;
		napms(2800);
	} else if (result == 1) {
		spotlighted = true;
		light_from_col = xplustabs();
		light_to_col = light_from_col + strlenpt(word);
#ifndef NANO_TINY
		filestruct *saved_mark = openfile->mark;
		openfile->mark = NULL;
#endif
		edit_refresh();

		/* Let the user supply a correctly spelled alternative. */
		proceed = (do_prompt(false, false, MSPELL, word, NULL,
								edit_refresh, _("Edit a replacement")) != -1);

		spotlighted = false;

#ifndef NANO_TINY
		openfile->mark = saved_mark;
#endif

		/* If a replacement was given, go through all occurrences. */
		if (proceed && strcmp(word, answer) != 0) {
			do_replace_loop(word, true, current_save, &current_x_save);

			/* TRANSLATORS: Shown after fixing misspellings in one word. */
			statusbar(_("Next word..."));
			napms(400);
		}
	}

#ifndef NANO_TINY
	if (openfile->mark) {
		/* Restore the (compensated) end points of the marked region. */
		if (right_side_up) {
			openfile->current = openfile->mark;
			openfile->current_x = openfile->mark_x;
			openfile->mark = top;
			openfile->mark_x = top_x;
		} else {
			openfile->current = top;
			openfile->current_x = top_x;
		}
	} else
#endif
	{
		/* Restore the (compensated) cursor position. */
		openfile->current = current_save;
		openfile->current_x = current_x_save;
	}

	/* Restore the string that was last searched for. */
	free(last_search);
	last_search = save_search;

	/* Restore the viewport to where it was. */
	openfile->edittop = edittop_save;
	openfile->firstcolumn = firstcolumn_save;

	return proceed;
}

/* Internal (integrated) spell checking using the spell program,
 * filtered through the sort and uniq programs.  Return NULL for normal
 * termination, and the error string otherwise. */
const char *do_int_speller(const char *tempfile_name)
{
	char *read_buff, *read_buff_ptr, *read_buff_word;
	size_t pipe_buff_size, read_buff_size, read_buff_read, bytesread;
	int spell_fd[2], sort_fd[2], uniq_fd[2], tempfile_fd = -1;
	pid_t pid_spell, pid_sort, pid_uniq;
	int spell_status, sort_status, uniq_status;

	/* Create all three pipes up front. */
	if (pipe(spell_fd) == -1 || pipe(sort_fd) == -1 || pipe(uniq_fd) == -1)
		return _("Could not create pipe");

	statusbar(_("Creating misspelled word list, please wait..."));

	/* A new process to run spell in. */
	if ((pid_spell = fork()) == 0) {
		/* Child continues (i.e. future spell process). */
		close(spell_fd[0]);

		/* Replace the standard input with the temp file. */
		if ((tempfile_fd = open(tempfile_name, O_RDONLY)) == -1)
			goto close_pipes_and_exit;

		if (dup2(tempfile_fd, STDIN_FILENO) != STDIN_FILENO) {
			close(tempfile_fd);
			goto close_pipes_and_exit;
		}

		close(tempfile_fd);

		/* Send spell's standard output to the pipe. */
		if (dup2(spell_fd[1], STDOUT_FILENO) != STDOUT_FILENO)
			goto close_pipes_and_exit;

		close(spell_fd[1]);

		/* Start the spell program; we are using $PATH. */
		execlp("spell", "spell", NULL);

		/* This should not be reached if spell is found. */
		exit(1);
	}

	/* Parent continues here. */
	close(spell_fd[1]);

	/* A new process to run sort in. */
	if ((pid_sort = fork()) == 0) {
		/* Child continues (i.e. future sort process).  Replace the
		 * standard input with the standard output of the old pipe. */
		if (dup2(spell_fd[0], STDIN_FILENO) != STDIN_FILENO)
			goto close_pipes_and_exit;

		close(spell_fd[0]);

		/* Send sort's standard output to the new pipe. */
		if (dup2(sort_fd[1], STDOUT_FILENO) != STDOUT_FILENO)
			goto close_pipes_and_exit;

		close(sort_fd[1]);

		/* Start the sort program.  Use -f to ignore case. */
		execlp("sort", "sort", "-f", NULL);

		/* This should not be reached if sort is found. */
		exit(1);
	}

	close(spell_fd[0]);
	close(sort_fd[1]);

	/* A new process to run uniq in. */
	if ((pid_uniq = fork()) == 0) {
		/* Child continues (i.e. future uniq process).  Replace the
		 * standard input with the standard output of the old pipe. */
		if (dup2(sort_fd[0], STDIN_FILENO) != STDIN_FILENO)
			goto close_pipes_and_exit;

		close(sort_fd[0]);

		/* Send uniq's standard output to the new pipe. */
		if (dup2(uniq_fd[1], STDOUT_FILENO) != STDOUT_FILENO)
			goto close_pipes_and_exit;

		close(uniq_fd[1]);

		/* Start the uniq program; we are using PATH. */
		execlp("uniq", "uniq", NULL);

		/* This should not be reached if uniq is found. */
		exit(1);
	}

	close(sort_fd[0]);
	close(uniq_fd[1]);

	/* The child process was not forked successfully. */
	if (pid_spell < 0 || pid_sort < 0 || pid_uniq < 0) {
		close(uniq_fd[0]);
		return _("Could not fork");
	}

	/* Get the system pipe buffer size. */
	if ((pipe_buff_size = fpathconf(uniq_fd[0], _PC_PIPE_BUF)) < 1) {
		close(uniq_fd[0]);
		return _("Could not get size of pipe buffer");
	}

	/* Read in the returned spelling errors. */
	read_buff_read = 0;
	read_buff_size = pipe_buff_size + 1;
	read_buff = read_buff_ptr = charalloc(read_buff_size);

	while ((bytesread = read(uniq_fd[0], read_buff_ptr, pipe_buff_size)) > 0) {
		read_buff_read += bytesread;
		read_buff_size += pipe_buff_size;
		read_buff = read_buff_ptr = charealloc(read_buff, read_buff_size);
		read_buff_ptr += read_buff_read;
	}

	*read_buff_ptr = '\0';
	close(uniq_fd[0]);

	/* Do any replacements case sensitive, forward, and without regexes. */
	SET(CASE_SENSITIVE);
	UNSET(BACKWARDS_SEARCH);
	UNSET(USE_REGEXP);

	read_buff_word = read_buff_ptr = read_buff;

	/* Process each of the misspelled words. */
	while (*read_buff_ptr != '\0') {
		if ((*read_buff_ptr == '\r') || (*read_buff_ptr == '\n')) {
			*read_buff_ptr = '\0';
			if (read_buff_word != read_buff_ptr) {
				if (!fix_spello(read_buff_word)) {
					read_buff_word = read_buff_ptr;
					break;
				}
			}
			read_buff_word = read_buff_ptr + 1;
		}
		read_buff_ptr++;
	}

	/* Special case: the last word doesn't end with '\r' or '\n'. */
	if (read_buff_word != read_buff_ptr)
		fix_spello(read_buff_word);

	free(read_buff);
	tidy_up_after_search();
	refresh_needed = true;

	/* Process the end of the three processes. */
	waitpid(pid_spell, &spell_status, 0);
	waitpid(pid_sort, &sort_status, 0);
	waitpid(pid_uniq, &uniq_status, 0);

	if (WIFEXITED(spell_status) == 0 || WEXITSTATUS(spell_status))
		return _("Error invoking \"spell\"");

	if (WIFEXITED(sort_status) == 0 || WEXITSTATUS(sort_status))
		return _("Error invoking \"sort -f\"");

	if (WIFEXITED(uniq_status) == 0 || WEXITSTATUS(uniq_status))
		return _("Error invoking \"uniq\"");

	/* When all went okay. */
	return NULL;

  close_pipes_and_exit:
	/* Don't leak any handles. */
	close(spell_fd[0]);
	close(spell_fd[1]);
	close(sort_fd[0]);
	close(sort_fd[1]);
	close(uniq_fd[0]);
	close(uniq_fd[1]);
	exit(1);
}

/* External (alternate) spell checking.  Return NULL for normal
 * termination, and the error string otherwise. */
const char *do_alt_speller(char *tempfile_name)
{
	int alt_spell_status;
	size_t current_x_save = openfile->current_x;
	size_t pww_save = openfile->placewewant;
	ssize_t lineno_save = openfile->current->lineno;
	bool was_at_eol = (openfile->current->data[openfile->current_x] == '\0');
	struct stat spellfileinfo;
	time_t timestamp;
	pid_t pid_spell;
	static char **spellargs = NULL;

	/* Get the timestamp and the size of the temporary file. */
	stat(tempfile_name, &spellfileinfo);
	timestamp = spellfileinfo.st_mtime;

	/* If the number of bytes to check is zero, get out. */
	if (spellfileinfo.st_size == 0)
		return NULL;

	/* Exit from curses mode. */
	endwin();

	construct_argument_list(&spellargs, alt_speller, tempfile_name);

	/* Fork a child process and run the alternate spell program in it. */
	if ((pid_spell = fork()) == 0) {
		execvp(spellargs[0], spellargs);

		/* Terminate the child process if no alternate speller is found. */
		exit(1);
	} else if (pid_spell < 0)
		return _("Could not fork");

	/* Wait for the alternate spell checker to finish. */
	wait(&alt_spell_status);

	/* Reenter curses mode. */
	doupdate();

	/* Restore the terminal to its previous state. */
	terminal_init();

	if (!WIFEXITED(alt_spell_status) || WEXITSTATUS(alt_spell_status) != 0)
		return invocation_error(alt_speller);

	/* Stat the temporary file again. */
	stat(tempfile_name, &spellfileinfo);

	/* Use the spell-checked file only when it changed. */
	if (spellfileinfo.st_mtime != timestamp) {
#ifndef NANO_TINY
		/* Replace the marked text (or entire text) with the corrected text. */
		if (openfile->mark) {
			filestruct *top, *bot;
			size_t top_x, bot_x;
			bool right_side_up;
			ssize_t was_mark_lineno = openfile->mark->lineno;

			mark_order((const filestruct **)&top, &top_x,
							(const filestruct **)&bot, &bot_x, &right_side_up);

			replace_marked_buffer(tempfile_name);

			/* Adjust the end point of the marked region for any change in
			 * length of the region's last line. */
			if (right_side_up)
				current_x_save = openfile->current_x;
			else
				openfile->mark_x = openfile->current_x;

			/* Restore the mark. */
			openfile->mark = fsfromline(was_mark_lineno);
		} else
#endif
			replace_buffer(tempfile_name);

		/* Go back to the old position. */
		goto_line_posx(lineno_save, current_x_save);
		if (was_at_eol || openfile->current_x > strlen(openfile->current->data))
			openfile->current_x = strlen(openfile->current->data);
#ifndef NANO_TINY
		update_undo(COUPLE_END);
#endif
		openfile->placewewant = pww_save;
		adjust_viewport(STATIONARY);
	}

	return NULL;
}

/* Spell check the current file.  If an alternate spell checker is
 * specified, use it.  Otherwise, use the internal spell checker. */
void do_spell(void)
{
	bool status;
	FILE *temp_file;
	char *temp;
	unsigned stash[sizeof(flags) / sizeof(flags[0])];
		/* A storage place for the current flag settings. */
	const char *result_msg;

	if (ISSET(RESTRICTED)) {
		show_restricted_warning();
		return;
	}

	temp = safe_tempfile(&temp_file);

	if (temp == NULL) {
		statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
		return;
	}

	/* Save the settings of the global flags. */
	memcpy(stash, flags, sizeof(flags));

	/* Don't add an extra newline when writing out the (selected) text. */
	SET(NO_NEWLINES);

#ifndef NANO_TINY
	if (openfile->mark)
		status = write_marked_file(temp, temp_file, true, OVERWRITE);
	else
#endif
		status = write_file(temp, temp_file, true, OVERWRITE, true);

	if (!status) {
		statusline(ALERT, _("Error writing temp file: %s"), strerror(errno));
		free(temp);
		return;
	}

	blank_bottombars();

	result_msg = (alt_speller ? do_alt_speller(temp) : do_int_speller(temp));

	unlink(temp);
	free(temp);

	/* Restore the settings of the global flags. */
	memcpy(flags, stash, sizeof(flags));

	/* If the spell-checker printed any error messages onscreen, make
	 * sure that they're cleared off. */
	total_refresh();

	if (result_msg != NULL) {
		if (errno == 0)
			/* Don't display an error message of "Success". */
			statusline(ALERT, _("Spell checking failed: %s"), result_msg);
		else
			statusline(ALERT, _("Spell checking failed: %s: %s"), result_msg,
												strerror(errno));
	} else
		statusbar(_("Finished checking spelling"));
}
#endif /* ENABLE_SPELLER */

#ifdef ENABLE_COLOR
/* Run a linting program on the current buffer.  Return NULL for normal
 * termination, and the error string otherwise. */
void do_linter(void)
{
	char *read_buff, *read_buff_ptr, *read_buff_word;
	size_t pipe_buff_size, read_buff_size, read_buff_read, bytesread;
	size_t parsesuccess = 0;
	int lint_status, lint_fd[2];
	pid_t pid_lint;
	bool helpless = ISSET(NO_HELP);
	static char **lintargs = NULL;
	lintstruct *lints = NULL, *tmplint = NULL, *curlint = NULL;
	time_t last_wait = 0;

	if (ISSET(RESTRICTED)) {
		show_restricted_warning();
		return;
	}

	if (!openfile->syntax || !openfile->syntax->linter) {
		statusbar(_("No linter defined for this type of file!"));
		return;
	}

#ifndef NANO_TINY
	openfile->mark = NULL;
#endif
	edit_refresh();

	if (openfile->modified) {
		int i = do_yesno_prompt(false, _("Save modified buffer before linting?"));

		if (i == -1) {
			statusbar(_("Cancelled"));
			return;
		} else if (i == 1 && (do_writeout(false, false) != 1))
			return;
	}

	/* Create a pipe up front. */
	if (pipe(lint_fd) == -1) {
		statusbar(_("Could not create pipe"));
		return;
	}

	blank_bottombars();
	currmenu = MLINTER;
	statusbar(_("Invoking linter, please wait"));

	construct_argument_list(&lintargs, openfile->syntax->linter, openfile->filename);

	/* Start a new process to run the linter in. */
	if ((pid_lint = fork()) == 0) {

		/* Child continues here (i.e. the future linting process). */
		close(lint_fd[0]);

		/* Send the linter's standard output + err to the pipe. */
		if (dup2(lint_fd[1], STDOUT_FILENO) != STDOUT_FILENO)
			exit(9);
		if (dup2(lint_fd[1], STDERR_FILENO) != STDERR_FILENO)
			exit(9);

		close(lint_fd[1]);

		/* Start the linter program; we are using $PATH. */
		execvp(lintargs[0], lintargs);

		/* This is only reached when the linter is not found. */
		exit(9);
	}

	/* Parent continues here. */
	close(lint_fd[1]);

	/* If the child process was not forked successfully... */
	if (pid_lint < 0) {
		close(lint_fd[0]);
		statusbar(_("Could not fork"));
		return;
	}

	/* Get the system pipe buffer size. */
	if ((pipe_buff_size = fpathconf(lint_fd[0], _PC_PIPE_BUF)) < 1) {
		close(lint_fd[0]);
		statusbar(_("Could not get size of pipe buffer"));
		return;
	}

	/* Read in the returned syntax errors. */
	read_buff_read = 0;
	read_buff_size = pipe_buff_size + 1;
	read_buff = read_buff_ptr = charalloc(read_buff_size);

	while ((bytesread = read(lint_fd[0], read_buff_ptr, pipe_buff_size)) > 0) {
		read_buff_read += bytesread;
		read_buff_size += pipe_buff_size;
		read_buff = read_buff_ptr = charealloc(read_buff, read_buff_size);
		read_buff_ptr += read_buff_read;
	}

	*read_buff_ptr = '\0';
	close(lint_fd[0]);

	/* Process the linter output. */
	read_buff_word = read_buff_ptr = read_buff;

	while (*read_buff_ptr != '\0') {
		if ((*read_buff_ptr == '\r') || (*read_buff_ptr == '\n')) {
			*read_buff_ptr = '\0';
			if (read_buff_word != read_buff_ptr) {
				char *filename = NULL, *linestr = NULL, *maybecol = NULL;
				char *message = mallocstrcpy(NULL, read_buff_word);

				/* At the moment we handle the following formats:
				 *
				 * filenameorcategory:line:column:message (e.g. splint)
				 * filenameorcategory:line,column:message (e.g. pylint)
				 * filenameorcategory:line:message        (e.g. pyflakes) */
				if (strstr(message, ": ") != NULL) {
					filename = strtok(read_buff_word, ":");
					if ((linestr = strtok(NULL, ":")) != NULL) {
						if ((maybecol = strtok(NULL, ":")) != NULL) {
							ssize_t tmplineno = 0, tmpcolno = 0;
							char *tmplinecol;

							tmplineno = strtol(linestr, NULL, 10);
							if (tmplineno <= 0) {
								read_buff_ptr++;
								free(message);
								continue;
							}

							tmpcolno = strtol(maybecol, NULL, 10);
							/* Check if the middle field is in comma format. */
							if (tmpcolno <= 0) {
								strtok(linestr, ",");
								if ((tmplinecol = strtok(NULL, ",")) != NULL)
									tmpcolno = strtol(tmplinecol, NULL, 10);
								else
									tmpcolno = 1;
							}

							/* Nice.  We have a lint message we can use. */
							parsesuccess++;
							tmplint = curlint;
							curlint = nmalloc(sizeof(lintstruct));
							curlint->next = NULL;
							curlint->prev = tmplint;
							if (curlint->prev != NULL)
								curlint->prev->next = curlint;
							curlint->msg = mallocstrcpy(NULL, message);
							curlint->lineno = tmplineno;
							curlint->colno = tmpcolno;
							curlint->filename = mallocstrcpy(NULL, filename);

							if (lints == NULL)
								lints = curlint;
						}
					}
				} else
					free(message);
			}
			read_buff_word = read_buff_ptr + 1;
		}
		read_buff_ptr++;
	}

	/* Process the end of the linting process. */
	waitpid(pid_lint, &lint_status, 0);

	if (!WIFEXITED(lint_status) || WEXITSTATUS(lint_status) > 2) {
		statusbar(invocation_error(openfile->syntax->linter));
		return;
	}

	free(read_buff);

	if (parsesuccess == 0) {
		statusline(HUSH, _("Got 0 parsable lines from command: %s"),
						openfile->syntax->linter);
		return;
	}

	if (helpless && LINES > 4) {
		UNSET(NO_HELP);
		window_init();
	}

	/* Show that we are in the linter now. */
	titlebar(NULL);
	bottombars(MLINTER);

	tmplint = NULL;
	curlint = lints;

	while (true) {
		int kbinput;
		functionptrtype func;
		struct stat lintfileinfo;

		if (stat(curlint->filename, &lintfileinfo) != -1 &&
					(openfile->current_stat == NULL ||
					openfile->current_stat->st_ino != lintfileinfo.st_ino)) {
#ifdef ENABLE_MULTIBUFFER
			const openfilestruct *started_at = openfile;

			openfile = openfile->next;
			while (openfile != started_at && (openfile->current_stat == NULL ||
						openfile->current_stat->st_ino != lintfileinfo.st_ino))
				openfile = openfile->next;

			if (openfile->current_stat == NULL ||
						openfile->current_stat->st_ino != lintfileinfo.st_ino) {
				char *msg = charalloc(1024 + strlen(curlint->filename));
				int i;

				sprintf(msg, _("This message is for unopened file %s,"
							" open it in a new buffer?"), curlint->filename);
				i = do_yesno_prompt(false, msg);
				currmenu = MLINTER;
				free(msg);

				if (i == -1) {
					statusbar(_("Cancelled"));
					break;
				} else if (i == 1) {
					open_buffer(curlint->filename, true);
				} else {
#endif
					char *dontwantfile = mallocstrcpy(NULL, curlint->filename);
					lintstruct *restlint = NULL;

					while (curlint != NULL) {
						if (strcmp(curlint->filename, dontwantfile) == 0) {
							if (curlint == lints)
								lints = curlint->next;
							else
								curlint->prev->next = curlint->next;
							if (curlint->next != NULL)
								curlint->next->prev = curlint->prev;
							tmplint = curlint;
							curlint = curlint->next;
							free(tmplint->msg);
							free(tmplint->filename);
							free(tmplint);
						} else {
							if (restlint == NULL)
								restlint = curlint;
							curlint = curlint->next;
						}
					}

					free(dontwantfile);

					if (restlint == NULL) {
						statusbar(_("No messages for this file"));
						break;
					} else {
						curlint = restlint;
						continue;
					}
#ifdef ENABLE_MULTIBUFFER
				}
			}
#endif
		}

		if (tmplint != curlint) {
			goto_line_posx(curlint->lineno, curlint->colno - 1);
			titlebar(NULL);
			adjust_viewport(CENTERING);
#ifdef ENABLE_LINENUMBERS
			confirm_margin();
#endif
			edit_refresh();
			statusline(NOTICE, curlint->msg);
			bottombars(MLINTER);
		}

		/* Place the cursor to indicate the affected line. */
		place_the_cursor();
		wnoutrefresh(edit);

		kbinput = get_kbinput(bottomwin, VISIBLE);

#ifndef NANO_TINY
		if (kbinput == KEY_WINCH)
			continue;
#endif
		func = func_from_key(&kbinput);
		tmplint = curlint;

		if (func == do_cancel || func == do_enter) {
			wipe_statusbar();
			break;
		} else if (func == do_help_void) {
			tmplint = NULL;
			do_help_void();
		} else if (func == do_page_up || func == do_prev_block) {
			if (curlint->prev != NULL)
				curlint = curlint->prev;
			else if (last_wait != time(NULL)) {
				statusbar(_("At first message"));
				napms(600);
				last_wait = time(NULL);
				statusline(NOTICE, curlint->msg);
			}
		} else if (func == do_page_down || func == do_next_block) {
			if (curlint->next != NULL)
				curlint = curlint->next;
			else if (last_wait != time(NULL)) {
				statusbar(_("At last message"));
				napms(600);
				last_wait = time(NULL);
				statusline(NOTICE, curlint->msg);
			}
		}
	}

	for (curlint = lints; curlint != NULL;) {
		tmplint = curlint;
		curlint = curlint->next;
		free(tmplint->msg);
		free(tmplint->filename);
		free(tmplint);
	}

	if (helpless) {
		SET(NO_HELP);
		window_init();
		refresh_needed = true;
	}

	currmenu = MMOST;
	titlebar(NULL);
}
#endif /* ENABLE_COLOR */

#ifndef NANO_TINY
/* Our own version of "wc".  Note that its character counts are in
 * multibyte characters instead of single-byte characters. */
void do_wordlinechar_count(void)
{
	size_t words = 0, chars = 0;
	ssize_t nlines = 0;
	size_t current_x_save = openfile->current_x;
	size_t pww_save = openfile->placewewant;
	filestruct *current_save = openfile->current;
	filestruct *was_mark = openfile->mark;
	filestruct *top, *bot;
	size_t top_x, bot_x;

	/* If the mark is on, partition the buffer so that it
	 * contains only the marked text, and turn the mark off. */
	if (was_mark) {
		mark_order((const filestruct **)&top, &top_x,
						(const filestruct **)&bot, &bot_x, NULL);
		filepart = partition_filestruct(top, top_x, bot, bot_x);
		openfile->mark = NULL;
	}

	/* Start at the top of the file. */
	openfile->current = openfile->fileage;
	openfile->current_x = 0;
	openfile->placewewant = 0;

	/* Keep moving to the next word (counting punctuation characters as
	 * part of a word, as "wc -w" does), without updating the screen,
	 * until we reach the end of the file, incrementing the total word
	 * count whenever we're on a word just before moving. */
	while (openfile->current != openfile->filebot ||
		openfile->current->data[openfile->current_x] != '\0') {
		if (do_next_word(false, true))
			words++;
	}

	/* Get the total line and character counts, as "wc -l"  and "wc -c"
	 * do, but get the latter in multibyte characters. */
	if (was_mark) {
		nlines = openfile->filebot->lineno - openfile->fileage->lineno + 1;
		chars = get_totsize(openfile->fileage, openfile->filebot);

		/* Unpartition the buffer so that it contains all the text
		 * again, and turn the mark back on. */
		unpartition_filestruct(&filepart);
		openfile->mark = was_mark;
	} else {
		nlines = openfile->filebot->lineno;
		chars = openfile->totsize;
	}

	/* Restore where we were. */
	openfile->current = current_save;
	openfile->current_x = current_x_save;
	openfile->placewewant = pww_save;

	/* Display the total word, line, and character counts on the statusbar. */
	statusline(HUSH, _("%sWords: %zu  Lines: %zd  Chars: %zu"), was_mark ?
						_("In Selection:  ") : "", words, nlines, chars);
}
#endif /* !NANO_TINY */

/* Get verbatim input. */
void do_verbatim_input(void)
{
	int *kbinput;
	size_t kbinput_len, i;
	char *output;

	/* TRANSLATORS: This is displayed when the next keystroke will be
	 * inserted verbatim. */
	statusbar(_("Verbatim Input"));
	place_the_cursor();

	/* Read in all the verbatim characters. */
	kbinput = get_verbatim_kbinput(edit, &kbinput_len);

	/* Unsuppress cursor-position display or blank the statusbar. */
	if (ISSET(CONSTANT_SHOW))
		suppress_cursorpos = false;
	else
		wipe_statusbar();

	/* Display all the verbatim characters at once, not filtering out
	 * control characters. */
	output = charalloc(kbinput_len + 1);

	for (i = 0; i < kbinput_len; i++)
		output[i] = (char)kbinput[i];
	output[i] = '\0';

	free(kbinput);

	do_output(output, kbinput_len, true);

	free(output);
}

#ifdef ENABLE_WORDCOMPLETION
/* Return a copy of the found completion candidate. */
char *copy_completion(char *text)
{
	char *word;
	size_t length = 0, index = 0;

	/* Find the end of the candidate word to get its length. */
	while (is_word_mbchar(&text[length], false))
		length = move_mbright(text, length);

	/* Now copy this candidate to a new string. */
	word = charalloc(length + 1);
	while (index < length)
		word[index++] = *(text++);
	word[index] = '\0';

	return word;
}

/* Look at the fragment the user has typed, then search the current buffer for
 * the first word that starts with this fragment, and tentatively complete the
 * fragment.  If the user types 'Complete' again, search and paste the next
 * possible completion. */
void complete_a_word(void)
{
	char *shard, *completion = NULL;
	size_t start_of_shard, shard_length = 0;
	size_t i = 0, j = 0;
	completion_word *some_word;
#ifdef ENABLE_WRAPPING
	bool was_set_wrapping = !ISSET(NO_WRAP);
#endif

	/* If this is a fresh completion attempt... */
	if (pletion_line == NULL) {
		/* Clear the list of words of a previous completion run. */
		while (list_of_completions != NULL) {
			completion_word *dropit = list_of_completions;
			list_of_completions = list_of_completions->next;
			free(dropit->word);
			free(dropit);
		}

		/* Prevent a completion from being merged with typed text. */
		openfile->last_action = OTHER;

		/* Initialize the starting point for searching. */
		pletion_line = openfile->fileage;
		pletion_x = 0;

		/* Wipe the "No further matches" message. */
		wipe_statusbar();
	} else {
		/* Remove the attempted completion from the buffer. */
		do_undo();
	}

	/* Find the start of the fragment that the user typed. */
	start_of_shard = openfile->current_x;
	while (start_of_shard > 0) {
		size_t step_left = move_mbleft(openfile->current->data, start_of_shard);

		if (!is_word_mbchar(&openfile->current->data[step_left], false))
			break;
		start_of_shard = step_left;
	}

	/* If there is no word fragment before the cursor, do nothing. */
	if (start_of_shard == openfile->current_x) {
		statusbar(_("No word fragment"));
		pletion_line = NULL;
		return;
	}

	shard = charalloc(openfile->current_x - start_of_shard + 1);

	/* Copy the fragment that has to be searched for. */
	while (start_of_shard < openfile->current_x)
		shard[shard_length++] = openfile->current->data[start_of_shard++];
	shard[shard_length] = '\0';

	/* Run through all of the lines in the buffer, looking for shard. */
	while (pletion_line != NULL) {
		ssize_t threshold = strlen(pletion_line->data) - shard_length - 1;
				/* The point where we can stop searching for shard. */

		/* Traverse the whole line, looking for shard. */
		for (i = pletion_x; (ssize_t)i < threshold; i++) {
			/* If the first byte doesn't match, run on. */
			if (pletion_line->data[i] != shard[0])
				continue;

			/* Compare the rest of the bytes in shard. */
			for (j = 1; j < shard_length; j++)
				if (pletion_line->data[i + j] != shard[j])
					break;

			/* If not all of the bytes matched, continue searching. */
			if (j < shard_length)
				continue;

			/* If the found match is not /longer/ than shard, skip it. */
			if (!is_word_mbchar(&pletion_line->data[i + j], false))
				continue;

			/* If the match is not a separate word, skip it. */
			if (i > 0 && is_word_mbchar(&pletion_line->data[
								move_mbleft(pletion_line->data, i)], false))
				continue;

			/* If this match is the shard itself, ignore it. */
			if (pletion_line == openfile->current &&
						i == openfile->current_x - shard_length)
				continue;

			completion = copy_completion(pletion_line->data + i);

			/* Look among earlier attempted completions for a duplicate. */
			some_word = list_of_completions;
			while (some_word && strcmp(some_word->word, completion) != 0)
				some_word = some_word->next;

			/* If we've already tried this word, skip it. */
			if (some_word != NULL) {
				free(completion);
				continue;
			}

			/* Add the found word to the list of completions. */
			some_word = (completion_word *)nmalloc(sizeof(completion_word));
			some_word->word = completion;
			some_word->next = list_of_completions;
			list_of_completions = some_word;

#ifdef ENABLE_WRAPPING
			/* Temporarily disable wrapping so only one undo item is added. */
			SET(NO_WRAP);
#endif
			/* Inject the completion into the buffer. */
			do_output(&completion[shard_length],
						strlen(completion) - shard_length, false);
#ifdef ENABLE_WRAPPING
			/* If needed, reenable wrapping and wrap the current line. */
			if (was_set_wrapping) {
				UNSET(NO_WRAP);
				do_wrap(openfile->current);
			}
#endif
			/* Set the position for a possible next search attempt. */
			pletion_x = ++i;

			free(shard);
			return;
		}

		pletion_line = pletion_line->next;
		pletion_x = 0;
	}

	/* The search has reached the end of the file. */
	if (list_of_completions != NULL) {
		statusline(ALERT, _("No further matches"));
		refresh_needed = true;
	} else
		/* TRANSLATORS: Shown when there are zero possible completions. */
		statusline(ALERT, _("No matches"));

	free(shard);
}
#endif /* ENABLE_WORDCOMPLETION */
