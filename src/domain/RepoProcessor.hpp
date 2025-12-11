#pragma once

#include <string>
#include <vector>
#include <expected>
#include <unordered_set>
#include <ranges>
#include <algorithm>
#include <openssl/sha.h>
#include <sstream>
#include <format>

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

    class RepoProcessor {
    public:
        static std::expected<ProcessResult, std::string> process_repository(
            const std::vector<FileEntry>& downloaded_files,
            const std::vector<std::string>& allowed_paths_vec
        ) {
            std::unordered_set<std::string> allowed_paths(allowed_paths_vec.begin(), allowed_paths_vec.end());
            
            std::stringstream full_concat;
            SHA512_CTX sha512;
            SHA512_Init(&sha512);

            auto filtered_view = downloaded_files | std::views::filter([&](const FileEntry& f) {
                return allowed_paths.contains(f.path);
            });

            std::vector<const FileEntry*> sorted_files;
            for (const auto& f : filtered_view) sorted_files.push_back(&f);
            
            std::ranges::sort(sorted_files, [](const auto* a, const auto* b) {
                return a->path < b->path;
            });

            size_t count = 0;
            for (const auto* file : sorted_files) {
                full_concat << file->content;
                SHA512_Update(&sha512, file->content.c_str(), file->content.size());

                if (!file->content.empty() && file->content.back() != '\n') {
                    full_concat << "\n";
                    SHA512_Update(&sha512, "\n", 1);
                }

                count++;
            }

            unsigned char hash[SHA512_DIGEST_LENGTH];
            SHA512_Final(hash, &sha512);

            std::string hash_string;
            for(int i = 0; i < SHA512_DIGEST_LENGTH; ++i)
                hash_string += std::format("{:02x}", hash[i]);

            return ProcessResult{ full_concat.str(), hash_string, count };
        }
    };
}
