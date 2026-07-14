# Migrating From The Java Version

The legacy Java application is no longer shipped in the current working tree;
its history remains available as a behavior reference. The Qt application
stores its own settings through `QSettings` and does not read or update Java
`config.txt` on every launch.

## Java Config Import

On first launch, the Qt app offers three startup choices. `Configure Engine`
requires a complete local KataGo setup: executable, model, GTP config, and
analysis config. `Import Java Config` can import both GTP and analysis-engine
commands from the Java version's `config.txt`. `No Engine Mode` enters the UI
without KataGo paths so you can inspect and edit SGF files first.

If you skipped Java import during first launch, use `File > Import Java Config`
later to run the same one-time import. Batch analysis and estimate mode use the
same analysis config gathered during first launch.

The import replaces the Qt engine configuration with the usable KataGo entries
from the selected Java config, while preserving unrelated Qt preferences that do
not exist in the Java file. The first imported local engine becomes the main
engine; the second becomes the compare engine. If the compare engine has no
separate extra args, it inherits the main engine's extra args at runtime.

The importer copies a conservative subset:

- local KataGo `gtp` command fields: executable, model, GTP config, and extra args
- local KataGo `analysis` command fields: executable, model, analysis config, and extra args
- first two local engines from `engine-settings-list` or the older `engine-command` fields
- legacy `engine-start-location` as the Qt engine working directory
- analysis interval, visit/playout/time limits, PDA, WRN, language, theme hint, and ownership display hint

Command extra arguments are parsed as an argument vector, not by invoking a
shell. Quoted arguments preserve whitespace, empty quoted arguments are kept as
empty values, and Windows-style paths keep their backslashes. Literal double
quotes inside one argument use Qt's saved form of three consecutive quote
characters.

Unsupported Java-era features are intentionally skipped in the first Qt release:
SSH engines, Fox/online board sync, screen board sync, and non-KataGo engines.
Skipped SSH engine entries are reported in the GTP console.

Relative model/config/working-directory paths are resolved relative to the
imported `config.txt`.
Bare executable names such as `katago` are left as command names so the system
`PATH` can resolve them.

The imported values are saved to Qt's `QSettings` immediately. If realtime or
compare analysis is currently requested, the affected engine process is
restarted or resynchronized so analysis continues with the imported executable,
model, config, and extra args.

The imported ownership display hint controls whether ownership is shown. The Qt
version also has its own Board Display setting for choosing main-board,
mini-board, or both-board ownership overlays; this display location is not read
from the Java config.

## SGF Analysis Data

The Qt version reads standard SGF properties and preserves unknown properties on
save. It also imports the Java version's private analysis cache properties:

- `LZOP`: root-position Lizzie/KataGo analysis
- `LZ`: node analysis for the primary engine

Imported data includes root visits, root winrate, score mean, ownership, move
candidates, policy/prior, PV, and PV visit counts when present. `LZ2` dual-engine
analysis is preserved as an SGF property, but only the primary analysis snapshot
is imported into the Qt analysis model.
If the private `LZOP`/`LZ` properties are missing but a node still contains a
generated `LizzieYzy analysis` comment section, Qt attempts a best-effort import
of the root winrate, score mean, visits, perspective, candidates, and PV from
that comment. For both private properties and generated comments, known KataGo
extension fields such as edge visits, utility, no-result values, and raw
statistical fields are treated as PV boundaries rather than PV moves.

If an SGF file contains a collection with multiple games, interactive Open asks
which game to load. Session restore saves the selected collection game index and
reopens that game on the next launch; other non-interactive loads still use the
first game and write a diagnostic to the GTP console. Saving after selecting a
game preserves the full collection and replaces only the edited game.

By default, batch analysis results are saved to sidecar files named:

```text
game.sgf.analysis.json
```

