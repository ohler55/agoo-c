#!/bin/sh

# Run from the repo root to pull in changes from the agoo repo which forms the
# core of this implemenation.

mkdir -p src/agoo

cp ../agoo/ext/agoo/atomic.h src/agoo
cp ../agoo/ext/agoo/base64.[ch] src/agoo
cp ../agoo/ext/agoo/bind.[ch] src/agoo
cp ../agoo/ext/agoo/con.[ch] src/agoo
cp ../agoo/ext/agoo/debug.[ch] src/agoo
cp ../agoo/ext/agoo/doc.[ch] src/agoo
cp ../agoo/ext/agoo/domain.[ch] src/agoo
cp ../agoo/ext/agoo/dtime.[ch] src/agoo
cp ../agoo/ext/agoo/early.[ch] src/agoo
cp ../agoo/ext/agoo/err.[ch] src/agoo
cp ../agoo/ext/agoo/graphql.[ch] src/agoo
cp ../agoo/ext/agoo/gqlcobj.[ch] src/agoo
cp ../agoo/ext/agoo/gqleval.[ch] src/agoo
cp ../agoo/ext/agoo/gqlintro.[ch] src/agoo
cp ../agoo/ext/agoo/gqljson.[ch] src/agoo
cp ../agoo/ext/agoo/gqlsub.[ch] src/agoo
cp ../agoo/ext/agoo/gqlvalue.[ch] src/agoo
cp ../agoo/ext/agoo/hook.[ch] src/agoo
cp ../agoo/ext/agoo/http.[ch] src/agoo
cp ../agoo/ext/agoo/kinds.h src/agoo
cp ../agoo/ext/agoo/log.[ch] src/agoo
cp ../agoo/ext/agoo/method.h src/agoo
cp ../agoo/ext/agoo/page.[ch] src/agoo
cp ../agoo/ext/agoo/pub.[ch] src/agoo
cp ../agoo/ext/agoo/queue.[ch] src/agoo
cp ../agoo/ext/agoo/ready.[ch] src/agoo
cp ../agoo/ext/agoo/req.[ch] src/agoo
cp ../agoo/ext/agoo/res.[ch] src/agoo
cp ../agoo/ext/agoo/response.[ch] src/agoo
cp ../agoo/ext/agoo/sha1.[ch] src/agoo
cp ../agoo/ext/agoo/sectime.[ch] src/agoo
cp ../agoo/ext/agoo/seg.h src/agoo
cp ../agoo/ext/agoo/server.[ch] src/agoo
cp ../agoo/ext/agoo/sdl.[ch] src/agoo
cp ../agoo/ext/agoo/sse.[ch] src/agoo
cp ../agoo/ext/agoo/subject.[ch] src/agoo
cp ../agoo/ext/agoo/text.[ch] src/agoo
cp ../agoo/ext/agoo/upgraded.[ch] src/agoo
cp ../agoo/ext/agoo/websocket.[ch] src/agoo
