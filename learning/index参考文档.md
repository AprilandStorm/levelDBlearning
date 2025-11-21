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
如果数据库已存在，您希望引发错误，请在调用 leveldb::DB::Open 之前添加以下代码行：
```
options.error_if_exists = true;
```

# Status
```leveldb::Status``` 类型。leveldb 中的大多数函数在遇到错误时都会返回此类型的值。可以检查此结果是否正常，并打印相关的错误消息：
```
leveldb::Status s = ...;
if (!s.ok()) cerr << s.ToString() << endl;
```

# 关闭数据库
当不再使用数据库时，只需删除数据库对象即可。例如：
```
... open the db as described above ...
... do something with db ...
delete db;
```

# 读和写
数据库提供 Put、Delete 和 Get 方法来修改/查询数据库。例如，以下代码将 key1 下存储的值移动到 key2。
```
std::string value;
leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
if (s.ok()) s = db->Put(leveldb::WriteOptions(), key2, value);
if (s.ok()) s = db->Delete(leveldb::WriteOptions(), key1);
```
# 原子更新Atomic Updates
如果进程在执行key2的Put操作之后、删除key1之前终止，则同一个值可能会存储在多个键下。可以使用```WriteBatch```类原子地应用一组更新来避免此类问题：
```
#include "leveldb/write_batch.h"
...
std::string value;
leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
if (s.ok()) {
  leveldb::WriteBatch batch;
  batch.Delete(key1);
  batch.Put(key2, value);
  s = db->Write(leveldb::WriteOptions(), &batch);
}
```
```WriteBatch``` 包含一系列要对数据库进行的编辑操作，并且批次内的这些编辑会按顺序执行。请注意，先调用了 Delete 再调用 Put，这样如果 key1 与 key2 相同，就不会错误地将值完全删除。除了原子性优势外，```WriteBatch ```还可以通过将大量单独的变更操作放入同一个批次中来加速批量更新。

# 同步写入
- 默认情况下，每次写入 leveldb 都是异步的：它会在将写入操作从进程推送到操作系统后返回;从操作系统内存到底层持久存储的数据传输是异步进行的。
- 可以为特定的写入操作启用同步标志，使写入操作在写入的数据完全推送到持久存储之前不会返回。（在 POSIX 系统中，这是通过在写入操作返回之前调用 ```fsync(...)```、```fdatasync(...)``` 或 ```msync(..., MS_SYNC)``` 来实现的）
```
leveldb::WriteOptions write_options;
write_options.sync = true;
db->Put(write_options, ...);
```
- 异步写入通常比同步写入快一千倍以上。异步写入的缺点是，机器崩溃可能会导致最后几次更新丢失。请注意，仅写入进程崩溃（即不是重启）不会造成任何丢失，因为即使 sync 为 false，更新在被视为完成之前也会从进程内存推送到操作系统中
- 异步写入通常可以安全使用。例如，在向数据库加载大量数据时，你可以通过在崩溃后重新启动批量加载来处理丢失的更新。混合方案也是可行的，即每第 N 次写入是同步的，并且在发生崩溃时，批量加载会从上一次运行中最后一次同步写入完成后重新启动。（同步写入可以更新一个标记，该标记描述了崩溃时从何处重新启动。）
- WriteBatch 提供了异步写入的一种替代方案。多个更新操作可以放入同一个 WriteBatch 中，并通过同步写入（即 write_options.sync 被设置为 true）一起执行。同步写入的额外成本将分摊到该批次中的所有写入操作上。

# 并发性Concurrency
一个数据库一次可能只能被一个进程打开。leveldb 实现会从操作系统获取锁以防止误用。在单个进程内，同一个 ```leveldb::DB``` 对象可以被多个并发线程安全地共享。也就是说，不同的线程可以写入数据库、获取迭代器或对同一个数据库调用 Get 方法，而无需任何外部同步（leveldb 实现会自动进行所需的同步）。但是，其他对象（如 Iterator 和 WriteBatch）可能需要外部同步。如果两个线程共享这样的对象，它们必须使用自己的锁定协议来保护对该对象的访问。更多详细信息可在公共头文件中找到。

