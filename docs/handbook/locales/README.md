# Handbook languages

`fun` is the default edition of chapters 00–22, stored in `fun.json`. English
remains selectable and lives directly in the chapter HTML as the fallback when
JavaScript is unavailable or a language cannot load. An explicit URL or remembered
language choice takes precedence over the default.

The sidebar language selector reloads the current chapter with `?lang=en` or
`?lang=fun`, preserves other query parameters and the fragment, and remembers
the selection in `toern-handbook-language` in localStorage. An explicit URL choice
overrides that preference. Chapter links and search submissions carry the choice;
search indexes are cached separately per language. Legacy `?lang=douglas` links
and saved `douglas` preferences migrate to `fun`; the URL is canonicalised without
losing its other query parameters or fragment.

## Adding a language, such as German

1. Add an entry to `LANGUAGES` in `handbook-language.js`, for example:
   `{ id: "de", label: "Deutsch", language: "de", file: "locales/de.json" }`.
2. Create the JSON file with this structure:
   ```json
   {
     "language": "de",
     "pages": {
       "matrix.html": {
         "matrix.001": "Die Matrix …"
       }
     }
   }
   ```
3. Match each dictionary key to the English element's `data-hb-copy` attribute.
   Keep existing keys stable when editing English. Add new, unique keys to other
   blocks as they are translated. Nested copy keys are unnecessary: a translated
   block contains all its inline markup.
4. Preserve links, technical values and dynamic spans such as `data-hb-steps`,
   `data-hb-pages` and `data-hb-size` within translated HTML. The matrix layout is
   reapplied after translation. Leave diagrams, canvases and scripts outside copy
   blocks. JSON is trusted local source content, not user-provided HTML.
5. Missing blocks fall back to English. A chapter missing from `pages` shows a
   short English-fallback note beside the selector. Set `language` to a valid HTML
   language tag (`en` for fun, `de` for German).
6. Bump the locale fetch version in `handbook-language.js` and the search index
   version in `handbook-search.js` when shipping changed translations.

Shared navigation and literal device labels currently remain in English. They
can receive `data-hb-copy` keys and matching per-page entries when translated;
keep firmware labels recognisable as they appear on the device.

## fun editorial direction

Write narrated explanations with room for observation, digression and developed
comic ideas. Change sentence order and passage length when it improves the piece;
a literal paraphrase with a joke attached is not the intended voice. Let an image
or situation develop across a section, and vary the rhythm rather than requiring
a punchline at the end of every block. The narrator may be amused by people and
machines while remaining attentive to the small pleasure of making music.

Keep technical facts, scope, ranges, gestures and consequences clear. Destructive
operations need direct instructions. Parameter lists and quick-reference captions
can stay short; jokes must not make a setting harder to find. English remains an
independent edition, and its wording is not edited to accommodate this edition.
