#include "ps3tool.h"
#include <boost/program_options.hpp>
#include <boost/variant.hpp>
#include <boost/hana.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace boost::program_options;
namespace hana = boost::hana;

struct NoneCommand {};
struct BadCommand {};

typedef boost::variant<NoneCommand,
                       BadCommand,
                       ShaderDasmCommand,
                       UnsceCommand,
                       RestoreElfCommand,
                       ReadPrxCommand,
                       ParseSpursTraceCommand,
                       RewriteCommand,
                       PrxStoreCommand,
                       RsxDasmCommand,
                       FindSpuElfsCommand,
                       DumpInstrDbCommand,
                       PrintGcmVizTraceCommand>
    Command;

template <typename C>
struct CommandParser {
    C command;
    options_description desc;
    std::string name;
    
    CommandParser(std::string name) : desc(name), name(name) {}
    virtual void init() = 0;
    virtual void parse(variables_map& vm) = 0;
    virtual ~CommandParser() = default;
    
    bool run(variables_map& vm, parsed_options& parsed, bool help) {
        init();
        desc.add_options() ("help", "produce help message");
        auto opts = collect_unrecognized(parsed.options, include_positional);
        if (begin(opts) != end(opts)) {
            opts.erase(begin(opts));
        }
        store(command_line_parser(opts).options(desc).run(), vm);
        if (vm.count("help") || help) {
            std::cout << desc;
            return false;
        }
        notify(vm);
        parse(vm);
        return true;
    }
};

struct ShaderParser : CommandParser<ShaderDasmCommand> {
    ShaderParser() : CommandParser<ShaderDasmCommand>("shader") {}
    
    void init() override {
        desc.add_options()(
            "binary", value<std::string>(&command.binary)->required(), "binary path");
    }
    
    void parse(variables_map& vm) override { }
};

struct UnsceParser : CommandParser<UnsceCommand> {
    UnsceParser() : CommandParser<UnsceCommand>("unsce") {}
    
    void init() override {
        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "output elf file")
            ("sce", value<std::string>(&command.sce)->required(), "input sce file")
            ("data", value<std::string>(&command.data)->required(), "data directory");
    }
    
    void parse(variables_map& vm) override { }
};

struct RestoreElfParser : CommandParser<RestoreElfCommand> {
    RestoreElfParser() : CommandParser<RestoreElfCommand>("restore-elf") {}
    
    void init() override {
        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ("dump", value<std::string>(&command.dump)->required(), "dump file")
            ("output", value<std::string>(&command.output)->required(), "output file");
    }
    
    void parse(variables_map& vm) override { }
};

struct ReadPrxParser : CommandParser<ReadPrxCommand> {
    ReadPrxParser() : CommandParser<ReadPrxCommand>("read-prx") {}
    
    void init() override {
        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ("script", "write Ida script");
    }
    
    void parse(variables_map& vm) override {
        command.writeIdaScript = vm.count("script");
    }
};

struct ParseSpursTraceParser : CommandParser<ParseSpursTraceCommand> {
    ParseSpursTraceParser() : CommandParser<ParseSpursTraceCommand>("parse-spurs-trace") {}
    
    void init() override {
        desc.add_options()
            ("dump", value<std::string>(&command.dump)->required(), "trace buffer dump file");
    }
    
    void parse(variables_map& vm) override { }
};

struct RewriteParser : CommandParser<RewriteCommand> {
    RewriteParser() : CommandParser<RewriteCommand>("rewrite") {}
    
    std::string imageBaseStr;
    
    void init() override {
        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ("cpp", value<std::string>(&command.cpp)->required(), "output cpp file")
            ("image-base", value<std::string>(&imageBaseStr)->default_value("0"), "image base")
            ("spu", bool_switch()->default_value(false), "spu")
            ;
    }
    
    void parse(variables_map& vm) override {
        command.imageBase = std::stoi(imageBaseStr, 0, 16);
        command.isSpu = vm["spu"].as<bool>();
    }
};

struct PrxStoreParser : CommandParser<PrxStoreCommand> {
    PrxStoreParser() : CommandParser<PrxStoreCommand>("prx-store") {}
    
