#pragma once

#include <vector>
#include <optional>
#include <iostream>


namespace ArgumentParser {

template <typename T>
class Storage {
 private:
    T* single_ = nullptr;
    std::vector<T>* multi_ = nullptr;
    bool is_multi_ = false;
    bool owned_ = true;
    bool empty_ = true;

 public:
    T* GetValue() {
        return single_;
    };

    std::vector<T>* GetValues() {
        return multi_;
    };

    bool SetExternal(T& storage) {
        if (single_ || multi_) {
            return false;
        }

        single_ = &storage;
        owned_ = false;

        return true;
    };

    bool SetExternal(std::vector<T>& storage) {
        if (single_ || multi_) {
            return false;
        }

        multi_ = &storage;
        owned_ = false;

        return true;
    };

    bool SetMulti(bool is_multi) {
        if (is_multi && single_) {
            return false;
        }

        if (!is_multi && multi_) {
            return false;
        }

        is_multi_ = is_multi;

        return true;
    }

    bool AddValue(T value) {
        if (!is_multi_ && !empty_) {
            return false;
        }

        if (!is_multi_) {
            if (!single_) {
                single_ = new T;
            }
            *single_ = value;
            empty_ = false;
            return true;
        }

        if (!multi_) {
            multi_ = new std::vector<T>;
        }
        multi_->push_back(value);
        empty_ = false;
        return true;
    };

    bool IsEmpty() {
        return empty_;
    }

    ~Storage() {
        if (owned_) {
            delete single_;
            delete multi_;
        }
    };
};

} // namespace ArgumentParser
