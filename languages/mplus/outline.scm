; Each command (TITLE, DATA, VARIABLE, MODEL, ...) is an outline entry.
(title_section
  header: (_) @name
  .
  (title_text)? @context) @item

(option_section
  header: (_) @name) @item

(model_section
  header: (_) @name) @item

(expression_section
  header: (_) @name) @item

; Mixture / multilevel class labels inside MODEL: %OVERALL%, %c#1%, %BETWEEN%
((class_label) @name) @item
