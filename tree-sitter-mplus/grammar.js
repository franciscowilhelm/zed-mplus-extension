/**
 * @file Tree-sitter grammar for Mplus input (.inp) files
 * @license MIT
 *
 * Mplus input files are made of commands ("sections") such as
 * `TITLE:`, `DATA:`, `VARIABLE:`, `ANALYSIS:`, `MODEL:`, `OUTPUT:` ...
 * Every statement inside a command ends with `;`, except in `TITLE:`,
 * which holds free text until the next command. Mplus is case-insensitive
 * and lets commands, options and settings be shortened to four letters.
 *
 * The grammar is deliberately permissive: it gives enough structure for
 * highlighting, outline and runnables, without trying to validate Mplus.
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

/** Case-insensitive regex source for a fixed word: "by" -> "[bB][yY]". */
function ci(word) {
  return word
    .split('')
    .map((c) => (/[a-z]/i.test(c) ? `[${c.toLowerCase()}${c.toUpperCase()}]` : c))
    .join('');
}

/**
 * Case-insensitive regex source for a command that Mplus lets you shorten
 * to its first `min` letters: abbrev("MODEL", 4) -> "[mM][oO][dD][eE]([lL])?".
 */
function abbrev(word, min = 4) {
  if (word.length <= min) return ci(word);
  let tail = '';
  for (let i = word.length - 1; i >= min; i--) {
    tail = `(${ci(word[i])}${tail})?`;
  }
  return ci(word.slice(0, min)) + tail;
}

const HS = '[ \\t]*'; // horizontal space
const COLON = `${HS}:`;
const LABEL_WORD = '[ \\t]+[A-Za-z0-9_.\\-]+';

const MODEL = abbrev('MODEL');

// Commands whose body is a list of `OPTION = value(s);` statements.
const OPTION_COMMANDS = [
  `${ci('DATA')}([ \\t]+[A-Za-z]+)?`, // DATA, DATA IMPUTATION, DATA WIDETOLONG, ...
  `${abbrev('VARIABLES')}`,
  `${abbrev('ANALYSIS')}`,
  `${abbrev('MONTECARLO')}`,
  `${abbrev('OUTPUT')}`,
  `${abbrev('SAVEDATA')}`,
  `${ci('PLOT')}`,
];

// MODEL sub-commands that hold expressions rather than model statements.
const EXPRESSION_MODEL_COMMANDS = [
  abbrev('CONSTRAINT'),
  ci('TEST'),
  abbrev('PRIORS'),
];

