# CHANGELOG

## [Unreleased]

## [0.6.0] - 2019-07-26

Early hints and sub-domains

### Added

- Support for early hints added.

- Sub-domains based on the host header in an HTTP request can now
  be used to set up multiple root directories.

### 0.5.1 - 2019-04-11

Makefile cleanup

- Removed the targets that pulled code from the Agoo repo and put that in a separate script.

### 0.5.0 - 2019-04-06

GraphQL

- Add GraphQL support and example.

### 0.4.0 - 2019-03-05

Merge in core changes.

- Add `agoo_io_loop_ratio` for more control over IO threads.

### 0.3.0 - 2018-11-25

Better thread management and library naming cleanup.

- All Agoo-C types, functions, and variable now include an 'agoo' prefix.

- Read/write thread creation uses a more optimized algorithm.

### 0.1.0 - 2018-11-13

First release.
