#include "../sdk.h"
#include "request_handler.h"
#include "api_handler.h"
#include "file_handler.h"


namespace http_handler {



VariantResponse RequestHandler::MakeResponse(StringRequest &&req) {
    
    if (ApiRequestHandler::IsApiRequest(req)) {
        return api_request_handler_.HandleRequest(std::move(req));
    }

    return file_request_handler_.HandleRequest(std::move(req));
}

}  // namespace http_handler