# 迭代
以下示例演示如何打印数据库中的所有键值对
```
leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
}
assert(it->status().ok());  // Check for any errors found during the scan
delete it;
```
以下变体展示了如何仅处理范围 [start,limit] 内的键：
```
for (it->Seek(start);
   it->Valid() && it->key().ToString() < limit;
   it->Next()) {
  ...
}
```
也可以按相反的顺序处理条目。（注意：反向迭代可能比正向迭代稍慢。）
```
for (it->SeekToLast(); it->Valid(); it->Prev()) {
  ...
}
```

# 快照Snapshots
快照提供了对键值存储整个状态的一致性只读视图。```ReadOptions::snapshot ```可能为非 NULL，这表示读取操作应在数据库状态的特定版本上进行。如果 ```ReadOptions::snapshot``` 为 NULL，读取操作将在当前状态的隐式快照上进行。\
```DB::GetSnapshot()``` 方法用于创建快照：
```
leveldb::ReadOptions options;
options.snapshot = db->GetSnapshot();
... apply some updates to db ...
leveldb::Iterator* iter = db->NewIterator(options);
... read using iter to view the state when the snapshot was created ...
delete iter;
db->ReleaseSnapshot(options.snapshot);
```
请注意，当不再需要某个快照时，应使用 DB::ReleaseSnapshot 接口将其释放。这样，实现就可以清除仅用于支持该快照读取操作而维护的状态。

# Slice
上述 it->key () 和 it->value () 调用的返回值是 leveldb::Slice 类型的实例。Slice 是一种简单的结构体，包含一个长度和一个指向外部字节数组的指针。返回 Slice 是返回 std::string 的一种更经济的替代方式，因为我们无需复制可能很大的键和值。此外，leveldb 方法不会返回以空字符结尾的 C 风格字符串，因为 leveldb 的键和值允许包含 '\0' 字节\
C++ 字符串和以空字符结尾的 C 风格字符串可以轻松转换为Slice：
```
leveldb::Slice s1 = "hello";

std::string str("world");
leveldb::Slice s2 = str;
```
Slice可以很容易地转换回 C++ 字符串
```
std::string str = s1.ToString();
assert(str == std::string("hello"));
```
使用Slice时务必小心，因为调用者需要确保Slice指向的外部字节数组在使用期间保持有效。例如，以下代码存在缺陷：
```
leveldb::Slice slice;
if (...) {
  std::string str = ...;
  slice = str;
}
Use(slice);
```
当 if 语句超出作用域时，str 将被销毁，切片的后备存储也将消失。

# 比较器Comparators
前面的示例使用了键的默认排序函数，该函数按字典顺序对字节进行排序。不过，在打开数据库时，可以提供一个自定义比较器。例如，假设每个数据库键由两个数字组成，应该先按第一个数字排序，若第一个数字相同，则按第二个数字排序。首先，定义一个 leveldb::Comparator 的合适子类来表达这些规则。
```
class TwoPartComparator : public leveldb::Comparator {
 public:
  // Three-way comparison function:
  //   if a < b: negative result
  //   if a > b: positive result
  //   else: zero result
  int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
    int a1, a2, b1, b2;
    ParseKey(a, &a1, &a2);
    ParseKey(b, &b1, &b2);
    if (a1 < b1) return -1;
    if (a1 > b1) return +1;
    if (a2 < b2) return -1;
    if (a2 > b2) return +1;
    return 0;
  }

  // Ignore the following methods for now:
  const char* Name() const { return "TwoPartComparator"; }
  void FindShortestSeparator(std::string*, const leveldb::Slice&) const {}
  void FindShortSuccessor(std::string*) const {}
};
```
现在使用这个自定义比较器创建一个数据库：
```
TwoPartComparator cmp;
leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
options.comparator = &cmp;
leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
...
```
## 向后兼容性
比较器的 Name 方法的结果会在数据库创建时附加到数据库中，并在之后每次打开数据库时进行检查。如果名称发生更改，```leveldb::DB::Open``` 调用将会失败。因此，只有当新的键格式和比较函数与现有数据库不兼容，并且可以丢弃所有现有数据库的内容时，才可以更改名称。\
不过，通过一些预先规划，你仍然可以随着时间的推移逐步改进你的键格式。例如，你可以在每个键的末尾存储一个版本号（对于大多数用途来说，一个字节就足够了）。当你希望切换到新的键格式时（例如，为 TwoPartComparator 处理的键添加一个可选的第三部分），可以（a）保持比较器名称不变；（b）为新键增加版本号；（c）修改比较器函数，使其利用键中包含的版本号来决定如何解析这些键。

