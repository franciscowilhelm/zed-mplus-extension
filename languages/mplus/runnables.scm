; Put a ▶ run button in the gutter next to the first command of the file.
; It runs the tasks tagged `mplus-run` (see tasks.json).
(source_file
  .
  [
    (title_section header: (_) @run)
    (option_section header: (_) @run)
    (model_section header: (_) @run)
    (expression_section header: (_) @run)
  ]
  (#set! tag mplus-run))
