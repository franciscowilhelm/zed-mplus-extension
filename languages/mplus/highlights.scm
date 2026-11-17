; Mplus is case-insensitive, so every word list below uses (?i).
; In Zed, later patterns take precedence over earlier ones: generic first.

; ── Basics ───────────────────────────────────────────────────────────────

(identifier) @variable

(comment) @comment

(number) @number

(string) @string

(path) @string

(title_text) @string.special

[
  (title_header)
  (option_header)
  (model_header)
  (expression_header)
] @title

; ── Operators and punctuation ────────────────────────────────────────────

(operator) @operator

(is_are) @keyword.operator

"=" @operator

";" @punctuation.delimiter

"," @punctuation.delimiter

[
  "("
  ")"
  "["
  "]"
  "{"
  "}"
] @punctuation.bracket

; ── Option sections (DATA, VARIABLE, ANALYSIS, OUTPUT, SAVEDATA, PLOT ...) ─

; Built-in settings: ESTIMATOR = MLR;  TYPE = MIXTURE COMPLEX;  ...
((option_section
  (option_statement
    (identifier) @constant.builtin))
  (#match? @constant.builtin "(?i)^(FREE|INDIVIDUAL|FULLCOV|FULLCORR|MEANS|STDEVIATIONS|MONTECARLO|IMPUTATION|OFF|ON|CHECK|NOCHECK|SEQUENTIAL|REGRESSION|SDROPOUT|DDROPOUT|BLANK|UNSCALED|ECLUSTER|GENERAL|BASIC|RANDOM|COMPLEX|MIXTURE|TWOLEVEL|THREELEVEL|CROSSCLASSIFIED|EFA|ESEM|ML|MLM|MLMV|MLR|MLF|MUML|WLS|WLSM|WLSMV|ULS|ULSMV|GLS|BAYES|DELTA|THETA|LOGIT|LOGLINEAR|PROBABILITY|PROBIT|GEOMIN|QUARTIMIN|OBLIMIN|VARIMAX|PROMAX|TARGET|CRAWFER|KAISER|NOMEANSTRUCTURE|NOCOVARIANCES|ALLFREE|JACKKNIFE|JACKKNIFE1|JACKKNIFE2|BRR|FAY|EM|EMA|FS|ODLL|INTEGRATION|STANDARD|GAUSSHERMITE|OBSERVED|EXPECTED|COMBINATION|MEDIAN|MEAN|MODE|UNPERTURBED|PERTURBED|LATENT|GIBBS|MH|PX1|PX2|PX3|RW|ALL|COUNT|STDYX|STDY|STD|FSCORES|CPROBABILITIES|LOGLIKELIHOOD|INFLUENCE|COOKS|MAHALANOBIS|COVARIANCE|CORRELATION|PLOT1|PLOT2|PLOT3|CONFIGURAL|METRIC|SCALAR|BCBOOTSTRAP|SYMMETRIC|BOOTSTRAP|R3STEP|DU3STEP|DE3STEP|BCH|DCATEGORICAL|DCONTINUOUS|FPC|SFRACTION|POPULATION|MEMBERSHIP|PROBABILITIES|PRIORS|ORTHOGONAL|OBLIQUE|EQUAL|UNEQUAL|FIXED|FREE|ALIGNMENT|ENTROPY|WIDE|LONG|BINARY|CONTINUOUS|NOMINAL|INVARIANCE|NONINVARIANCE|PARTIAL|SVALUES|BMIX|BDIFF|NORMAL|SKEWNORMAL|TDISTRIBUTION|SKEWT|RESCOVARIANCES|RESCOV|CF-VARIMAX|CF-QUARTIMAX|CF-EQUAMAX|CF-PARSIMAX|CF-FACPARSIM|BI-GEOMIN|BI-CF-QUARTIMAX|SQRT|FOURTHRT|REFGROUP|PRODUCT|ITERATIONS|CONVERGENCE|RESIDUAL|EQTAIL|HPD|SENSITIVITY|PROPENSITY|REPWEIGHTS|BCHWEIGHTS|LRESPONSES|BSEM|UW|UB|_RECNUM)$"))

; Settings that are also common variable names, so only after their option:
; WTSCALE = CLUSTER;  BWTSCALE = SAMPLE;  TRANSFORM = LOG;  TYPE = MISSING;
((option_statement
  .
  (identifier) @_option
  (identifier) @constant.builtin)
  (#match? @_option "(?i)^(WTSC|BWTS|TRAN|TYPE)")
  (#match? @constant.builtin "(?i)^(CLUSTER|SAMPLE|LOG|MISSING)$"))

; Short settings that only appear in parentheses: AUXILIARY = (R) x;  IMPUTE = y (c);
((option_group
  (identifier) @constant.builtin)
  (#match? @constant.builtin "(?i)^(M|R|E|B|C|BCH|R3STEP|DU3STEP|DE3STEP|DCATEGORICAL|DCONTINUOUS|STDYX|STDY|STD|ALL|COUNT|STARTS|CLUSTER|SYMMETRIC|BOOTSTRAP|BCBOOTSTRAP|EQTAIL|HPD|COVARIANCE|SEQUENTIAL|STANDARD|RESIDUAL|OBLIQUE|ORTHOGONAL|EQUAL|UNEQUAL|FPC|SFRACTION|POPULATION|MEMBERSHIP|PROBABILITIES|PRIORS|CONFIGURAL|BSEM|PX1|PX2|PX3|RW|NOT|RIGHT)$"))

; CROSSTABS (%ROW);  CROSSTABS (%COLUMN);  CROSSTABS (%TOTAL);
(percent_setting) @constant.builtin

; Option names: the first word of a statement (NAMES = ..., ESTIMATOR IS ...)
(option_statement
  .
  (identifier) @property)

; OUTPUT / PLOT / SAVEDATA flags that stand on their own (SAMPSTAT TECH1 ...)
((option_statement
  (identifier) @property)
  (#match? @property "(?i)^(SAMPSTAT|CROSSTABS|STANDARDIZED|STDYX|STDY|STD|RESIDUAL|MODINDICES|CINTERVAL|SVALUES|NOCHISQUARE|NOSERROR|H1SE|H1TECH3|PATTERNS|FSCOEFFICIENT|FSDETERMINACY|BASEHAZARD|LOGRANK|ENTROPY|ALIGNMENT|H1MODEL|FSCOMPARISON|TECH[0-9]+|SAMP|CROSS|STAND|RES|MOD|CINT|SVAL|NOCHI|NOSERR|PAT|FSCOEF|FSDET|BASE)$"))

; Missing value flag in VARIABLE / DEFINE
((identifier) @constant.builtin
  (#match? @constant.builtin "(?i)^_MISSING$"))

; ── MODEL sections ───────────────────────────────────────────────────────

; f1 BY y1-y5;  y ON x;  y1 WITH y2;  i s | y1@0 y2@1;  y IND x;
((model_section
  (statement
    (identifier) @keyword))
  (#match? @keyword "(?i)^(BY|ON|WITH|PON|PWITH|XWITH|IND|VIA|MOD|AT)$"))

; %OVERALL%  %c#1%  %BETWEEN%  %WITHIN%
(class_label) @type

; Parameter labels: y ON x (b1);  f BY y1-y4 (l1-l4);
(model_section
  (statement
    (group
      (identifier) @label)))

; ── DEFINE / MODEL CONSTRAINT / MODEL TEST / MODEL PRIORS ────────────────

; Logical operators and control words (also used in USEOBSERVATIONS)
((identifier) @keyword.operator
  (#match? @keyword.operator "(?i)^(AND|OR|NOT|EQ|NE|GE|LE|GT|LT)$"))

((expression_section
  (statement
    (identifier) @keyword))
  (#match? @keyword "(?i)^(IF|THEN|DO|NEW|CENTER|STANDARDIZE|CUT|GRANDMEAN|GROUPMEAN|CLUSTER_MEAN)$"))

; CENTER x (GRANDMEAN);  CENTER x (GROUPMEAN clus);
((expression_section
  (statement
    (group
      (identifier) @constant.builtin)))
  (#match? @constant.builtin "(?i)^(GRANDMEAN|GROUPMEAN)$"))

; Functions: LOG(x)  EXP(x)  MEAN(y1-y5)  NEW(a b)  N(0, 1)  LOOP(x, -2, 2, .1)
((expression_section
  (statement
    (identifier) @function
    .
    (group)))
  (#match? @function "(?i)^(LOG|LOG10|EXP|SQRT|ABS|SIN|COS|TAN|ASIN|ACOS|ATAN|MEAN|SUM|CLUSTER_MEAN|NEW|DO|DIFF|DIFFERENCE|COV|COVARIANCE|PLOT|LOOP|N|LN|IW|IG|G|U|D|DIRICHLET|CN)$"))
