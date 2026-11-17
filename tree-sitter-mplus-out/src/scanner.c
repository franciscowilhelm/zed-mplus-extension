/*
 * External scanner for tree-sitter-mplus-out.
 *
 * Mplus output has no syntax of its own; what a line means follows from
 * where it starts and how it is written. The scanner only acts at the start
 * of a line and classifies it:
 *
 *   column 0, upper case        heading          MODEL FIT INFORMATION
 *                               input_heading    INPUT INSTRUCTIONS
 *                               banner           MUTHEN & MUTHEN
 *   column 0, mixed case        subheading       Chi-Square Test of Model Fit
 *   indented, upper case        subheading       PARAMETER SPECIFICATION
 *   column 1, no numbers        parameter_header  F1       BY / Residual Variances
 *   `*** ERROR ...`             error            with the indented text below it
 *   `*** WARNING ...`           warning          ditto
 *   upper-case sentences        notice           THE MODEL ESTIMATION TERMINATED NORMALLY
 *                               warning          if they report a problem
 *   after INPUT INSTRUCTIONS    input_text       the indented echo of the input
 *
 * Upper-case headings, notices and warnings cover their whole paragraph (the
 * following lines with the same indentation). Every other line is left to
 * the grammar, which lexes it as numbers and words.
 *
 * No libc calls are used, so it builds for WebAssembly without a stdlib.
 */

#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum TokenType {
  HEADING,
  INPUT_HEADING,
  BANNER,
  SUBHEADING,
  PARAMETER_HEADER,
  WARNING,
  ERROR,
  NOTICE,
  INPUT_TEXT,
  ERROR_SENTINEL,
};

#define TEXT_MAX 160

typedef struct {
  char text[TEXT_MAX + 1];  // start of the line, for keyword checks
  unsigned len;
  unsigned words;
  unsigned numeric_words;  // words after the first that start like a number
  char last_word[8];
  char last_char;
  bool has_lower;
  bool has_caps_word;  // two or more upper-case letters in a row
  bool has_gap;        // columns: two or more blanks inside the line
  bool has_sentence;   // `WORD.` followed by a blank or the end of the line
  bool has_colon;      // `: ` inside the line, or `:` at its end
} Line;

static bool is_lower(int32_t c) { return c >= 'a' && c <= 'z'; }
static bool is_upper(int32_t c) { return c >= 'A' && c <= 'Z'; }
static bool is_alpha(int32_t c) { return is_lower(c) || is_upper(c); }
static bool is_digit(int32_t c) { return c >= '0' && c <= '9'; }
static bool is_blank(int32_t c) { return c == ' ' || c == '\t' || c == '\r' || c == '\f'; }

static bool at_line_end(TSLexer *lexer) {
  return lexer->eof(lexer) || lexer->lookahead == '\n';
}

static bool starts_with(const char *text, const char *prefix) {
  while (*prefix) {
    if (*text++ != *prefix++) return false;
  }
  return true;
}

static bool equals(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return *a == *b;
}

static bool contains(const char *text, const char *needle) {
  for (; *text; text++) {
    if (starts_with(text, needle)) return true;
  }
  return false;
}

/*
 * Consume the rest of the current line (not the newline) and describe it.
 * With `mark`, the token end is moved to the last non-blank character.
 */
