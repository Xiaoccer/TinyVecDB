# TinyVecDB

A lightweight vector database based on C++17.

## Build Third-Party

```shell
cd third_party
bash build.sh
```

## Build TinyVecDB

Switch to the project directory:

```shell
cmake . -B build
cd build
make -j
```

## Start Server

```shell
cd bin
./server
```

## Testing

You can send `upsert`/`search`/`query`/`snapshot` commands to the server, following the example commands in `test/test.h`.

## Reference

Book

* 《从零构建向量数据库》

Third-Party Libraries

* faiss
* openblas
* brpc
* gflags
* protobuf
* glog
* crypto
* leveldb
* ssl
* z
* rocksdb
* snappy
* lz4
* bz2
* roaring

## License

TinyVecDB is licensed under the MIT License. For more details, please refer to the [LICENSE](./LICENSE.txt) file.
