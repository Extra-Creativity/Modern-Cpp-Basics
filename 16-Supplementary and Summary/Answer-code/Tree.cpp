#include <cassert>
#include <filesystem>
#include <iostream>
#include <optional>
#include <print>
#include <vector>

namespace stdfs = std::filesystem;

struct TreeArgs
{
    stdfs::path root{ "." };
    int maxDepth = std::numeric_limits<int>::max();
};

// 写得比较简陋，可以用argparse库或者自己改进一下。
std::optional<TreeArgs> ParseArgs(int argc, const char *argv[])
{
    TreeArgs args;
    if (argc > 4)
    {
        std::println(stderr, "USAGE: PROGRAM [-L max_depth] [directory_path]");
        return std::nullopt;
    }
    if (argc == 1)
        return args; // Default settings

    if (argc == 2) // directory_path
        args.root = argv[1];
    else // -L number [directory_path]
    {
        if (auto depthOpt = std::string{ argv[1] }; depthOpt != "-L")
        {
            std::println(stderr, "Unrecognized option: {}", depthOpt);
            return std::nullopt;
        }

        args.maxDepth = std::stoi(argv[2]);
        if (argc == 4)
            args.root = argv[3];
    }

    return args; // Implicit move.
}

void Tree(const TreeArgs &args)
{
    std::vector<std::string> prefixes;
    std::vector<stdfs::directory_iterator> iterators;

    auto AssignOrAppendPrefix = [&](int depth, std::string_view str) {
        if (prefixes.size() <= depth)
            prefixes.push_back(std::string{ str });
        else
            prefixes[depth] = str;
    };

    auto AssignOrAppendIterator = [&](int depth,
                                      stdfs::directory_iterator &&it) {
        if (iterators.size() <= depth)
            iterators.push_back(std::move(it));
        else
            iterators[depth] = std::move(it);
    };

    auto PrintRawPath = [](const stdfs::path &path) {
        if constexpr (std::is_same_v<stdfs::path::value_type, char>)
        { // To prevent copy.
            std::cout << path.native() << "\n";
        }
        else
        {
            std::cout << path.string() << "\n";
        }
        // If you want to process in UTF-8 canonically:
        // auto u8Path = path.u8string();
        // std::string_view rawView{
        //     reinterpret_cast<const char *>(u8Path.c_str()), u8Path.size()
        // };
        // std::println("{}", rawView);
    };

    auto PrintPrefix = [&](int depth) {
        for (int i = 0; i <= depth; i++)
            std::print("{}", prefixes[i]);
    };

    auto root = args.root.lexically_normal();
    iterators.emplace_back(root,
                           stdfs::directory_options::skip_permission_denied);
    PrintRawPath(root);
    if (args.maxDepth == 0)
        return; // Print nothing else.

    int currDepth = 0;
    stdfs::path currFileName;
    const auto end = stdfs::directory_iterator{};
    while (currDepth >= 0)
    {
        bool advanceToNextLevel = false;
        {
            auto &it = iterators[currDepth];
            assert(it != end);
            auto &entry = *it;
            currFileName = entry.path().filename();
            if (entry.is_directory() && !entry.is_symlink() &&
                currDepth < args.maxDepth - 1) // Don't follow symlink.
            {
                // TOC/TOU: directory may be invalid now, then don't advance.
                std::error_code err;
                auto nextLevelIterator = stdfs::directory_iterator{
                    entry.path(),
                    stdfs::directory_options::skip_permission_denied, err
                };
                // Empty directory or error.
                advanceToNextLevel = (nextLevelIterator != end) && !err;
                // Well, it's okay to not add if (advanceToNextLevel) here.
                AssignOrAppendIterator(currDepth + 1,
                                       std::move(nextLevelIterator));
            }
        }
        // We need to re-index iterators, since vector may be reallocated so
        // original reference can be invalidated.
        bool endCurrLevel = (++iterators[currDepth] == end);
        if (endCurrLevel)
        {
            AssignOrAppendPrefix(currDepth, "\u2514\u2500\u2500 ");
            PrintPrefix(currDepth);
            prefixes[currDepth] = "    ";
        }
        else
        {
            AssignOrAppendPrefix(currDepth, "\u251C\u2500\u2500 ");
            PrintPrefix(currDepth);
            prefixes[currDepth] = "\u2502   ";
        }

        PrintRawPath(currFileName);
        if (advanceToNextLevel)
            currDepth++;
        else if (endCurrLevel)
        { // Pop invalid handles; currDepth has been checked so use pre-dec.
            while (--currDepth >= 0 && iterators[currDepth] == end)
                ;
        }
    }
}

int main(int argc, const char *argv[])
{
    try
    {
        auto args = ParseArgs(argc, argv);
        if (!args.has_value())
            return 0;
        Tree(*args);
    }
    catch (const std::exception &ex)
    {
        std::println(stderr, "{}", ex.what());
        return 1;
    }
}