# dom.js explained

dom.js is a prepacked `cheerio` (from npm) created with browserify,
because I was too lazy to implement proper modules for the embedded
V8 engine.

You can recreate this library with:

input.js:
```
YMD.cheerio = require('cheerio');
```

and running `browserify input.js -o dom.js`.