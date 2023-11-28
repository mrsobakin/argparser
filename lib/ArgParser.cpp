#include "ArgParser.h"

#include <any>
#include <optional>
#include <sstream>
#include <string>

#include "Arguments.h"


namespace ArgumentParser {

std::optional<Argument*> ArgParser::GetArgument(std::string_view arg_name) {
    auto argument = long_arguments.find(arg_name);

    if (argument == long_arguments.end()) {
        return std::nullopt;
    }

    return argument->second;
}

std::optional<Argument*> ArgParser::GetArgument(char arg_name) {
    auto argument = short_arguments.find(arg_name);

    if (argument == short_arguments.end()) {
        return std::nullopt;
    }

    return argument->second;
}

void ArgParser::AddArgument(char short_name, std::string long_name, Argument* argument) {
    arguments.push_back(argument);
    arguments_names.push_back({short_name, long_name});
    long_arguments[long_name] = argument;
    short_arguments[short_name] = argument;
}

void ArgParser::AddArgument(std::string long_name, Argument* argument) {
    arguments.push_back(argument);
    arguments_names.push_back({std::nullopt, long_name});
    long_arguments[long_name] = argument;
}


bool ArgParser::ParseShort(std::vector<std::string_view>::const_iterator& token, std::vector<std::string_view>::const_iterator end) {
    for (size_t i = 1; i < token->size(); ++i) {
        auto maybe_argument = GetArgument((*token)[i]);

        if (!maybe_argument.has_value()) {
            return false;
        }

        auto argument = maybe_argument.value();

        if (!argument->IsFlag()) {
            std::string_view value = token->substr(i + 1);

            if (value.starts_with('=')) {
                // The tests say that short argument value
                // should be specified via -s=value
                // However, this is not consistent with GNU
                // cli arguments style: -svalue
                // So, I picked the even-more-inconsistent
                // middle ground: ignore the first '='.
                value = value.substr(1);
            } else if (value.empty()) {
                if (++token == end) {
                    return false;
                }
                value = *token;
            }

            return argument->TryParse(value);
        }

        if (!argument->TryParse("")) {
            return false;
        }
    }
    return true;
}

bool ArgParser::ParseLong(std::vector<std::string_view>::const_iterator& token, std::vector<std::string_view>::const_iterator end) {
    size_t idx = token->find('=');

    std::string_view name;
    if (idx != std::string_view::npos) {
        name = token->substr(2, idx - 2);
    } else {
        name = token->substr(2);
    }

    auto maybe_argument = GetArgument(name);
    if (!maybe_argument.has_value()) {
        return false;
    }

    auto argument = maybe_argument.value();

    if (argument->IsFlag()) {
        return argument->TryParse("");
    }

    std::string_view value;
    if (idx != std::string_view::npos) {
        value = token->substr(idx + 1);
    } else {
        if (++token == end) {
            return false;
        }
        value = *token;
    }

    return argument->TryParse(value);
}

bool ArgParser::ParsePositional(const std::vector<std::string_view>& args) {
    std::vector<Argument*> leftside_args;
    Argument* multiarg = nullptr;
    std::vector<Argument*> rightside_args;

    // Split arguments into left*, multi, right*
    {
        std::vector<Argument*> pos_arguments;
        for (auto arg : arguments) {
            if (arg->IsPositional()) {
                pos_arguments.push_back(arg);
            }
        }

        size_t i = 0;
        while (i < pos_arguments.size()) {
            if (pos_arguments[i]->IsMultiValue()) {
                break;
            }

            leftside_args.push_back(pos_arguments[i]);
            ++i;
        }

        if (i < pos_arguments.size()) {
            multiarg = arguments[i];
            ++i;
        }

        while (i < pos_arguments.size()) {
            if (pos_arguments[i]->IsMultiValue()) {
                return false;
            }
            rightside_args.push_back(pos_arguments[i]);
            ++i;
        }
    }

    if (args.size() < (leftside_args.size() + rightside_args.size())) {
        return false;
    }

    for (size_t i = 0; i < leftside_args.size(); ++i) {
        if (!leftside_args[i]->TryParse(args[i])) {
            return false;
        }
    }

    for (size_t i = 0; i < rightside_args.size(); ++i) {
        if (!rightside_args[rightside_args.size() - i - 1]->TryParse(args[args.size() - i - 1])) {
            return false;
        }
    }

    for (size_t i = leftside_args.size(); i < args.size() - rightside_args.size(); ++i) {
        if (!multiarg->TryParse(args[i])) {
            return false;
        }
    }

    return true;
}

bool ArgParser::PostVerify() {
    for (auto arg : arguments) {
        if (!arg->PostVerify()) {
            return false;
        }
    }
    return true;
}


bool ArgParser::Parse(const std::vector<std::string>& args) {
    std::vector<std::string_view> views;
    for (auto& arg : args) {
        views.push_back(std::string_view(arg));
    }
    return Parse(views);
}

bool ArgParser::Parse(size_t argc, char** argv) {
    std::vector<std::string_view> views;
    for (size_t i = 0; i < argc; ++i) {
        views.push_back(std::string_view(argv[i]));
    }
    return Parse(views);
}

bool ArgParser::Parse(const std::vector<std::string_view>& args) {
    auto token = args.cbegin();
    bool doubledash = false;

    if (args.empty()) {
        return PostVerify();
    }

    std::vector<std::string_view> positional_args;

    while (++token != args.end()) {
        if (doubledash) {
            positional_args.push_back(*token);
        } else if (token->starts_with("--")) {
            if (token->size() == 2) {
                doubledash = true;
            } else {
                if (!ParseLong(token, args.end())) {
                    return false;
                }
            }
        } else if (token->starts_with("-") && token->size() != 1) {
            if(!ParseShort(token, args.end())) {
                return false;
            }
        } else {
            positional_args.push_back(*token);
        }
    }

    if (!ParsePositional(positional_args)) {
        return false;
    }

    return Help() || PostVerify();
}


std::any ArgParser::GetValue(std::string_view parameter_name) {
    auto argument = GetArgument(parameter_name);
    if (!argument.has_value()) {
        return std::any();
    }

    return argument.value()->GetValue();
}


std::string ArgParser::ArgumentDescription(size_t i) {
    std::ostringstream ss;
    if (arguments[i]->IsPositional()) {
        ss << arguments_names[i].second << ',';
        ss << '\t';
    } else {
        if (arguments_names[i].first.has_value()) {
            ss << '-' << arguments_names[i].first.value() << ',';
        } else {
            ss << "   ";
        }
        ss << "  ";

        ss << "--" << arguments_names[i].second;

        if (!arguments[i]->IsFlag()) {
            ss << "=value";
        }

        ss << "," << "  ";
    }

    ss << arguments[i]->description;

    if (arguments[i]->IsMultiValue()) {
        ss << "  " << '[' << "repeated";
        if (arguments[i]->GetMinMultiValue()) {
            ss << ", min args = " << arguments[i]->GetMinMultiValue();
        }
        ss << ']';
    }

    return ss.str();
}

std::string ArgParser::HelpDescription() {
    std::ostringstream ss;

    ss << program_name;
    ss << '\n';
    ss << description;
    ss << "\n\n";

    for (size_t i = 0; i < arguments.size(); ++i) {
        ss << ArgumentDescription(i) << '\n';
    }

    return ss.str();
}

} // namespace ArgumentParser
