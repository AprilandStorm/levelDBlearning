# 打开数据库
leveldb 数据库的名称对应于文件系统目录。数据库的所有内容都存储在该目录中。以下示例展示了如何打开数据库，并在必要时创建数据库：
```
#include <cassert>
#include "leveldb/db.h"

leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
assert(status.ok());
...
```
