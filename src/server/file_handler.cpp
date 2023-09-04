#include "../sdk.h"
#include "ci_string.h"
#include "file_handler.h"
#include "url.h"


namespace http_handler {

namespace sys = boost::system;


struct FileError {
    FileError() = delete;
    constexpr static std::string_view FILE_NOT_FOUND = "We are sorry, the file you requested cannot be found."sv;
    constexpr static std::string_view BAD_REQUEST = "The file you requested is outside server scope."sv;
    constexpr static std::string_view INVALID_METHOD = "Invalid method"sv;
};

struct FileName {
    FileName() = delete;
    constexpr static std::string_view INDEX = "index.html"sv;
};

FileRequestHandler::FileRequestHandler(const fs::path& root)
: root_(root) {
}

VariantResponse FileRequestHandler::HandleRequest(StringRequest &&req) {

    const auto method = req.method();
    if (method != http::verb::get && method != http::verb::head) {
        return InvalidMethodResponse(std::move(req), FileError::INVALID_METHOD, Allow::GET_HEAD);
    }

    //
    //  убираю url-encoding и
    //  пропускаю первый символ '/' который мне все портит
    //
    std::string decoded = url::Decode(req.target());
    std::string_view tail{decoded};
    if (tail[0] == '/') {
        tail.remove_prefix(1);
    }

    fs::path target = fs::weakly_canonical(root_ / tail);

    //
    //  Если URI-строка запроса задаёт путь к каталогу
    //  в файловой системе, должно вернуться содержимое
    //  файла index.html из этого каталога.  
    //

    if (IsDirectory(target)) {
        target /= FileName::INDEX;
    }

    if (!CheckFileSubdir(target)) {
        //
        //  Файл лежит в директории за пределами нашего корня
        //
        return PlainStringResponse(std::move(req), http::status::bad_request, FileError::BAD_REQUEST);
    }

    if (!CheckFileExistence(target)) {
        //
        //  Файла нет
        //
        return PlainStringResponse(std::move(req), http::status::not_found, FileError::FILE_NOT_FOUND);
    }

    return MakeFileResponse(std::move(req), target);

}

//
//  Получить ContentType по расширению файла
//
/* static */ 
std::string_view FileRequestHandler::GetMimeType(const fs::path& target)
{
    const util::ci_string ext = util::to_ci_string(target.extension());

    //
    //  в процессе работы добавления новых значений в хэш таблицу не предполагается
    //  а поиск работает за O(1) - поэтому хэш таблица
    //
    using mime_types = std::unordered_map<util::ci_string, std::string_view>;


    static const mime_types mimeTypes = {
        { ".htm",   ContentType::TEXT_HTML },
        { ".html",  ContentType::TEXT_HTML },
        { ".css",   ContentType::TEXT_CSS },
        { ".txt",   ContentType::TEXT_PLAIN },
        { ".js",    ContentType::TEXT_JS },
        { ".json",  ContentType::APP_JSON },
        { ".xml",   ContentType::APP_XML },
        { ".png",   ContentType::IMAGE_PNG },
        { ".jpe",   ContentType::IMAGE_JPEG },
        { ".jpg",   ContentType::IMAGE_JPEG },
        { ".jpeg",  ContentType::IMAGE_JPEG },
        { ".gig",   ContentType::IMAGE_GIF },
        { ".bmp",   ContentType::IMAGE_BMP },
        { ".ico",   ContentType::IMAGE_ICO },
        { ".tiff",  ContentType::IMAGE_TIFF },
        { ".tif",   ContentType::IMAGE_TIFF },
        { ".svg",   ContentType::IMAGE_SVG },
        { ".svgz",  ContentType::IMAGE_SVG },
        { ".mp3",   ContentType::AUDIO_MPEG }
    };

    if (auto it = mimeTypes.find(ext); it != mimeTypes.end()) {
        return it->second;
    }

    return ContentType::APP_OCT;
}

/* static */
bool FileRequestHandler::IsDirectory(const fs::path& target) noexcept
{
    sys::error_code ec;
    auto status = fs::status(target, ec);
    if (!ec && fs::is_directory(status)) {
        return true;
    }

    return false;
}

/* static */
std::uintmax_t FileRequestHandler::GetFileSize(const fs::path& target) noexcept
{
    sys::error_code ec;
    std::uintmax_t  size = fs::file_size(target, ec);

    return (ec) ? 0 : size;
}

/* static */
bool FileRequestHandler::CheckFileExistence(const fs::path& target) noexcept
{
    sys::error_code ec;
    auto status = fs::status(target, ec);
    if (!ec && fs::is_regular_file(status)) {
        return true;
    }

    return false;
}

bool FileRequestHandler::CheckFileSubdir(const fs::path& target) noexcept
{
    //
    // Оба пути уже приведены к каноничному виду (без . и ..)
    // Проверяем, что все компоненты root_ содержатся внутри target_
    //
    for (auto b = root_.begin(), p = target.begin(); b != root_.end(); ++b, ++p) {
        if (p == root_.end() || *p != *b) {
            return false;
        }
    }

    return true;
}

VariantResponse FileRequestHandler::MakeFileResponse(StringRequest &&req, const fs::path& target)
{
    FileResponse response(http::status::ok, req.version());

    auto mimeType = GetMimeType(target);
    response.set(http::field::content_type, mimeType);
    response.keep_alive(req.keep_alive());

    //
    //  если просят только заголовок - то файл вообще не читаю, отдаю только его размер
    //  (в эту функцию я могу попасть только с GET или HEAD)
    //
    const bool headOnly = (req.method() == boost::beast::http::verb::head);
    if (headOnly) {
        response.content_length(GetFileSize(target));
        return response;
    }

    //
    //  а здесь уже нормальный полный запрос GET
    //

    http::file_body::value_type file;

    if (sys::error_code ec; file.open(target.c_str(), beast::file_mode::read, ec), ec) {
        return PlainStringResponse(std::move(req), http::status::not_found, FileError::FILE_NOT_FOUND);
    }

    response.body() = std::move(file);
    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    response.prepare_payload();

    return response;
}


}   // namespace http_handler
