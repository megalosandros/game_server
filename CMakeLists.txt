cmake_minimum_required(VERSION 3.16)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -save-temps")

# boost.beast будет использовать std::string_view вместо boost::string_view
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

add_library(game_model STATIC
	src/sdk.h
	src/game/model.h
	src/game/model.cpp
	src/game/tagged.h
	src/game/loot_generator.h
	src/game/loot_generator.cpp
	src/game/player_tokens.h
	src/game/player_tokens.cpp
	src/game/game_session.h
	src/game/game_session.cpp
	src/game/player.h
	src/game/player.cpp
	src/game/app.h
	src/game/app.cpp
	src/game/model_units.h
	src/game/math_utils.h
	src/game/math_utils.cpp
	src/game/ticker.h
	src/game/ticker.cpp	
	src/game/geom.h
	src/game/collision_detector.h
	src/game/collision_detector.cpp
	src/game/model_serialization.h
	src/game/model_serialization.cpp
	src/game/postgres.h
	src/game/postgres.cpp
	src/game/dogs_collector.h
	src/game/dogs_collector.cpp
)
target_compile_options(game_model PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(game_model PUBLIC CONAN_PKG::boost)
target_link_libraries(game_model PUBLIC CONAN_PKG::boost)
target_link_libraries(game_model PUBLIC Threads::Threads)
target_link_libraries(game_model PUBLIC CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/sdk.h
	src/server/main.cpp
	src/server/http_server.cpp
	src/server/http_server.h
	src/server/json_tags.h
	src/server/boost_json.cpp
	src/server/json_loader.h
	src/server/json_loader.cpp
	src/server/json_serializer.h
	src/server/json_serializer.cpp
	src/server/request_handler.h
	src/server/request_handler.cpp
	src/server/api_handler.h
	src/server/api_handler.cpp
	src/server/file_handler.h
	src/server/file_handler.cpp
	src/server/url.h
	src/server/url.cpp
	src/server/ci_string.h
	src/server/http_response.h
	src/server/http_response.cpp
	src/server/logger.h
	src/server/logger.cpp
	src/server/command_line.h
	src/server/command_line.cpp
)
target_compile_options(game_server PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(game_server game_model)

add_executable(game_tests
    tests/loot_generator_tests.cpp
	tests/collision_detector_tests.cpp
	tests/state_serialization_tests.cpp
)
target_link_libraries(game_tests CONAN_PKG::catch2 game_model)

