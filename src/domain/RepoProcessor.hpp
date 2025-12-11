#pragma once

#include <string>
#include <vector>
#include <expected>
#include <regex>
#include <ranges>
#include <openssl/sha.h>
#include <sstream>

namespace gh::domain {

    struct FileEntry {
        std::string path;
        std::string content;
    };

    struct ProcessResult {
        std::string preview_content;
        std::string sha512_hash;
        size_t files_processed;
    };

    template<typename T>
    concept StringMatcher = requires(T a, std::string s) {
        { a.matches(s) } -> std::convertible_to<bool>;
    };

    class RepoProcessor {
    public:
        static std::expected<ProcessResult, std::string> process_repository(
            const std::vector<FileEntry>& files,
            const std::string& include_regex,
            const std::string& exclude_regex
        ) {
            std::stringstream full_concat;
            SHA512_CTX sha256;
            SHA512_Init(&sha256);

            std::regex inc_re(include_regex.empty() ? ".*" : include_regex);
            std::regex exc_re(exclude_regex.empty() ? "$^" : exclude_regex);

            auto filtered_view = files | std::views::filter([&](const FileEntry& f) {
                return std::regex_search(f.path, inc_re) && !std::regex_search(f.path, exc_re);
            });

            size_t count = 0;
            std::vector<const FileEntry*> sorted_files;
            for (const auto& f : filtered_view) sorted_files.push_back(&f);
            
            std::ranges::sort(sorted_files, [](const auto* a, const auto* b) {
                return a->path < b->path;
            });

            for (const auto* file : sorted_files) {
                std::string header = std::format("--- FILE: {} ({}) ---\n", file->path, file->content.size());
                
                full_concat << header << file->content << "\n";

                SHA512_Update(&sha256, header.c_str(), header.size());
                SHA512_Update(&sha256, file->content.c_str(), file->content.size());
                count++;
            }

            unsigned char hash[SHA512_DIGEST_LENGTH];
            SHA512_Final(hash, &sha256);

            std::string hash_string;
            for(int i = 0; i < SHA512_DIGEST_LENGTH; ++i)
                hash_string += std::format("{:02x}", hash[i]);

            return ProcessResult{ full_concat.str(), hash_string, count };
        }
    };
}
