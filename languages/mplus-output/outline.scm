; INPUT INSTRUCTIONS and every upper-case section of the output.
(input_section
  heading: (_) @name) @item

(section
  heading: (_) @name) @item

; Titles inside a section: Chi-Square Test of Model Fit, Latent Class 1, ...
; Left out, so that long outputs stay readable (they are still highlighted):
; everything under TECHNICAL n OUTPUT, and the TECH1 matrix names (THETA, ALPHA(C)).
(section
  heading: (heading) @_heading
  (subheading) @name @item
  (#not-match? @_heading "^TECHNICAL")
  (#not-match? @name "^(NU|LAMBDA|THETA|ALPHA|BETA|PSI|TAU|GAMMA|DELTA|KAPPA)(\\(.*)?$"))

; Messages, so errors and warnings can be jumped to (first line shown).
((error) @name) @item

((warning) @name) @item
