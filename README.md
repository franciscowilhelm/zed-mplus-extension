# Mplus for Zed

[Mplus](https://www.statmodel.com/) support for the [Zed](https://zed.dev) editor:

- **Syntax highlighting** for input (`.inp`) and output (`.out`) files.
- **Tasks** to run Mplus on the current file, with a ▶ button for easy access.
- **Outline panel / breadcrumbs**: one entry per command plus mixture class labels in
  input files; the sections of the output (`MODEL FIT INFORMATION`, `MODEL RESULTS`, …),
  their sub-headings and any errors or warnings in output files.
- **Bracket matching**, `!` comment toggling (`cmd-/`)

No language server is used, only tree-sitter.

Input files (`tree-sitter-mplus/`):
  - command headers (`TITLE:`, `DATA:`, `VARIABLE:`, `MODEL:`, `MODEL CONSTRAINT:`, …, incl. 4-letter abbreviations)
  - free-text titles, `!` line comments and `!* … *!` block comments
  - option names (`NAMES ARE`, `ESTIMATOR =`) and built-in settings (`MLR`, `BAYES`, `MIXTURE`, …)
  - model keywords (`BY`, `ON`, `WITH`, `PON`, `PWITH`, `XWITH`, `IND`, `VIA`, `|`), `@` / `*` / `$` / `#`
  - class labels (`%OVERALL%`, `%c#1%`), parameter labels (`(b1)`), DEFINE/CONSTRAINT functions

Commands, options and settings follow the language summary in chapter 20 of the
Mplus User's Guide.

Output files (`tree-sitter-mplus-out/`):
  - section headings and the titles inside them (`Chi-Square Test of Model Fit`, `Latent Class 1`)
  - the echoed input under `INPUT INSTRUCTIONS`, highlighted like an `.inp` file
  - `*** ERROR` / `*** WARNING` messages with their text, estimation problems
    (`NO CONVERGENCE`, `WARNING: …`) and status lines (`THE MODEL ESTIMATION TERMINATED NORMALLY`)
  - numbers, and the rows that open a block of estimates (`F1 BY`, `Means`, `Residual Variances`)

Keyword lists are adapted from the VS Code
[language-mplus](https://marketplace.visualstudio.com/items?itemName=pacbard.language-mplus) extension;
the run task follows the idea of the SSCC
[Mplus and VS Code](https://sscc.wisc.edu/sscc/pubs/MPlus/Slurm/MPlus-and-VSCode.html) guide.

## Install

Until the extension is in Zed's extension registry, install it as a dev extension:

1. Clone this repository:

   ```bash
   git clone https://github.com/franciscowilhelm/zed-mplus-extension.git
   ```

2. In Zed: command palette → **zed: install dev extension** → pick the cloned folder.
   Zed downloads the grammar from this repository and compiles it.

3. Open any `.inp` file; the language selector in the status bar should say **Mplus**.
   For an `.out` file it says **Mplus Output**.

`.out` is not only used by Mplus. To keep other `.out` files as plain text, add this to
your Zed settings; Mplus output is then still recognised by its first line
(`Mplus VERSION …`):

```json
"file_types": { "Plain Text": ["out"] }
```

## Developing the grammar

There are two grammars: `tree-sitter-mplus/` for input files (`grammar.js`, plus
`src/scanner.c` for `TITLE:` text) and `tree-sitter-mplus-out/` for output files, where
`src/scanner.c` does most of the work by classifying whole lines.
Zed compiles the committed `src/parser.c`, so after changing a grammar:

1. Regenerate and check the examples parse without errors:

   ```bash
   cd tree-sitter-mplus && npm install && npx tree-sitter generate && npx tree-sitter parse examples/*.inp --quiet --stat
   ```

   ```bash
   cd tree-sitter-mplus-out && npm install && npx tree-sitter generate && npx tree-sitter parse examples/*.out --quiet --stat
   ```

2. Commit (including the grammar's `src/`) and push.

3. Set `rev` of that grammar in `extension.toml` to the full SHA of that commit, then run
   **zed: rebuild dev extension**. To test before pushing, temporarily set
   `repository = "file:///absolute/path/to/zed-mplus-extension"`.

Changes to `languages/*` (queries, tasks, config) only need
**zed: rebuild dev extension**, no commit.


## Running Mplus

The extension ships these tasks (command palette → **task: spawn**, or the ▶ in the gutter
next to the first command of the file):

| Task | What it does |
| --- | --- |
| `Mplus: run current file` | saves the file, runs `mplus <file>.inp` in the file's folder |
| `Mplus: run current file and open output` | same, then opens `<file>.out` in Zed |
| `Mplus: open output of current file` | opens the `.out` file next to the input |
| `Mplus: open input of current output` | from an `.out` file: opens the `.inp` file next to it |
| `Mplus: re-run input of current output` | from an `.out` file: runs `mplus <file>.inp` again |

Mplus writes `<name>.out` next to the input. Opening the output uses the `zed` CLI
(Zed menu → **Install CLI**).

### Keybinding

Add to `~/.config/zed/keymap.json` to run the current file with `cmd-shift-r`:

```json
[
  {
    "context": "Editor && extension == inp",
    "bindings": {
      "cmd-shift-r": ["task::Spawn", { "task_name": "Mplus: run current file" }]
    }
  }
]
```

### Mplus not on PATH / other setups

The tasks call `mplus`. If it isn't on your `PATH`, copy a task into
`~/.config/zed/tasks.json` (global) or `.zed/tasks.json` (project) and change `command`,
e.g. `/Applications/Mplus/mplus` on macOS or `C:\\Program Files\\Mplus\\Mplus.exe` on
Windows. User tasks with the tag `mplus-run` also show up on the ▶ button.

Example for a Slurm cluster (as in the SSCC guide), in `.zed/tasks.json`:

```json
[
  {
    "label": "Mplus: submit current file via Slurm",
    "command": "ssubmit",
    "args": ["-L", "mplus", "--cores=1", "--mem=2G", "--partition=short", "mplus $ZED_FILENAME"],
    "cwd": "$ZED_DIRNAME",
    "save": "current",
    "tags": ["mplus-run"]
  }
]
```

## Notes and limitations

- The grammar is lenient on purpose: it gives structure for highlighting and the outline,
  not validation. Mplus itself is the syntax checker.
- A line *starting* with a command and a colon (e.g. `Model 2: …` on its own line) ends
  the `TITLE:` text, the same way Mplus reads it. Text on the `TITLE:` line itself is
  always title.
- Output has no real syntax, so `.out` files are read line by line: upper-case lines at
  the left margin are section headings, upper-case sentences are messages, and so on.
  A title written in capitals is echoed by Mplus at the left margin and therefore shows
  up as a heading.
- Every `.out` file opens as Mplus Output; see [Install](#install) to change that.


## Layout

```
extension.toml            Zed extension manifest (points at the grammar)
languages/mplus/          input files (.inp)
  config.toml             file suffixes, comments, brackets
  highlights.scm          syntax highlighting
  outline.scm             outline panel
  brackets.scm            bracket matching
  runnables.scm           ▶ gutter button on the first command
  tasks.json              "Mplus: run …" tasks
languages/mplus-output/   output files (.out)
  config.toml             file suffix and first-line pattern
  highlights.scm          syntax highlighting
  injections.scm          echoed input is highlighted as Mplus
  outline.scm             outline panel
  tasks.json              open / re-run the input of an output
tree-sitter-mplus/
  grammar.js              the input grammar
  src/scanner.c           external scanner for TITLE text
  src/parser.c            generated, must be committed (Zed compiles it)
  examples/*.inp          sample inputs used to check the parser
tree-sitter-mplus-out/
  grammar.js              the output grammar
  src/scanner.c           external scanner that classifies output lines
  src/parser.c            generated, must be committed (Zed compiles it)
  examples/*.out          sample outputs used to check the parser
```

## Publishing (internal note)

Push to GitHub, then in `extension.toml` set `repository` to the GitHub URL and `rev`
to the full commit SHA, and follow Zed's
[publishing guide](https://zed.dev/docs/extensions/developing-extensions#publishing-your-extension).
