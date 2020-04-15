# clixx

CLI++ is a header-only library that simplifies implenting command line interfaces.

This library is inspired by mow.cli

## Usage

The following example demonstrates basic usage of the library:

```cpp
auto main(int argc, char* argv[]) -> int
{
    // declare storage for flags and arguments accessible by all commands and
    // subcommands
    std::optional<bool> verbose;
    std::string filename;

    auto cl = cli::app{"App Description"};

    cl.subcommand("info", "show information", [](cli::command& cmd) {
        // declare parameter storage accessible by "info" subcommand
        std::optional<bool> detailed;
        cmd.flag(detailed, "", "d", "show detailed info");
        cmd.action = []() { printf("executing info command\n"); };
    });

    cl.flag(verbose, "verbose", "v", "show detailed info");
    cl.arg(filename, "FILENAME", "load filename");
    cl.action = []() { printf("executing root command\n"); };

    try {
        cl.execute(argc, argv);
    }
    catch (const cli::help& msg) {
        printf("%s\n", msg.what());
    }
    catch (const cli::error& msg) {
        printf("error: %s\n", msg.what());
    }

    return 0;
}
```

To define our terms, consider the following command line example: 

```bash
exename command1 command2 -f --flag=val arg1 arg2 --another_flag
```

> Note: this library expects all components of the command line properly
> unquoted and chunked. This is what you normally get within `argc`, `argv`
> parameters of your `main(...)` entry point.

First chunk is the name of the executable. The name of the executable is
followed by optional command or an optional sequence of commands. The commands, then are followed by a mix of flags and arguments that are relevant in the context of the command.


## Declaring Root Handler

Create root handler instance :

```cpp
auto cl = cli::app("my app description");
```

To print out usage, the library will get the name of the executable from the
command line. You can also explicitly specify executable name in the second
overloaded version of the constructor:

```cpp
auto cl = cli::app("exename", "my app description");
```

Once done with all the bindings, as described in following sections, use one of
the `cli::app::execute()` methods to parse the command line and dispatch to
appropriate root or subcommand callbacks:

```cpp
auto main

try {
    // parse command line
    cl.execute(argc, argv);
    // or cl.execute({"exename", "cmd", "-f", "arg", etc...});
}
catch (const cli::help& msg) {
    // print out usage
    printf("%s\n", msg.what());
}
catch (const cli::error& msg) {
    // report command line parsing errors
    printf("error: %s\n", msg.what());
}
```

## Implementing Commands

An optional command or a chain of commands and subcommands immediately follow
the executable.

For example:

