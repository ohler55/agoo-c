# [![{}j](misc/agoo_128.svg)](http://www.ohler.com/agoo) Agoo-C

Agoo webserver and GraphQL server in C with the goal of being the best
performing webserver and GraphQL server across all languages. So far
so good according to
[benchmarks](https://github.com/the-benchmarker/web-frameworks) and
[GraphQL benchmarks](https://github.com/ohler55/graphql-benchmarks).

This is a new project. Feedback is required to keep it alive. Feel free to
email me directly or create an issue for additions or changes.

## Installation

```
make
```

The library will be in the `lib` directory and the headers will be in the `include` directory.
A C11 compiler or gcc-7 are needed to build.

## Examples

### example/simple

A simple webserver demonstrating the use of handlers for a few routes.

### example/graphql

A simple GraphQL server with handlers set up for a query and a mutation.

## Releases

See [file:CHANGELOG.md](CHANGELOG.md)

Releases are made from the master branch. The default branch for checkout is
the develop branch. Pull requests should be made against the develop branch.

Also note that at the core of Agoo-C is the Agoo code from
[https://github.com/ohler55/agoo](https://github.com/ohler55/agoo).
