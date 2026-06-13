
#include <imgui.h>
#include <imgui_internal.h>
#include <stdlib.h> // abort
#include "args.hpp"
#include "records.hpp"
#include "ui.hpp"
int main(int argc, char **argv)
{
    args::ArgumentParser parser("A  tool for visualizing, analyzing & debugging event/scheduler in high-performance multi-concurrent embedded systems/kernels.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Group commands(parser, "commands");


    args::Command open_cmd(commands, "open", "Use spinetail to parse a records file in offline mode");
    args::Positional<std::string> open_cmd_file(open_cmd, "input", "The input file path", args::Options::Required);

    args::Command connect_cmd(commands, "connect", "Use spinetail to connect to a remote TCP server");
    args::Positional<std::string> connect_cmd_addr(connect_cmd, "address", "The remote server address.", args::Options::Required);

    args::CompletionFlag completion(parser, {"complete"});

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Completion &e)
    {
        std::cout << e.what();
        return 0;
    }
    catch (const args::Help &)
    {
        std::cout << parser;
        return 0;
    }
    catch (const args::RequiredError &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (const args::ParseError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (open_cmd)
    {
        std::string path = args::get(open_cmd_file);
        parse_records_from_file(path);
        RecordsManager::the().update_all_records();
        ui();
    }
    else if (connect_cmd)
    {
        std::string addr = args::get(connect_cmd_addr);
        printf("Connecting to %s...\n", addr.c_str());
        return -1;
    }

    return 0;
}