static void read_line(TSLexer *lexer, Line *line, bool mark) {
  line->len = 0;
  line->words = 0;
  line->numeric_words = 0;
  line->last_word[0] = '\0';
  line->last_char = '\0';
  line->has_lower = false;
  line->has_caps_word = false;
  line->has_gap = false;
  line->has_sentence = false;
  line->has_colon = false;

  unsigned blanks = 0;       // blanks since the last non-blank character
  unsigned letters = 0;      // length of the current run of letters
  unsigned caps = 0;         // length of the current run of upper-case letters
  unsigned word_len = 0;
  bool period_pending = false;  // saw `WORD.`, waiting for a blank
  int32_t prev = 0;             // last non-blank character

  while (!at_line_end(lexer)) {
    int32_t c = lexer->lookahead;
    lexer->advance(lexer, false);

    if (is_blank(c)) {
      if (period_pending) line->has_sentence = true;
      if (prev == ':') line->has_colon = true;
      period_pending = false;
      blanks++;
      letters = 0;
      caps = 0;
      if (line->len < TEXT_MAX && blanks == 1) line->text[line->len++] = ' ';
      continue;
    }

    if (mark) lexer->mark_end(lexer);

    if (blanks > 0 || prev == 0) {
      // A new word starts here.
      if (blanks >= 2 && prev != '.' && prev != ':') line->has_gap = true;
      line->words++;
      word_len = 0;
      if (line->words > 1) {
        bool sign = c == '-' || c == '+' || c == '.';
        if (is_digit(c) || (sign && is_digit(lexer->lookahead))) line->numeric_words++;
      }
    }
    blanks = 0;

    if (word_len < sizeof(line->last_word) - 1) {
      line->last_word[word_len++] = (char)(c < 128 ? c : '?');
      line->last_word[word_len] = '\0';
    } else {
      line->last_word[0] = '\0';  // too long to be one of the model keywords
      word_len = sizeof(line->last_word);
    }

    period_pending = c == '.' && letters >= 3;
    if (is_alpha(c)) {
      letters++;
      if (is_lower(c)) {
        line->has_lower = true;
        caps = 0;
      } else if (++caps >= 2) {
        line->has_caps_word = true;
      }
    } else {
      letters = 0;
      caps = 0;
    }

    if (line->len < TEXT_MAX) line->text[line->len++] = (char)(c < 128 ? c : '?');
    prev = c;
    line->last_char = (char)(c < 128 ? c : '?');
  }

  if (period_pending) line->has_sentence = true;
  if (prev == ':') line->has_colon = true;
  while (line->len > 0 && line->text[line->len - 1] == ' ') line->len--;
  line->text[line->len] = '\0';
}

/*
 * From the end of a line, move to the first character of the next one.
 * Returns false at the end of the file or if the next line is blank;
 * otherwise sets `indent` to that line's indentation.
 */
static bool next_line(TSLexer *lexer, unsigned *indent) {
  if (lexer->eof(lexer)) return false;
  lexer->advance(lexer, false);  // the newline
  unsigned n = 0;
  while (!lexer->eof(lexer) && is_blank(lexer->lookahead)) {
    lexer->advance(lexer, false);
    n++;
  }
  if (at_line_end(lexer)) return false;
  *indent = n;
  return true;
}

/* Upper-case text that reports a problem rather than a status. */
static bool reports_problem(const char *text) {
  static const char *const WORDS[] = {
    "WARNING",        "NO CONVERGENCE",        "NONCONVERGENCE", "NOT TERMINATE",
    "COULD NOT",      "NOT POSITIVE DEFINITE", "NON-POSITIVE",   "PROBLEM",
    "NOT IDENTIFIED", "NONIDENTIFIED",         "UNTRUSTWORTHY",  "NOT BE TRUSTWORTHY",
    "NOT CONVERGE",   "NONIDENTIFICATION",     "POORLY",         "SIZE FOR THE POSTERIOR",
    "NOT REPLICATED", "SINGULAR",
  };
  for (unsigned i = 0; i < sizeof(WORDS) / sizeof(WORDS[0]); i++) {
    if (contains(text, WORDS[i])) return true;
  }
  return false;
}

/* `F1 BY`, `Y1 ON`, `F1 WITH`, `S |`: the row that opens a block of estimates. */
static bool is_model_keyword(const char *word) {
  static const char *const WORDS[] = {"BY", "ON", "WITH", "PON", "PWITH", "XWITH", "|"};
  for (unsigned i = 0; i < sizeof(WORDS) / sizeof(WORDS[0]); i++) {
    if (equals(word, WORDS[i])) return true;
  }
  return false;
}

void *tree_sitter_mplus_out_external_scanner_create(void) { return NULL; }

void tree_sitter_mplus_out_external_scanner_destroy(void *payload) { (void)payload; }

