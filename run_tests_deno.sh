#!/bin/bash -e

COVERAGE_DIR=./tests/coverage
rm -rf $COVERAGE_DIR

./deno.sh test                  \
    --allow-read=.,/tmp         \
    --no-prompt                 \
    --cached-only               \
    --coverage=$COVERAGE_DIR/raw    \
    ${@-tests/}

NO_COLOR=1 ./deno.sh coverage --exclude=./tests $COVERAGE_DIR/raw > $COVERAGE_DIR/coverage.txt