    void init() override {
        desc.add_options()
            ("map", bool_switch()->default_value(false), "update prx segment bases in ps3 config")
            ("rewrite", bool_switch()->default_value(false), "rewrite prx binaries")
            ("verbose", bool_switch()->default_value(false), "verbose")
            ;
    }
    
    void parse(variables_map& vm) override {
        command.map = vm["map"].as<bool>();
        command.rewrite = vm["rewrite"].as<bool>();
        command.verbose = vm["verbose"].as<bool>();
    }
};

struct DumpInstrDbParser : CommandParser<DumpInstrDbCommand> {
    DumpInstrDbParser() : CommandParser<DumpInstrDbCommand>("dump-instrdb") {}
    void init() override { }
    void parse(variables_map& vm) override { }
};

struct RsxDasmParser : CommandParser<RsxDasmCommand> {
    RsxDasmParser() : CommandParser<RsxDasmCommand>("rsx-dasm") {}
    
    void init() override {
        desc.add_options()
            ("bin", value<std::string>(&command.bin)->required(), "bin file")
            ;
    }
    
    void parse(variables_map& vm) override { }
};
    
struct FindSpuElfsParser : CommandParser<FindSpuElfsCommand> {
    FindSpuElfsParser() : CommandParser<FindSpuElfsCommand>("find-spu-elfs") {}
    
    void init() override {
        desc.add_options()
            ("elf", value<std::string>(&command.elf)->required(), "elf file")
            ;
    }
    
    void parse(variables_map& vm) override { }
};

struct PrintGcmVizTraceParser : CommandParser<PrintGcmVizTraceCommand> {
    PrintGcmVizTraceParser() : CommandParser<PrintGcmVizTraceCommand>("print-gcmviz-trace") {}
    
    void init() override {
        desc.add_options()
            ("trace", value<std::string>(&command.trace)->required(), "trace file")
            ("frame", value<int>(&command.frame)->default_value(-1), "frame")
            ("command", value<int>(&command.command)->default_value(-1), "command")
            ;
    }
    
    void parse(variables_map& vm) override { }
};
    
Command ParseOptions(int argc, const char *argv[]) {
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
    
    auto parsers = hana::make_tuple(
        ShaderParser(),
        UnsceParser(),
        RestoreElfParser(),
        ReadPrxParser(),
        ParseSpursTraceParser(),
        RewriteParser(),
        PrxStoreParser(),
        RsxDasmParser(),
        FindSpuElfsParser(),
        DumpInstrDbParser(),
        PrintGcmVizTraceParser()
    );
    
    if (commandName == "") {
        std::cout << "Usage: ps3tool [COMMAND] [OPTIONS]...\n";
        hana::for_each(parsers, [&](auto parser) {
            std::cout << "    " << parser.name << std::endl;
        });
        return NoneCommand();
    }
    
    Command command = BadCommand();
    hana::for_each(parsers, [&](auto parser) {
        if (parser.name == commandName) {
            if (parser.run(vm, parsed, false)) {
                command = parser.command;
            } else {
                command = NoneCommand();
            }
        }
    });

    return command;
}

class CommandVisitor : public boost::static_visitor<> {
public:
    void operator()(NoneCommand& command) const { }
    
    void operator()(BadCommand& command) const { 
        std::cout << "unrecognized command\n";
    }
    
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
    
    void operator()(PrxStoreCommand& command) const {
        HandlePrxStore(command);
    }
    
    void operator()(RsxDasmCommand& command) const {
        HandleRsxDasm(command);
    }
    
    void operator()(FindSpuElfsCommand& command) const {
        HandleFindSpuElfs(command);
    }
    
    void operator()(DumpInstrDbCommand& command) const {
        HandleDumpInstrDb(command);
    }
    
    void operator()(PrintGcmVizTraceCommand& command) const {
        HandlePrintGcmVizTrace(command);
    }
};

int main(int argc, const char* argv[]) {
    try {
        auto command = ParseOptions(argc, argv);
        boost::apply_visitor(CommandVisitor(), command);
        return 0;
    } catch (std::exception& e) {
        std::cout << "error occured: " << e.what() << std::endl;
        return 1;
    }
}
