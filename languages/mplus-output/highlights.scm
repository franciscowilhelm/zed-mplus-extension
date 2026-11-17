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

; *** WARNING in MODEL command ...,  NO CONVERGENCE ...,  WARNING:  THE ...
(warning) @constant

; *** ERROR in DATA command ...
(error) @keyword

; MUTHEN & MUTHEN
(banner) @comment
