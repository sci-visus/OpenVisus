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

## Semi-Automated Documentation

These docs are semi-automated. Python scripts starting with `config_` can be ran to refresh automated documentation. Keep in mind this will override exisiting automated documentation.

## TODO:

- Migrate exisiting pages from wiki.visus.org (maybe even create a potential redirect to this docs website once it is finished?)
    - https://wiki.visus.org/index.php/MIDX_examples
    - https://wiki.visus.org/index.php/Scripting_Node
- Add docs link to main readme for visibility
    - Add list of datasets that can be used for viewer/package testing and examples
