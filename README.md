# CCask

A log-structured KV store written in C inspired by Bitcask.

## Build
One command:

`$ make`

## Run

After building,

`$ ./build/ccask`

A URL shortener intended to run with a local CCask as its backend storage can be found at [github.com/tydar/smallurl-ccask](https://github.com/tydar/smallurl-ccask).

## Tests

First, switch the `#define MAX_FILE_BYTES` statements in [ccask_db](https://github.com/tydar/ccask/blob/main/src/ccask_db.h#L14-L16).

`$ make build-test && ./build/ccask_test`

Ideally, these messy tests will be cleaned up. After each run, delete the directory `CCASK_TEST`.