unsigned tree_sitter_mplus_out_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_mplus_out_external_scanner_deserialize(void *payload, const char *buffer,
                                                        unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

/* The echoed input: every following line that is indented, blank lines included. */
static bool scan_input_text(TSLexer *lexer) {
  lexer->result_symbol = INPUT_TEXT;
  for (;;) {
    while (!at_line_end(lexer)) {
      int32_t c = lexer->lookahead;
      lexer->advance(lexer, false);
      if (!is_blank(c)) lexer->mark_end(lexer);
    }
    unsigned indent = 0;
    bool found = false;
    while (!lexer->eof(lexer)) {
      if (next_line(lexer, &indent)) {
        found = true;
        break;
      }
    }
    if (!found || indent == 0) return true;
  }
}

bool tree_sitter_mplus_out_external_scanner_scan(void *payload, TSLexer *lexer,
                                                 const bool *valid_symbols) {
  (void)payload;
  if (valid_symbols[ERROR_SENTINEL]) return false;

  // Only the first token of a line is of interest.
  bool line_start = lexer->get_column(lexer) == 0;
  while (!lexer->eof(lexer)) {
    int32_t c = lexer->lookahead;
    if (c == '\n') {
      line_start = true;
    } else if (!is_blank(c)) {
      break;
    }
    lexer->advance(lexer, true);
  }
  if (!line_start || lexer->eof(lexer)) return false;

  unsigned indent = lexer->get_column(lexer);
  int32_t first = lexer->lookahead;

  if (valid_symbols[INPUT_TEXT] && indent > 0) return scan_input_text(lexer);

  Line line;
  read_line(lexer, &line, true);

  // *** ERROR in DATA command / *** WARNING in MODEL command, plus their text.
  if (starts_with(line.text, "***")) {
    enum TokenType symbol = starts_with(line.text, "*** ERROR") ? ERROR : WARNING;
    unsigned next_indent = 0;
    Line rest;
    while (next_line(lexer, &next_indent) && next_indent > 0) read_line(lexer, &rest, true);
    lexer->result_symbol = symbol;
    return valid_symbols[symbol];
  }

  // Results tables:  ` F1       BY`, ` Means`, ` Residual Variances`
  if (indent == 1 && is_alpha(first) && line.numeric_words == 0 && !line.has_sentence &&
      !line.has_colon && line.words <= 4 &&
      ((line.has_lower && !line.has_gap) || is_model_keyword(line.last_word))) {
    lexer->result_symbol = PARAMETER_HEADER;
    return valid_symbols[PARAMETER_HEADER];
  }

  // Mixed-case titles at column 0:  `Information Criteria`, `Latent Class 1`
  if (line.has_lower) {
    if (indent == 0 && is_upper(first) && !line.has_gap && !line.has_colon &&
        line.last_char != '.' && line.last_char != ',') {
      lexer->result_symbol = SUBHEADING;
      return valid_symbols[SUBHEADING];
    }
    return false;
  }

  // From here on: lines without lower-case letters.
  bool counted = is_digit(first) && (contains(line.text, "WARNING(S)") ||
                                     contains(line.text, "ERROR(S)"));
  if (!line.has_caps_word || line.has_gap || !(is_upper(first) || counted)) return false;

  bool is_input = indent == 0 && equals(line.text, "INPUT INSTRUCTIONS");
  bool is_banner = indent == 0 && equals(line.text, "MUTHEN & MUTHEN");
  bool is_fit_index = indent == 0 && equals(line.text, "CFI/TLI");
  // A status line, as opposed to a heading.
  bool sentence = line.has_sentence || contains(line.text, "TERMINATED") ||
                  starts_with(line.text, "MODEL RESULTS USE ");
  // The upper-case end of a mixed-case paragraph:  `MODINDICES (ALL).`
  if (!sentence && line.last_char == '.') return false;
  bool problem = reports_problem(line.text);

  // The rest of the paragraph: following upper-case lines with the same
  // indentation. A line is read before it is known to belong, so the token
  // end is only moved once it does.
  if (!is_input && !is_banner && !is_fit_index) {
    unsigned next_indent = 0;
    while (next_line(lexer, &next_indent) && next_indent == indent) {
      Line rest;
      read_line(lexer, &rest, false);
      if (rest.has_lower || rest.has_gap || starts_with(rest.text, "***")) break;
      lexer->mark_end(lexer);
      sentence = sentence || rest.has_sentence || contains(rest.text, "TERMINATED");
      problem = problem || reports_problem(rest.text);
    }
  }

  enum TokenType symbol;
  if (problem) {
    symbol = WARNING;
  } else if (sentence) {
    symbol = NOTICE;
  } else if (counted) {
    return false;
  } else if (is_input) {
    symbol = INPUT_HEADING;
  } else if (is_banner) {
    symbol = BANNER;
  } else if (is_fit_index) {
    symbol = SUBHEADING;
  } else if (indent == 0) {
    symbol = HEADING;
  } else if (line.words >= 2) {
    symbol = SUBHEADING;
  } else {
    return false;
  }

  lexer->result_symbol = symbol;
  return valid_symbols[symbol];
}
