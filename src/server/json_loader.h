#pragma once

#include <filesystem>
#include <string>

#include "../game/model.h"
#include "../game/app.h"

namespace json_loader {

//
//  Загрузка модели игры из конфигурационного JSON файла
//
model::Game::Ptr LoadGame(const std::filesystem::path &json_path);

//
//  Парсинг body для входа в игру
//
std::optional<app::JoinGameRequest> ParseJoinRequest(const std::string& body);

//
//  Парсинг body для перемещения
//
std::optional<model::Dog::Direction> ParseActionRequest(const std::string& body);

//
//  Парсинг body для дельта time
//
std::optional<model::TimeInterval> ParseTickRequest(const std::string& body);


}  // namespace json_loader
