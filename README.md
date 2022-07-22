# OpenViSUS Documentation

The following branch in the OpenViSUS repository serves as documentation for the OpenViSUS package.

Access the documentation site [here](https://sci-visus.github.io/OpenVisus/).

## Running the Jekyll Server for Local Testing and Troubleshooting

These docs can be ran locally for faster development and testing so you don't have to make unecessary commits just to see the build fail without verbose output on GitHub pages.

Make sure you have `ruby` and `bundle` installed. There is already a [Gemfile](Gemfile) for this project that contains package dependencies.

Install dependencies through `bundle`:
```bash
bundle install
```

Run the server after modifying docs:
```bash
bundle exec jekyll serve
```
