#pragma once

#include <filesystem>
#include "http_response.h"


namespace http_handler {

namespace fs = std::filesystem;


class FileRequestHandler {
    // это все мне не нужно
    FileRequestHandler(const FileRequestHandler &) = delete;
    FileRequestHandler& operator=(const FileRequestHandler &) = delete;
    FileRequestHandler(FileRequestHandler&&) = delete;
    FileRequestHandler& operator=(FileRequestHandler&&) = delete;

public:
    explicit FileRequestHandler(const fs::path& root);

    VariantResponse HandleRequest(StringRequest &&req);

private:
    static std::string_view GetMimeType(const fs::path& target);
    static bool IsDirectory(const fs::path &target) noexcept;
    static std::uintmax_t GetFileSize(const fs::path& target) noexcept;
    static bool CheckFileExistence(const fs::path &target) noexcept;
    bool CheckFileSubdir(const fs::path &target) noexcept;
    VariantResponse MakeFileResponse(StringRequest &&req, const fs::path &target);

private:
    fs::path root_;
};

}   // namespace http_handler
