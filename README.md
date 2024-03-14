# ♾ ArgParser

Type-safe\* & exception-free CLI argument parser written in C++, powered by the template metaprogramming magic 🪄.

## 🧐 Overview

This library was written as a lab work in my university. Hence, it contains some dirty hacks (like the function alias macro) and unsafe functions (like the `ArgParser::GetValueDangerous<T>`), that exist for the sole purpose of complying with the original task. However, even with that said, this library is completely downcast free, does not throw exceptions and runtime errors, and is type-safe. Also, it is easuly extensible (you can see the examples below), and it does not require niether modifying the source code, niether writing boilerplate code and using a ten-layer inheritance.

## ✨ Features

- **Completely Type-Safe (if you want it to be)**: You can either choose the methods that unsafely cast the underlying objects and unwrap the `std::optional`s or not. Aside from these functions, the `ArgParser` library is completely type-safe and runtime-error free.
- **Easily Extensible**: `ArgParser` supports adding custom arguments with minimal effort.
- **Default Values**: Define default values for arguments to handle cases where arguments are not provided.
- **Flag Arguments**: Define flags that can be toggled on or off.
- **Multi-Value Arguments**: Support for parsing multiple values for an argument.
- **Positional Arguments**: Parse arguments based on their position in the command line.
- **Help Command**: Automatic generation of the help page.

## 🚀 Usage example

```cpp
#include <lib/ArgParser.h>

using namespace ArgumentParser;

int main(int argc, char** argv) {
    ArgParser parser("Some CLI Program");
    parser.AddStringArgument("param1");

    if (parser.Parse(argc, argv)) {
        std::string param1_value = parser.GetStringValue("param1");
        // Use param1_value as needed
    }

    return 0;
}
```

## 🛠️ Adding custom arguments

In addition to built-in types like strings and integers, the argument parser supports custom types. You can define a custom argument type by providing a parsing function for your type.

Suppose you have a custom type `CustomType`:

```cpp
using namespace ArgumentParser;

struct CustomType {
    int value1;
    float value2;
};

std::optional<CustomType> ParseCustomType(std::string_view view) {
    // Implementation details...
    // Parse values from view and construct CustomType object
}

using CustomArgument = GenericArgument<MyType, ParseCustomType>;

int main() {
    ArgParser parser("Some CLI Program");

    parser.AddArgument<CustomArgument>('c', "custom", "This is a custom argument!");

    if (parser.Parse({"myapp", "--custom", "argument_value"})) {
        CustomType val = parser.GetValue<CustomType>().value();
        std::cout << val.value1 << ' ' << val.value2 << std::endl;
    }

    return 0;
}
```

If your argument has a custom logic that spans beyond parsing, you can manually derive from either `Argument` interface, or from `GenericArgument<T, P>`.
