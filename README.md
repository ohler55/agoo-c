# Agoo-C

Agoo webserver and GraphQL server in C with the goal of being the best
performing webserver and GraphQL server across all languages. So far
so good according to
[benchmarks](http://opo.technology/benchmarks.html#web_benchmarks) and
[GraphQL benchmarks](https://github.com/ohler55/graphql-benchmarks).

This is a new project. Feedback is required to keep it alive. Feel free to
email me directly or create an issue for additions or changes.

## Installation

```
make
```

The library will be in the `lib` directory and the hdeaders will be in the `include` directory.

## Examples

### example/simple

A simple webserver demonstrating the use of handlers for a few routes.

### example/graphql

## Releases

A simple GraphQL server with handlers set up for a query and a mutation.

See [file:CHANGELOG.md](CHANGELOG.md)

Releases are made from the master branch. The default branch for checkout is
the develop branch. Pull requests should be made against the develop branch.

Also note that at the core of Agoo-C is the Agoo code from
[https://github.com/ohler55/agoo](https://github.com/ohler55/agoo).