# performance
可以通过更改 include/options.h 中定义的类型的默认值来调整性能
## 块大小
leveldb 会将相邻的键组合到同一个块中，这样的块是与持久化存储之间进行数据传输的单位。默认的块大小约为 4096 未压缩字节。主要对数据库内容进行批量扫描的应用程序可能希望增大这个大小。如果性能测试表明有改进，那么进行大量小值点读的应用程序可能希望切换到更小的块大小。使用小于 1 千字节或大于几兆字节的块没有太多好处。另外需要注意的是，块大小越大，压缩效果会越好
## 压缩
每个数据块在写入持久存储之前都会进行单独压缩。默认情况下压缩是开启的，因为默认的压缩方法速度很快，而且对于不可压缩的数据会自动禁用压缩。在极少数情况下，应用程序可能希望完全禁用压缩，但只有在基准测试显示性能有所提升时才应这样做：
```
leveldb::Options options;
options.compression = leveldb::kNoCompression;
... leveldb::DB::Open(options, name, ...) ....
```
## 缓存
数据库内容存储在文件系统中的一组文件中，每个文件存储一系列压缩数据块。如果 options.block_cache 不为 NULL，则用于缓存常用的未压缩数据块内容。
```
#include "leveldb/cache.h"

leveldb::Options options;
options.block_cache = leveldb::NewLRUCache(100 * 1048576);  // 100MB cache
leveldb::DB* db;
leveldb::DB::Open(options, name, &db);
... use the db ...
delete db
delete options.block_cache;
```
请注意，缓存中存储的是未压缩的数据，因此其大小应根据应用程序级别的数据大小来确定，而不应因压缩而有任何缩减。（压缩块的缓存由操作系统的缓冲区缓存或客户端提供的任何自定义 Env 实现来处理。）\
执行批量读取时，应用程序可能希望禁用缓存，以避免批量读取处理的数据覆盖大部分缓存内容。可以使用迭代器选项来实现此目的：
```
leveldb::ReadOptions options;
options.fill_cache = false;
leveldb::Iterator* it = db->NewIterator(options);
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  ...
}
delete it;
```
## key布局
请注意，磁盘传输和缓存的单位是块。相邻的键（根据数据库的排序顺序）通常会被放置在同一个块中。因此，应用程序可以通过将一起访问的键放在彼此附近，并将不常使用的键放在键空间的单独区域来提高其性能。\
例如，假设要在 leveldb 之上实现一个简单的文件系统。我们可能希望存储的条目类型有：
```
filename -> permission-bits, length, list of file_block_ids
file_block_id -> data
```
我们可能希望文件名键前面加上一个字母（例如“/”），文件块 ID 键前面加上另一个字母（例如“0”），这样，仅扫描元数据时就不会强制我们获取和缓存庞大的文件内容
## Filter
由于 LevelDB 数据在磁盘上的组织方式，一次``` Get()``` 调用可能涉及多次磁盘读取。可选的 FilterPolicy 机制可以显著减少磁盘读取次数
```
leveldb::Options options;
options.filter_policy = NewBloomFilterPolicy(10);
leveldb::DB* db;
leveldb::DB::Open(options, "/tmp/testdb", &db);
... use the database ...
delete db;
delete options.filter_policy;
```
前面的代码将基于布隆过滤器的过滤策略与数据库相关联。基于布隆过滤器的过滤依赖于为每个键在内存中保留一定数量的数据位（在这种情况下，每个键 10 位，因为这是传递给 NewBloomFilterPolicy 的参数）。该过滤器会将 Get () 调用所需的不必要磁盘读取次数减少约 100 倍。增加每个键的位数会进一步减少磁盘读取次数，但代价是更多的内存使用。建议那些工作集无法放入内存且进行大量随机读取的应用程序设置过滤策略。\
如果您正在使用自定义比较器，那么应当确保所使用的过滤策略与该比较器兼容。例如，假设有一个比较器在比较键时会忽略尾随空格，那么就不能将 NewBloomFilterPolicy 与这种比较器一起使用。相反，应用程序应该提供一个同样会忽略尾随空格的自定义过滤策略。例如:
```
class CustomFilterPolicy : public leveldb::FilterPolicy {
 private:
  leveldb::FilterPolicy* builtin_policy_;

 public:
  CustomFilterPolicy() : builtin_policy_(leveldb::NewBloomFilterPolicy(10)) {}
  ~CustomFilterPolicy() { delete builtin_policy_; }

  const char* Name() const { return "IgnoreTrailingSpacesFilter"; }

  void CreateFilter(const leveldb::Slice* keys, int n, std::string* dst) const {
    // Use builtin bloom filter code after removing trailing spaces
    std::vector<leveldb::Slice> trimmed(n);
    for (int i = 0; i < n; i++) {
      trimmed[i] = RemoveTrailingSpaces(keys[i]);
    }
    builtin_policy_->CreateFilter(trimmed.data(), n, dst);
  }
};
```
高级应用程序可能会提供一种不使用布隆过滤器，而是使用其他机制来汇总一组键的过滤策略。详情请参阅 ```leveldb/filter_policy.h```

