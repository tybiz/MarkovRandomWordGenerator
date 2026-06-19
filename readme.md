# MarkovRandomWordGenerator 
https://tybiz.github.io/MarkovRandomWordGenerator/

Generates nonsense words using a trigram Markov chain trained on a list of real words fetched by theme. The model is written in C and compiled to WebAssembly — it runs entirely in the browser.

## How it works

1. User enters a theme (e.g. `ocean`)
2. JS fetches up to 200 related words from [Datamuse API](https://www.datamuse.com/api/)
3. The word list is passed into the WASM module, which builds a trigram frequency table
4. `generate_word()` picks a random starting bigram and walks the table to produce a new word

## Build

Requires [Emscripten](https://emscripten.org/) (`brew install emscripten`).

```bash
emcc randomMarkovWord.c -o word_gen.js \
  -s EXPORTED_FUNCTIONS='["_train","_generate_word","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["cwrap","HEAPU8","HEAP32"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="WordGenModule" \
  -s ALLOW_MEMORY_GROWTH=1 \
  -O2
```

This produces `word_gen.js` and `word_gen.wasm`. Serve all three files (`index.html`, `word_gen.js`, `word_gen.wasm`) from the same directory.

## Run locally

```bash
npx serve .
```

Then open `http://localhost:3000`.
