# CCask

A C KV store inspired by Bitcask.

# Project-wide TODO

*Code improvements*
* Implement more robust error handling (set errno values & read them)
* Add DEBUG flags to have more or less verbose logs as needed
* Remove redundant imports

*Feature improvements*
* Multi-file support (max file size)
* For immutable files, hint file creation / key merging
* Hints file for keydir
* Tombstone values for deletion
* Multi-instance support
