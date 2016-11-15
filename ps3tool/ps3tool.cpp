#include "ps3tool.h"
#include <boost/program_options.hpp>
#include <boost/variant.hpp>
#include <string>
#include <vector>
#include <iostream>

using namespace boost::program_options;

typedef boost::variant<ShaderDasmCommand,
                       UnsceCommand,
                       RestoreElfCommand,
                       ReadPrxCommand,
                       ParseSpursTraceCommand,
                       RewriteCommand>
    Command;

Command ParseOptions(int argc, const char *argv[])
{
    options_description global("Global options");
    std::string commandName;
    global.add_options()
        ("subcommand", value<std::string>(&commandName), "command")
        ("subargs", value<std::vector<std::string> >(), "arguments");

    positional_options_description pos;
    pos.add("subcommand", 1).
        add("subargs", -1);

    variables_map vm;

    parsed_options parsed = command_line_parser(argc, argv).
        options(global).
        positional(pos).
        allow_unregistered().
        run();

    store(parsed, vm);
    notify(vm);

    if (commandName == "shader") {
        ShaderDasmCommand command;
        options_description desc("shader");

        desc.add_options()
            ("binary", value<std::string>(&command.binary)->required(), "binary path");

        auto opts = collect_unrecognized(parsed.options, include_positional);
        opts.erase(opts.begin());
        store(command_line_parser(opts).options(desc).run(), vm);
        notify(vm);
        return command;
    } else if (commandName == "unsce") {
        UnsceCommand command;
        options_description desc("unsce");

        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "output elf file")
            ("sce", value<std::string>(&command.sce)->required(), "input sce file")
            ("data", value<std::string>(&command.data)->required(), "data directory");

        auto opts = collect_unrecognized(parsed.options, include_positional);
        opts.erase(opts.begin());
        store(command_line_parser(opts).options(desc).run(), vm);
        notify(vm);
        return command;
    } else if (commandName == "restore-elf") {
        RestoreElfCommand command;
        options_description desc("restore-elf");

        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ("dump", value<std::string>(&command.dump)->required(), "dump file")
            ("output", value<std::string>(&command.output)->required(), "output file");

        auto opts = collect_unrecognized(parsed.options, include_positional);
        opts.erase(opts.begin());
        store(command_line_parser(opts).options(desc).run(), vm);
        notify(vm);
        return command;
    } else if (commandName == "read-prx") {
        ReadPrxCommand command;
        options_description desc("read-prx");

        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ("script", "write Ida script");

        auto opts = collect_unrecognized(parsed.options, include_positional);
        opts.erase(opts.begin());
        store(command_line_parser(opts).options(desc).run(), vm);
        notify(vm);
        command.writeIdaScript = vm.count("script");
        return command;
    } else if (commandName == "parse-spurs-trace") {
        ParseSpursTraceCommand command;
        options_description desc("parse-spurs-trace");

        desc.add_options()
            ("dump", value<std::string>(&command.dump)->required(), "trace buffer dump file");

        auto opts = collect_unrecognized(parsed.options, include_positional);
        opts.erase(opts.begin());
        store(command_line_parser(opts).options(desc).run(), vm);
        notify(vm);
        return command;
    } else if (commandName == "rewrite") {
        RewriteCommand command;
        options_description desc("rewrite");

        std::vector<std::string> entries, ignored;
        
        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ("cpp", value<std::string>(&command.cpp)->required(), "output cpp file")
            ("trace", bool_switch()->default_value(false), "enable trace")
            ("entries", value<std::vector<std::string>>(&entries)->multitoken(), "entry points")
            ("ignored", value<std::vector<std::string>>(&ignored)->multitoken(), "ignored entry points");
        
        auto opts = collect_unrecognized(parsed.options, include_positional);
        opts.erase(opts.begin());
        store(command_line_parser(opts).options(desc).run(), vm);
        notify(vm);
        command.trace = vm["trace"].as<bool>();
        
        for (auto str : entries) {
            command.entryPoints.push_back(std::stoi(str, 0, 16));
        }
        
        for (auto str : ignored) {
            command.ignoredEntryPoints.push_back(std::stoi(str, 0, 16));
        }
        
        return command;
    } else {
        throw std::runtime_error("unknown command");
    }
}

class CommandVisitor : public boost::static_visitor<> {
public:
    void operator()(ShaderDasmCommand& command) const {
        HandleShaderDasm(command);
    }

    void operator()(UnsceCommand& command) const {
        HandleUnsce(command);
    }

    void operator()(RestoreElfCommand& command) const {
        HandleRestoreElf(command);
    }

    void operator()(ReadPrxCommand& command) const {
        HandleReadPrx(command);
    }
    
    void operator()(ParseSpursTraceCommand& command) const {
        HandleParseSpursTrace(command);
    }
    
    void operator()(RewriteCommand& command) const {
        HandleRewrite(command);
    }
};

int main(int argc, const char* argv[]) {
    try {
        auto command = ParseOptions(argc, argv);
        boost::apply_visitor(CommandVisitor(), command);
    } catch (std::exception& e) {
        std::cout << "error occured: " << e.what() << std::endl;
    }
}
