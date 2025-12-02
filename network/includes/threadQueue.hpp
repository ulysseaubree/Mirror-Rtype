#pragma once
#include <queue>
#include <mutex>
#include <string>
#include <iostream>



class ThQueue {
public:
    ThQueue() = default;
    ~ThQueue() = default;

    void push(const std::string value);
    bool try_pop(std::string &result);
    bool empty();

private :
    std::queue<std::string> _queue;
    std::mutex  _queueMutex; 

};