/*
** EPITECH PROJECT, 2025
** jetpack
** File description:
** threadQueue
*/

#include "../includes/threadQueue.hpp"
#include <mutex>
#include <string>
#include <queue>

void ThQueue::push(const std::string value)
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    _queue.push(value);
}

bool ThQueue::try_pop(std::string &result)
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    if (!_queue.empty()) {
        result = _queue.front();
        _queue.pop();
        return true;
    } else {
        return false;
    }
}

bool ThQueue::empty()
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    return _queue.empty();
}