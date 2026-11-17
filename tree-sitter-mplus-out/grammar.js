/**
 * @file Tree-sitter grammar for Mplus output (.out) files
 * @license MIT
 *
 * Mplus output is plain text laid out by line and indentation, so almost all
 * of the structure comes from the external scanner (src/scanner.c), which
 * classifies whole lines:
 *
 *   INPUT INSTRUCTIONS            input_heading, followed by input_text
 *   MODEL FIT INFORMATION         heading   (upper case, column 0)
 *   Chi-Square Test of Model Fit  subheading
 *    F1       BY                  parameter_header (results tables)
 *   *** ERROR in DATA command     error / warning, with their text
 *   THE MODEL ESTIMATION ...      notice
 *
 * Everything else is a flat run of numbers and words. The grammar is lenient
 * on purpose: it exists for highlighting and the outline, not validation.
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: 'mplus_out',

  extras: (_) => [/\s/],

  externals: ($) => [
    $.heading,
    $.input_heading,
    $.banner,
    $.subheading,
    $.parameter_header,
    $.warning,
    $.error,
    $.notice,
    $.input_text,
    $._error_sentinel,
  ],

  rules: {
    source_file: ($) =>
      seq(repeat($._item), repeat(choice($.input_section, $.section, $.banner_section))),

    // The echo of the input file; `input_text` is injected as Mplus.
    input_section: ($) =>
      seq(field('heading', $.input_heading), optional($.input_text), repeat($._item)),

    // SUMMARY OF ANALYSIS, MODEL FIT INFORMATION, MODEL RESULTS, ...
    section: ($) => seq(field('heading', $.heading), repeat($._item)),

    // `MUTHEN & MUTHEN` and the date or address lines after it.
    banner_section: ($) => seq($.banner, repeat($._item)),

    _item: ($) =>
      choice(
        $.subheading,
        $.parameter_header,
        $.warning,
        $.error,
        $.notice,
        $.number,
        $._word,
      ),

    // 0.969   -694.334   0.500D-04   0.483E+00   0.20%   500
    // Listed before `_word` so that it wins when both match the same text;
    // a longer word (a date, a time, `Y1`) still wins by length.
    number: (_) => token(/[-+]?(\d+\.?\d*|\.\d+)([DdEe][-+]?\d+)?%?/),

    _word: (_) => /[^\s]+/,
  },
});
