; INPUT INSTRUCTIONS and every upper-case section of the output.
(input_section
  heading: (_) @name) @item

(section
  heading: (_) @name) @item

; Titles inside a section: Chi-Square Test of Model Fit, Latent Class 1, ...
(section
  (subheading) @name @item)

; Messages, so errors and warnings can be jumped to (first line shown).
((error) @name) @item

((warning) @name) @item
