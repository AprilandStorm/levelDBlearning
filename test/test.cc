#include"leveldb/db.h"
#include"leveldb/write_batch.h"
#include"leveldb/comparator.h"
#include<string>
#include<iostream>

//g++ -o test test.cc ../build/libleveldb.a -I ../include -pthread

int main(){
    leveldb::DB* db; 
    
    //open
    leveldb::Options options;
    options.create_if_missing = true;
    std::string name = "testdb"; 
    leveldb::Status status = leveldb::DB::Open(options, name, &db);
    if(status.ok()){
        std::cout << "leveldb open success" << std::endl;
    }
    else{
        std::cout << "leveldb open failed: " << status.ToString() << std::endl;
    }
    std::cout << status.ToString() << std::endl;

    //if(options.comparator == BytewiseComparator()){
    //    cout << "BytewiseComparator" << endl;
    //}

    //put
    leveldb::WriteOptions woptions;
    status = db->Put(woptions, "name", "yujian");

    //get
    leveldb::ReadOptions roptions;
    std::string value;
    status = db->Get(roptions, "name", &value);
    if(status.ok()){
        std::cout << status.ToString() << "," << value << std::endl;
    }
    else if(status.IsNotFound()){
        std::cout << "query success, value not found" << std::endl;
    }
    else{
        std::cout << "query failed, reason: " << status.ToString() << std::endl;
    }

    //writebatch
    leveldb::WriteBatch batch;
    batch.Put("a", "1");
    batch.Put("b", "2");
    status = db->Write(woptions, &batch);

    //delete
    db->Delete(woptions, "name");

    //iterator
    leveldb::Iterator* iter = db->NewIterator(roptions);
    iter->SeekToFirst();
    while(iter->Valid()){
        leveldb::Slice key = iter->key();
        leveldb::Slice value = iter->value();
        std::cout << key.ToString() << "=" << value.ToString() << std::endl;
        iter->Next();
    }

    delete iter;
    delete db;
    return 0;
}