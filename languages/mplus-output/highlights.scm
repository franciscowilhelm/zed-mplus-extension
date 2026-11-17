; Mplus output is plain text; the grammar only marks what helps to read it.
; In Zed, later patterns take precedence over earlier ones.

(number) @number

; SUMMARY OF ANALYSIS, MODEL FIT INFORMATION, MODEL RESULTS, ...
[
  (heading)
  (input_heading)
] @title

; Chi-Square Test of Model Fit, Latent Class 1, PARAMETER SPECIFICATION, ...
(section
  (subheading) @label)

; Rows that open a block of estimates:  F1 BY,  Y1 ON,  Means,  Residual Variances
(parameter_header) @property

; THE MODEL ESTIMATION TERMINATED NORMALLY
(notice) @emphasis.strong

; Themes have no colours for messages, so warnings borrow the function colour
; and errors the keyword colour. The extra `.warning` / `.error` lets a theme
; or `theme_overrides` in the Zed settings style them on their own.

; *** WARNING in MODEL command ...,  NO CONVERGENCE ...,  WARNING:  THE ...
(warning) @function.warning

; *** ERROR in DATA command ...
(error) @keyword.error

; MUTHEN & MUTHEN
(banner) @comment
