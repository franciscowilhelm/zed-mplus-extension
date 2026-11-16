/*
 * External scanner for tree-sitter-mplus.
 *
 * It produces `title_text`: one line of free text inside `TITLE:`. A title
 * runs until the next Mplus command, so a line that *starts* with a command
 * header (e.g. `DATA:`, `VARIABLE:`, `MODEL male:`) ends the title. Text on
 * the same line as `TITLE:` is always title text.
 *
 * No libc calls are used, so it builds for WebAssembly without a stdlib.
 */

#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum TokenType {
  TITLE_TEXT,
  ERROR_SENTINEL,
};

enum LabelKind {
  LABEL_NONE,   // command takes no second word
  LABEL_ALPHA,  // DATA IMPUTATION:, DATA WIDETOLONG:, ...
  LABEL_WORD,   // MODEL INDIRECT:, MODEL male:, MODEL g-2:, ...
};

typedef struct {
  const char *name;
  unsigned min_len;  // Mplus accepts commands shortened to four letters
  enum LabelKind label;
} Command;

static const Command COMMANDS[] = {
  {"TITLE", 4, LABEL_NONE},      {"DATA", 4, LABEL_ALPHA},
  {"VARIABLES", 4, LABEL_NONE},  {"DEFINE", 4, LABEL_NONE},
  {"ANALYSIS", 4, LABEL_NONE},   {"MODEL", 4, LABEL_WORD},
  {"MONTECARLO", 4, LABEL_NONE}, {"OUTPUT", 4, LABEL_NONE},
  {"SAVEDATA", 4, LABEL_NONE},   {"PLOT", 4, LABEL_NONE},
};

#define WORD_MAX 16

static bool is_alpha(int32_t c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_digit(int32_t c) { return c >= '0' && c <= '9'; }

static bool is_blank(int32_t c) { return c == ' ' || c == '\t'; }

static bool is_space(int32_t c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
}

static bool is_label_char(int32_t c, enum LabelKind kind) {
  if (kind == LABEL_ALPHA) return is_alpha(c);
  return is_alpha(c) || is_digit(c) || c == '_' || c == '.' || c == '-';
}

static char to_upper(int32_t c) {
  return (char)((c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c);
}

static const Command *match_command(const char *word, unsigned len) {
  for (unsigned i = 0; i < sizeof(COMMANDS) / sizeof(COMMANDS[0]); i++) {
    const Command *cmd = &COMMANDS[i];
    if (len < cmd->min_len) continue;
    unsigned j = 0;
    while (j < len && cmd->name[j] != '\0' && cmd->name[j] == word[j]) j++;
    if (j == len) return cmd;  // `word` is a prefix of the command name
  }
  return NULL;
}

typedef struct {
  TSLexer *lexer;
  bool has_text;
} Scan;

// Advance one character as part of the token; the token ends after the last
// non-space character so trailing blanks are left to `extras`.
static void consume(Scan *s) {
  int32_t c = s->lexer->lookahead;
  s->lexer->advance(s->lexer, false);
  if (!is_space(c)) {
    s->lexer->mark_end(s->lexer);
    s->has_text = true;
  }
}

static bool at_line_end(TSLexer *lexer) {
  return lexer->eof(lexer) || lexer->lookahead == '\n' || lexer->lookahead == '!';
}

void *tree_sitter_mplus_external_scanner_create(void) { return NULL; }

void tree_sitter_mplus_external_scanner_destroy(void *payload) { (void)payload; }

unsigned tree_sitter_mplus_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_mplus_external_scanner_deserialize(void *payload, const char *buffer,
                                                    unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

bool tree_sitter_mplus_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  (void)payload;
  if (valid_symbols[ERROR_SENTINEL] || !valid_symbols[TITLE_TEXT]) return false;

  bool line_start = false;
  while (!lexer->eof(lexer)) {
    int32_t c = lexer->lookahead;
    if (c == '\n') {
      line_start = true;
      lexer->advance(lexer, true);
    } else if (is_space(c)) {
      lexer->advance(lexer, true);
    } else {
      break;
    }
  }

  // Comments are lexed by the grammar as `extras`.
  if (at_line_end(lexer)) return false;

  lexer->result_symbol = TITLE_TEXT;
  Scan s = {lexer, false};

  // Does this line start a new command? If so, end the title here.
  if (line_start && is_alpha(lexer->lookahead)) {
    char word[WORD_MAX];
    unsigned len = 0;
    bool overflow = false;
    while (is_alpha(lexer->lookahead)) {
      if (len < WORD_MAX) {
        word[len++] = to_upper(lexer->lookahead);
      } else {
        overflow = true;
      }
      consume(&s);
    }

    const Command *cmd = overflow ? NULL : match_command(word, len);
    if (cmd) {
      bool spaced = false;
      while (is_blank(lexer->lookahead)) {
        spaced = true;
        consume(&s);
      }
      if (lexer->lookahead == ':') return false;

      if (cmd->label != LABEL_NONE && spaced &&
          is_label_char(lexer->lookahead, cmd->label)) {
        while (is_label_char(lexer->lookahead, cmd->label)) consume(&s);
        while (is_blank(lexer->lookahead)) consume(&s);
        if (lexer->lookahead == ':') return false;
      }
    }
  }

  while (!at_line_end(lexer)) consume(&s);

  return s.has_text;
}