# 校验和
leveldb 会将校验和与其存储在文件系统中的所有数据相关联。对于这些校验和的验证力度，有两个独立的控制项：可以将 ```ReadOptions::verify_checksums``` 设置为 true，强制对代表特定读取操作从文件系统读取的所有数据进行校验和验证。默认情况下，不执行此类验证。在打开数据库之前，可以将 ```Options::paranoid_checks``` 设置为 true，使数据库实现在检测到内部损坏时立即抛出错误。根据数据库中损坏部分的不同，错误可能在打开数据库时抛出，也可能在之后的其他数据库操作中抛出。默认情况下，偏执模式检查是关闭的，因此即使其持久存储的部分内容已损坏，数据库仍可使用。
如果数据库损坏（例如，在开启偏执模式检查时无法打开），可以使用 ```leveldb::RepairDB``` 函数来恢复尽可能多的数据。

# Approximate Sizes
```GetApproximateSizes``` 方法可用于获取一个或多个键范围使用的文件系统空间的近似字节数
```
leveldb::Range ranges[2];
ranges[0] = leveldb::Range("a", "c");
ranges[1] = leveldb::Range("x", "z");
uint64_t sizes[2];
db->GetApproximateSizes(ranges, 2, sizes);
```
前面的调用会将 ```sizes[0] ```设置为键范围 ```[a..c)``` 使用的文件系统空间的近似字节数，并将 ```sizes[1] ```设置为键范围 ```[x..z]``` 使用的近似字节数

# 环境Environment
leveldb 实现所发出的所有文件操作（以及其他操作系统调用）都通过一个 ```leveldb::Env``` 对象进行路由。复杂的客户端可能希望提供自己的 Env 实现，以获得更好的控制权。例如，某个应用程序可能会在文件 IO 路径中引入人为延迟，以限制 leveldb 对系统中其他活动的影响。
```
class SlowEnv : public leveldb::Env {
  ... implementation of the Env interface ...
};

SlowEnv env;
leveldb::Options options;
options.env = &env;
Status s = leveldb::DB::Open(options, ...);
```
# 其他信息
有关 leveldb 实现的详细信息，请参阅以下文档：
- [Implementation notes](https://github.com/google/leveldb/blob/main/doc/impl.md)
- [Format of an immutable Table file](https://github.com/google/leveldb/blob/main/doc/table_format.md)
- [Format of a log file](https://github.com/google/leveldb/blob/main/doc/log_format.md)