- `myexe`: executes root command
- `myexe add`: executes "add" command
- `myexe submodule add`: executes "submodule" command with "add" subcommand
```

Command and subcommand handlers are created with `cli::app::subcommand` method:

```cpp
cl.subcommand("cmdname", "cmd description", [](cli::command& cmd) {
    // declare local flags and arguments here...
    cmd.flag(...);
    cmd.arg(...);
    // declare command's actuion callback
    cmd.action = []() { 
        // cmdname handler
        printf("executing cmdname\n"); 
    };

    // declare subcommands, if desired:
    cmd.subcommand("subcommandname", "subcommand description", [](cli::command& subcmd){
        ...
        subcmd.action = []() { ... };
    });
});
```

The chunks of command line is parsed left to right. After skipping over the
first chunk (exename), the parser tries to match the next chunk to one of the
known commands. If an exact match found, the parser then switches to its
handler, otherwise it will try to interpret it as a flag or as an argument.

> There is no limitation on the symbols used within command's name. The parser
> will perform exact matching of the incoming chunk with the command name. All
> commands, therefore, are case sensitive.

> To avoid ambiguity between commands and flags, it is recommended not to start
> names with dashes. In addition, also avoid using spaces because then you would
> have to wrap commands in quotes. In general, it is better to restrain to lower
> case ASCII letters.


## Handling Flags and Arguments

Command line flags can have a short single-letter form (`-f`) or a long form
(`--verbose`). Some flags may specify optional or mandatory values separated by
equals sign (`--std=<value>`) or space (`--std <value>`).

Arguments are just values without any prefixes (`filename.ext`).

When declaring a flag or an argument, you will need bind it to a runtime storage
(a variable, a struct member, etc.), which may be one of the following types:

- `bool`
- `std::optional<bool>`
- `std::string`
- `std::optional<string>`
- `std::vector<std::string>`

Flags are declared with `app::flag(...)` or `command::flag(...)` method:

```cpp
bool bool_target;
std::string string_target;
cmd.flag(bool_target, "verbose", "vV", "show detailed info");
cmd.flag(string_target, "output", "o", "output file name", "sample");
```

The first parameter is a reference to target storage, the second parameter is a
long name of the flag to be used with double dash. The third parameter is a
sequence of letters that are accepted as single-letter forms. The fourth parameter
is a description that will be used when printing out flag usage.

An optional last parameter is used for more detailed usage printouts in the
form: `-f=<sample>` instead of the default `-f`.

> If you don't want a long form or a short form of the flag, simply pass an
> empty string as a parameter.

> Flags are not case-sensitive, but you can make short forms case-insensitive by
> specifying both cases in the third parameter.

> Flags defined in parents propagate to child commands.

> In the current implementation, the library only supports single and double
> dashes for passing flags. Forward slash flags `/f` are not recognized.

Arguments are declared with `app::arg(...)` or `command::arg(...)` method:

```cpp
std::string string_target;
cmd.arg(string_target, "argname", "description");
```

The `argname` parameter is only used for printing out usage information.

### Boolean Flags

A flag is declared to be boolean if you bind it to a `bool` or
`std::optional<bool>` target. Flags that are bound to `bool` are mandatory, if
they don't appear in the command line, it's an error. Flags that are bound ot
`std::optional<bool>` are optional.

Supported syntax includes:
- stand-alone flags: `-f`, `--flag`;
- with space or equal sign separated (true/false) values: `-f=true`, `-f=false`,
  `-f true`, `-f false`;
- folding: `-a -b -c` can be specified as `-abc`;

### Boolean Flag Lists

> This section/implementation is incomplete

### String Flags

String flags that appear in the command line must be followed by a value. Both
`-f=<value>` and `-f <value>` versions are acceptable. A stand alone `-f` in
case of a string flag is an error.

A flag that is bound to a `string` storage is considered a mandatory string
flag; it must be specified in the command line exactly once.

A flag that is bound to a `std::optional<string>` is optional, however if it
appears in the command line, it must be followed by a value as any other string
flag.

A flag that is bound to a `std::vector<std::string>` may appear 0, 1, or
multiple times.

### Arguments

Arguments are typically bound to `string`, `std::optional<string>`, or
`std::vector<std::string>` targets providing mandatory, optional, multi-value
semantics.

You can also bind those to boolean targets, in which case arguments that contain
`true` or `false` will be recognized.

When declaring arguments, apply general sanity rules. Optional and multi-value
flags must be resolvable without ambiguity:

- `MANDATORY1 OPTIONAL MANDATORY2` sequence is resolvable
- `MANDATORY1 MULTI-VAL` sequence is resolvable
- `MULTI-VAL MANDATORY` sequence is resolvable
- `MULTI-VAL OPTIONAL` is ambiguous
- `OPTIONAL MULTI_VAL` is ambiguous
- etc.

## Printing Usage/Help

Root handler exposes two `std::string` members: `help_cmd` (default is empty
string) and `help_flag` (default is `--help`). Appearance of those in a command
line triggers a special `cli::help` exception that can be used for usage print
outs.

The messages that are generated are context sensitive. Meaning that `exename
--help` will print out usage info applicable to root command, while `exename CMD
--help` will print out usage information applicable to CMD subcommand.

By assigning a value to `help_cmd` you can also provide an alternative syntax in
the form of `exename help` and `exename help CMD`.

