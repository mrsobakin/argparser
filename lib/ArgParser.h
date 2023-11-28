#pragma once

#include <any>
#include <map>
#include <optional>
#include <sstream>
#include <string>

#include "Arguments.h"
#include "Storage.h"


// Cool macro to avoid writing boilerplate code.
// https://www.fluentcpp.com/2017/10/27/function-aliases-cpp/
#define ALIAS_TEMPLATE_FUNCTION(highLevelF, lowLevelF) \
template<typename... Args> \
inline auto highLevelF(Args&&... args) -> decltype(lowLevelF(std::forward<Args>(args)...)) \
{ \
    return lowLevelF(std::forward<Args>(args)...); \
}


namespace ArgumentParser {

class ArgParser {
 private:
    std::vector<Argument*> arguments;
    std::vector<std::pair<std::optional<char>, std::string>> arguments_names;
    std::map<char, Argument*, std::less<>> short_arguments;
    std::map<std::string, Argument*, std::less<>> long_arguments;
    bool is_help_requested = false;

 public:
    std::string program_name = "";
    std::string description = "";


 private:
    std::optional<Argument*> GetArgument(std::string_view arg_name);
    std::optional<Argument*> GetArgument(char arg_name);
    void AddArgument(char short_name, std::string long_name, Argument* argument);
    void AddArgument(std::string long_name, Argument* argument);


 public:
    ArgParser() {}
    ArgParser(std::string name) : program_name(name) {}

    // They don't worth implementing.
    ArgParser(const ArgParser& other) = delete;
    ArgParser(ArgParser&& other) = delete;
    ArgParser& operator=(const ArgParser& other) = delete;
    ArgParser& operator=(ArgParser&& other) = delete;

    ~ArgParser() {
        for (auto argument : arguments) {
            delete argument;
        }
    }


 private:
    bool ParseShort(std::vector<std::string_view>::const_iterator& token, std::vector<std::string_view>::const_iterator end);
    bool ParseLong(std::vector<std::string_view>::const_iterator& token, std::vector<std::string_view>::const_iterator end);
    bool ParsePositional(const std::vector<std::string_view>& args);
    bool PostVerify();

 public:
    bool Parse(const std::vector<std::string>& args);
    bool Parse(size_t argc, char** argv);
    bool Parse(const std::vector<std::string_view>& args);


 public:
    template <typename T>
    T& AddArgument(char short_name, std::string long_name, std::string arg_description = "") {
        T* argument = new T();
        argument->description = arg_description;
        AddArgument(short_name, long_name, argument);
        return *argument;
    }

    template <typename T>
    T& AddArgument(std::string long_name, std::string arg_description = "") {
        T* argument = new T();
        argument->description = arg_description;
        AddArgument(long_name, argument);
        return *argument;
    }

    ALIAS_TEMPLATE_FUNCTION(AddIntArgument, AddArgument<IntArgument>)
    ALIAS_TEMPLATE_FUNCTION(AddStringArgument, AddArgument<StringArgument>)
    ALIAS_TEMPLATE_FUNCTION(AddFloatArgument, AddArgument<FloatArgument>)
    ALIAS_TEMPLATE_FUNCTION(AddFlag, AddArgument<FlagArgument>)


 public:
    template <typename T>
    std::optional<T> GetValue(std::string_view parameter_name) {
        auto argument = GetArgument(parameter_name);
        if (!argument.has_value()) {
            return std::nullopt;
        }

        std::any some_value = argument.value()->GetValue();

        if (some_value.type() != typeid(T)) {
            return std::nullopt;
        }

        return std::any_cast<T>(some_value);
    }

    template <typename T>
    std::optional<T> GetValue(std::string_view parameter_name, size_t idx) {
        auto argument = GetArgument(parameter_name);
        if (!argument.has_value()) {
            return std::nullopt;
        }

        std::any some_value = argument.value()->GetValues();

        if (some_value.type() != typeid(std::vector<T>)) {
            return std::nullopt;
        }

        std::vector<T>& values = std::any_cast<std::vector<T>&>(some_value);

        if (idx >= values.size()) {
            return std::nullopt;
        }

        return values[idx];
    }

    std::any GetValue(std::string_view parameter_name);

    // The following functions exist for the sole
    // purpose of complying with the provided tests.
    // Ideally, they should be either removed, or made
    // safe by removing dangerous .value() unwrapping.

    template <typename T>
    T GetValueDangerous(std::string_view parameter_name) {
        return GetValue<T>(parameter_name).value();
    }

    template <typename T>
    T GetValueDangerous(std::string_view parameter_name, size_t idx) {
        return GetValue<T>(parameter_name, idx).value();
    }

    ALIAS_TEMPLATE_FUNCTION(GetIntValue, GetValueDangerous<int>)
    ALIAS_TEMPLATE_FUNCTION(GetStringValue, GetValueDangerous<std::string>)
    ALIAS_TEMPLATE_FUNCTION(GetFloatValue, GetValueDangerous<float>)
    ALIAS_TEMPLATE_FUNCTION(GetFlag, GetValueDangerous<bool>)


 private:
    std::string ArgumentDescription(size_t i);

 public:
    std::string HelpDescription();

    bool Help() {
        return is_help_requested;
    }

    void AddHelp(char short_name, std::string long_name, std::string program_description) {
        AddFlag(short_name, long_name, "Display this help and exit")
            .Default(false)
            .StoreValue(is_help_requested);

        this->description = program_description;
    }
};

} // namespace ArgumentParser

#undef ALIAS_TEMPLATE_FUNCTION