Sidecar files store successful analysis snapshots and failed-node diagnostics.
Sidecars written from a multi-game SGF collection also store
`collectionGameIndex`; when reopening a collection, sidecars without a matching
index are skipped with a diagnostic to avoid applying one game's analysis to
another game in the same SGF file.
When sidecar writing is disabled, successful batch analysis is written into
legacy-compatible `LZOP`/`LZ` SGF properties plus a generated comment section.
Qt also writes `LZYPERSP` to record whether the stored winrate and score values
are from Black's or White's perspective, so realtime GTP analysis keeps the same
graph semantics after saving and reopening.
Failed-node diagnostics are written into the Qt-specific `LZYERR` SGF property
and mirrored into a generated `LizzieYzy analysis error` comment section so the
SGF remains readable outside the Qt application. When a node is written as a
failed analysis node, stale `LZOP`/`LZ`/`LZYPERSP` success data on that node is
removed to avoid showing contradictory analysis. If both a success snapshot and
a failure diagnostic are present in memory, the failure diagnostic takes
precedence when writing SGF. User comments are kept before the generated
analysis or error section, and repeated saves replace the generated section
instead of duplicating it.
For multi-game collections, SGF write-back preserves the unselected games while
writing analysis into the selected game.

When opening an SGF, Qt imports legacy SGF analysis first and then applies the
sidecar if it exists, so sidecar data takes precedence.

Sidecar files with an unknown non-empty `format` are rejected with a diagnostic
instead of being partially imported. Older sidecar files without a `format` field
remain accepted for compatibility.

SGF files containing `LZYERR` restore the node diagnostic into the Qt analysis
model. Clearing analysis from a node or subtree removes stale `LZOP`, `LZ`,
`LZYPERSP`, and `LZYERR` properties while preserving unrelated SGF properties
and user comments.

These file behaviors are configurable in `Settings`: legacy SGF analysis import,
sidecar loading on open, and sidecar writing after batch analysis can each be
disabled. When sidecar writing after batch analysis is disabled, batch results
are written back into the current SGF instead. Defaults keep the compatibility
behavior enabled.

## Game Info Fields

The New Game dialog can create square or rectangular SGF boards up to 52 columns
and rows, including boards that are wider than standard GTP coordinates can
represent. When the selected square board size and handicap count have standard
points, Qt writes `HA` and places the matching root `AB` stones so White is next
to move. The dialog offers `0` or `2` through `9` handicap choices and disables
handicap on rectangular or otherwise unsupported board sizes so invalid `HA[1]`
or empty unsupported handicap metadata is not created; use setup-stone editing
plus Game Info if you need a custom SGF position.

The Qt Game Info dialog edits common root properties such as players, ranks,
result, date, source, rules, komi, and handicap. Empty text fields are saved as
absent SGF properties rather than as empty values.

Rules and komi are optional in SGF. If a file did not contain `RU` or `KM`,
opening Game Info and changing unrelated fields does not inject default rules or
komi into the saved file. The dialog still shows the usual Chinese rules and
7.5 komi defaults as disabled placeholders so a user can enable and save them
explicitly.

To clear an existing `RU` or `KM`, uncheck the checkbox next to Rules or Komi in
Game Info before saving. Changing rules, komi, or handicap while realtime or
compare analysis is running clears stale analysis, resynchronizes the active
KataGo position, and restarts the requested analysis mode with the updated
settings. Editing descriptive fields such as player names, ranks, result, date,
or source does not restart active analysis.
Starting realtime analysis on a node also clears that node's stale SGF analysis
properties and generated analysis comment before the first new KataGo update, so
saving during a restart does not write old analysis as if it were current.
Known SGF rule aliases such as `jp`, `tt`, `new_zealand`, surrounding
whitespace, and case variations are mapped to the built-in
Chinese/Japanese/Tromp-Taylor/AGA/New Zealand/Korean options in Game Info.
Saving after opening Game Info writes the canonical SGF value, while unknown
custom rules remain editable text.