module.exports = grammar({
  name: 'mplus',

  extras: ($) => [/\s/, $.comment],

  externals: ($) => [$.title_text, $._error_sentinel],

  word: ($) => $.identifier,

  rules: {
    source_file: ($) => repeat($._section),

    _section: ($) =>
      choice($.title_section, $.option_section, $.model_section, $.expression_section),

    // ── Commands ──────────────────────────────────────────────────────────

    title_section: ($) => seq(field('header', $.title_header), repeat($.title_text)),

    option_section: ($) => seq(field('header', $.option_header), repeat($.option_statement)),

    model_section: ($) =>
      seq(field('header', $.model_header), repeat(choice($.statement, $.class_label))),

    // DEFINE, MODEL CONSTRAINT, MODEL TEST, MODEL PRIORS
    expression_section: ($) =>
      seq(field('header', $.expression_header), repeat($.statement)),

    title_header: (_) => token(new RegExp(abbrev('TITLE') + COLON)),

    option_header: (_) => token(new RegExp(`(${OPTION_COMMANDS.join('|')})${COLON}`)),

    // MODEL:, MODEL INDIRECT:, MODEL POPULATION:, MODEL <group label>: ...
    model_header: (_) => token(new RegExp(`${MODEL}(${LABEL_WORD})?${COLON}`)),

    expression_header: (_) =>
      token(
        prec(
          1,
          new RegExp(
            `(${abbrev('DEFINE')}|${MODEL}[ \\t]+(${EXPRESSION_MODEL_COMMANDS.join('|')}))${COLON}`,
          ),
        ),
      ),

    // ── Statements ────────────────────────────────────────────────────────

    // Kept flat on purpose (no LR conflicts); the option name is simply the
    // first identifier of the statement.
    //   NAMES ARE y1-y6;   ESTIMATOR = MLR;   FILE (male) IS male.dat;
    //   OUTPUT: SAMPSTAT STANDARDIZED TECH1;
    option_statement: ($) => seq(repeat1($._option_value), ';'),

    is_are: (_) => token(prec(1, new RegExp(`${ci('IS')}|${ci('ARE')}`))),

    _option_value: ($) =>
      choice(
        $.identifier,
        $.number,
        $.string,
        $.path,
        $.option_group,
        $.operator,
        $.is_are,
        alias($._hyphenated_setting, $.identifier),
        $.percent_setting,
        '=',
        ',',
        '.',
      ),

    // GROUPING = g (1 = male 2 = female);  CLASSES = c(3);  MISSING = ALL(-99);
    option_group: ($) => seq('(', repeat($._option_value), ')'),

    // Option names and settings that contain a hyphen; without this they
    // would lex as `K`, `-`, `1`, `STARTS`.
    //   K-1STARTS = 20 4;   ROTATION = CF-VARIMAX (ORTHOGONAL);
    _hyphenated_setting: (_) =>
      token(
        new RegExp(
          [
            'K-1STARTS',
            'CF-VARIMAX',
            'CF-QUARTIMAX',
            'CF-EQUAMAX',
            'CF-PARSIMAX',
            'CF-FACPARSIM',
            'BI-GEOMIN',
            'BI-CF-QUARTIMAX',
          ]
            .map(ci)
            .join('|'),
        ),
      ),

    // CROSSTABS (%ROW);  CROSSTABS (%COLUMN);  CROSSTABS (%TOTAL);
    percent_setting: (_) => token(/%[A-Za-z]+/),

    // Model and expression statements are flat item sequences ending in `;`.
    // f1 BY y1-y5* (l1-l5);   [y1$1@0];   s | y ON x;   NEW(ab); ab = a*b;
    statement: ($) => seq(repeat1($._item), ';'),

    _item: ($) =>
      choice(
        $.identifier,
        $.number,
        $.string,
        $.group,
        $.bracket_group,
        $.brace_group,
        $.operator,
        '=',
        ',',
      ),

    group: ($) => seq('(', repeat($._item), ')'),
    bracket_group: ($) => seq('[', repeat($._item), ']'),
    brace_group: ($) => seq('{', repeat($._item), '}'),

    // %OVERALL%  %c#1%  %BETWEEN%  %WITHIN c#2%
    class_label: (_) => token(/%[^%\n;]*%/),

    // ── Tokens ────────────────────────────────────────────────────────────

    identifier: (_) => /[A-Za-z_][A-Za-z0-9_]*/,

    number: (_) => /\d+(\.\d*)?([eE][+-]?\d+)?|\.\d+([eE][+-]?\d+)?/,

    string: (_) => token(choice(/"[^"\n]*"/, /'[^'\n]*'/)),

    // Unquoted file names: ex3.1.dat, ../data/x.dat, C:\data\x.dat, imp*.dat
    // Must contain `.`, `/` or `\` so plain names stay identifiers, may not
    // start with a digit (numbers), and only allows `:` after a drive letter
    // so it never swallows a command header such as `VARIABLE:`.
    path: (_) =>
      token(/([A-Za-z]:)?[A-Za-z_.\/\\~][A-Za-z0-9_\-*]*([.\/\\][A-Za-z0-9_.\/\\~\-*]*)+/),

    operator: (_) =>
      token(
        choice(
          '**', '*', '/', '+', '-', '@', '$', '#', '|', '~', '&', '^',
          '==', '/=', '<=', '>=', '<', '>', '<>',
        ),
      ),

    // `! line comment`  and  `!* block comment *!`
    comment: (_) => token(choice(/!\*([^*]|\*+[^*!])*\*+!/, /![^\n]*/)),
  },
});
