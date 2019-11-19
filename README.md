# Template repository for new Pandoc reports

This is meant to make it easy to add a new report to pandoc.

## Getting started

1. Create a copy of this repo in your name on github by clicking the "Use this template button" 
1. Add docs to the docs folder in pandoc flavored markdown (they will be built in lexicographical order)
1. Run `make` to get your output

# Features

## Output formats

By default, this can output pdf (via pdflatex), epub (via pandoc) and html (via pandoc).

For html, you can apply a style. We use github by default. You can specify
your own pandoc template if you like, but by default the github template is used.

## Code annotation

You can easily annotate code thanks to the inclusion of https://github.com/owickstrom/pandoc-include-code

For example, add a repository to the src directory (see [the readme for that](src/README.md)), and then you can reference it like:

````
```{.cpp include=src/path-to-src startLine=SOME_LINE endLine=END_LINE}
```
````

Or include a whole file (for small files):

````
```{.cpp include=src/path-to-src }
```
````

If you want to comment on different versions of the same repository, add each
version as a separate, shallow submodule.

You can also set the highlight style for your code.

You can see the available languages for highlight (above cpp was used) with:

```bash
docker run --user `id -u`:`id -g`  "quay.io/dalehamel/pandoc-report-builder"  pandoc --list-highlight-languages
```

And the available code styles with:

```bash
docker run --user `id -u`:`id -g`  "quay.io/dalehamel/pandoc-report-builder"  pandoc --list-highlight-styles
```

## References

Citing work and giving people credit where it is due is very important! It also helps to build a community, help search-engines raise the profile of work that is linked to, and make it easy for readers to follow up on the sources for their own discovery.

We use pandoc-citeproc to parse bibliography.yml, and use the ieee reference format, though you cat substitute in anything that works with a sort of bibtex csl style.

You can easily reference other people's hard work within the markdone using the 
syntax:

```
[@some-bibliography-id]
```

Where `some-bibliography-id`, is the `id` field of some bibliography entry. The
format of this bibliography.yml file can be found in the pandoc documentation.

By default, the ieee bibliography format is used an included, but you can easily
specify your own.

# Dependencies

A \*nix system, with a modern gnu `make` and a `docker` build environment

# Publishing to github pages

Check out the sample travis file. All you need to do is add your repo to travis,
add a developer token to allow pushing to public repos so that travis can
push to a gh-pages branch, and you can automatically publish the report to
github pages.
