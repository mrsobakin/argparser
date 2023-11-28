#pragma once

#include <any>
#include <charconv>
#include <optional>
#include <string>
#include <vector>

#include "Storage.h"


namespace ArgumentParser {

class Argument {
 protected:
    bool is_flag_;
    bool is_positional_;
    bool is_multivalue_;
    size_t min_multivalue_;

 public:
    std::string description;

 public:
    virtual bool TryParse(std::string_view string) = 0;
    virtual std::any GetValue() = 0;
    virtual std::any GetValues() = 0;
    virtual bool PostVerify() = 0;
    virtual ~Argument() = default;

    bool IsFlag() const {
        return is_flag_;
    }

    bool IsPositional() const {
        return is_positional_;
    }

    bool IsMultiValue() const {
        return is_multivalue_;
    }

    size_t GetMinMultiValue() const {
        return min_multivalue_;
    }
};


template<typename T>
using ParseFunc = std::optional<T> (*)(std::string_view);


template <typename T, ParseFunc<T> Parse>
class GenericArgument : public Argument {
 protected:
    Storage<T> storage_;
    std::optional<T> default_value_;

 public:
    bool TryParse(std::string_view string) override {
        std::optional<T> parse_result = Parse(string);
        if (!parse_result.has_value()) {
            return false;
        }
        bool did_fit = storage_.AddValue(parse_result.value());
        return did_fit;
    }

    std::any GetValue() override {
        T* value = storage_.GetValue();
        if (value) {
            return *value;
        } else {
            return std::any();
        }
    }

    std::any GetValues() override {
        std::vector<T>* values = storage_.GetValues();
        if (values) {
            return *values;
        } else {
            return std::any();
        }
    }

    GenericArgument& StoreValue(T& storage) {
        storage_.SetExternal(storage);
        return *this;
    }

    GenericArgument& StoreValues(std::vector<T>& storage) {
        storage_.SetExternal(storage);
        return *this;
    }

    GenericArgument& Default(T value) {
        default_value_ = value;
        return *this;
    }

    GenericArgument& Positional(bool is_positional = true) {
        is_positional_ = is_positional;
        return *this;
    }

    GenericArgument& MultiValue(size_t min_count = 0) {
        is_multivalue_ = true;
        min_multivalue_ = min_count;
        this->storage_.SetMulti(true);
        return *this;
    }

    bool PostVerify() override {
        if (is_multivalue_) {
            if (storage_.GetValues() == nullptr) {
                return false;
            }

            return storage_.GetValues()->size() >= min_multivalue_;
        }

        if (!storage_.IsEmpty()) {
            return true;
        }

        if (!default_value_.has_value()) {
            return false;
        }

        return storage_.AddValue(default_value_.value());
    }
};

std::optional<bool> AlwaysTruthy(std::string_view);

template <typename T>
std::optional<T> ParseFromChars(std::string_view view) {
    T value;

    auto [ptr, ec] = std::from_chars(view.begin(), view.end(), value);

    if (ec != std::errc()) {
        return std::nullopt;
    }

    if (ptr != view.end()) {
        return std::nullopt;
    }

    return value;
}

std::optional<std::string> ParseString(std::string_view view);

typedef GenericArgument<int, ParseFromChars<int>> IntArgument;
typedef GenericArgument<long, ParseFromChars<long>> LongArgument;
typedef GenericArgument<float, ParseFromChars<float>> FloatArgument;
typedef GenericArgument<double, ParseFromChars<double>> DoubleArgument;
typedef GenericArgument<std::string, ParseString> StringArgument;

class FlagArgument : public GenericArgument<bool, AlwaysTruthy> {
 public:
    FlagArgument() {
        default_value_ = false;
        is_flag_ = true;
    }
};

} // namespace ArgumentParser
