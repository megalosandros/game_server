#include "../sdk.h"
#include <boost/json.hpp>
#include <boost/json/string_view.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "json_loader.h"
#include "json_tags.h"

namespace json_loader {

using namespace std::literals;
namespace json = boost::json;


std::string LoadFileContent(const std::filesystem::path &file_path) {
    
    //
    //  Сначала проверю, передали ли мне какое-то вменяемое имя конфиг файла
    //
    if  ( !file_path.has_filename()
        || file_path.filename() == "."s
        || file_path.filename() == ".."s) {
            throw std::invalid_argument("The path "s + file_path.c_str() + " does not seem to be a good file name."s);
        }

    std::ifstream inputFile(file_path);

    //
    //  Можно заставить генерировать std::ifstream исключения, но это 
    //  очень абстрактные исключения, по ним мало что поймешь, поэтому
    //  просто вручную проверю результат открытия файла
    //
    if (!inputFile) {
        throw std::invalid_argument("Could not open confg file. Check file existence and permissions.");
    }

    //
    //  Сначала посмотрю размер файла и зарезервирую достаточно места
    //  в строке - но если вдруг файл пустой, то с ним не работаю
    //

    inputFile.seekg(0, std::ios::end);
    auto fileSize = inputFile.tellg();
    if (fileSize <= 0) {
        throw std::invalid_argument("JSON config file does not contain any data");
    }
    inputFile.seekg(0, std::ios::beg);

    std::string outputString;
    outputString.reserve(fileSize);
    outputString.assign(
        std::istreambuf_iterator<char>(inputFile),
        std::istreambuf_iterator<char>());

    //
    //  Если что-то пошло не так - пустую строчку возвращать не буду, не нужна она.
    //  Теоретически (и даже практически) размер строки равен размеру файла
    //  можно было бы и сравнить, но такое дело боязно как-то.
    //
    if (outputString.empty()) {
        throw std::invalid_argument("Could not read any data from JSON config file");
    }
    
    return outputString;
}

model::Road LoadRoad(const json::value& jsonValue)
{
    //
    //  x0 и y0 есть (должны быть) всегда, а вот
    //  x1 и y1 в зависимости от направления дороги
    //
    auto const &roadX0 = jsonValue.at(JsonTag::X0);
    auto const &roadY0 = jsonValue.at(JsonTag::Y0);

    const model::Point roadStart = {
        json::value_to<model::Coord>(roadX0),
        json::value_to<model::Coord>(roadY0)
        };

    //
    //  если есть x1 - создаю горизонтальную дорогу
    //
    if (auto const* roadX1 = jsonValue.as_object().if_contains(JsonTag::X1)) {
        return model::Road(model::Road::HORIZONTAL, roadStart, json::value_to<model::Coord>(*roadX1));
    }

    //
    //  с горизонтальной дорогой ничего не вышло - пробую вертикальную
    //  это значит должен быть задан y1
    //
    if (auto const* roadY1 = jsonValue.as_object().if_contains(JsonTag::Y1)) {
        return model::Road(model::Road::VERTICAL, roadStart, json::value_to<model::Coord>(*roadY1));
    }

    //
    //  если я попал сюда - значит конфиг файл испорчен либо не распарсился
    //
    throw std::invalid_argument("Config file does not conatan valid road list");
}

model::Building LoadBuilding(const json::value& jsonValue)
{
    //
    //  Здание на карте представлено в виде объекта 
    //  со свойствами x, y, w, h, которые задают целочисленные
    //  координаты верхнего левого угла дома, его ширину и высоту.
    //
    auto const &x = jsonValue.at(JsonTag::X);
    auto const &y = jsonValue.at(JsonTag::Y);
    auto const &width = jsonValue.at(JsonTag::W);
    auto const &height = jsonValue.at(JsonTag::H);

    model::Rectangle buildingRect = {
        { json::value_to<model::Coord>(x), json::value_to<model::Coord>(y) },
        { json::value_to<model::Dimension>(width), json::value_to<model::Dimension>(height) }
    };

    return model::Building(buildingRect);
}

model::Office LoadOffice(const json::value& jsonValue)
{
    //
    //  Офис представлен объектом со свойствами:
    //  id — уникальный идентификатор офиса. Строка.
    //  x, y — координаты офиса. Целые числа. 
    //  offsetX, offsetY — смещение изображения офиса относительно координат x и y.  
    //
    auto const &officeId = jsonValue.at(JsonTag::ID);
    auto const &x = jsonValue.at(JsonTag::X);
    auto const &y = jsonValue.at(JsonTag::Y);
    auto const &offsetX = jsonValue.at(JsonTag::OFFSETX);
    auto const &offsetY = jsonValue.at(JsonTag::OFFSETY);

    model::Point position = {
        json::value_to<model::Coord>(x),
        json::value_to<model::Coord>(y)
    };
    model::Offset offset = {
        json::value_to<model::Dimension>(offsetX),
        json::value_to<model::Dimension>(offsetY)
    };

    model::Office::Id parsedId(json::value_to<std::string>(officeId));

    return model::Office(parsedId, position, offset);
}

model::Loot::Value LoadLoot(const json::value& jsonValue) {

     const auto& value = jsonValue.at(JsonTag::VALUE);

     return json::value_to<model::Loot::Value>(value);

}

model::Map LoadMap(const json::value& jsonValue, model::Real defaultDogSpeed, size_t defaultBagCapacity)
{
    //
    //  Каждый элемент этого массива — объект, описывающий дороги,
    //  здания и офисы на игровой карте. Свойства этого объекта:
    //  id — уникальный идентификатор карты. Тип: строка.
    //  name — название карты, которое выводится пользователю. Тип: строка.
    //  dogSpeed - опциональное поле задает скорость персонажей на конкретной карте. Тип: double. 
    //  bagCapacity - опциональное поле задает вместимость рюкзака на конкретной карте. Тип: uint64.
    //  roads — дороги игровой карты. Тип: массив объектов. Массив должен содержать хотя бы один элемент.
    //  buildings — здания. Тип: массив объектов. Массив может быть пустым.
    //  offices — офисы бюро находок. Тип: массив объектов. Массив может быть пустым.
    //
    auto const &mapId = jsonValue.at(JsonTag::ID);
    auto const &mapName = jsonValue.at(JsonTag::NAME);

    //
    //  Скорость персонажей на конкретной карте задаёт опциональное поле
    //  dogSpeed в соответствующем объекте карты.
    //  Если это поле отсутствует, на карте используется скорость по умолчанию
    //
    model::Real dogSpeed = defaultDogSpeed;
    if (auto const* jsonDogSpeed = jsonValue.as_object().if_contains(JsonTag::DOG_SPEED)) {
        dogSpeed = jsonDogSpeed->as_double();
    }

    //
    //  Вместимость рюкзаков на конкретной карте задаёт опциональное поле
    //  bagCapacity в соответствующем объекте карты.
    //  Если это поле отсутствует, на карте используется вместимость по умолчанию
    //
    size_t bagCapacity = defaultBagCapacity;
    if (auto const* jsonBagCapacity = jsonValue.as_object().if_contains(JsonTag::BAG_CAPACITY)) {
        bagCapacity = jsonBagCapacity->as_uint64();
    }


    auto const &mapRoads = jsonValue.at(JsonTag::ROADS).as_array();
    auto const &mapBuildings = jsonValue.at(JsonTag::BUILDINGS).as_array();
    auto const &mapOffices = jsonValue.at(JsonTag::OFFICES).as_array();
    auto const &mapLoots = jsonValue.at(JsonTag::LOOT_TYPES).as_array();

    //
    //  хотя бы одна дорога должна быть
    //
    if (mapRoads.empty()) {
        throw std::invalid_argument("Config map must contain at least one road");
    }

    //
    //  Создать объект Map с идентификатором и именем
    //
    model::Map::Id  parsedMapId(json::value_to<std::string>(mapId));
    std::string     parsedMapName(json::value_to<std::string>(mapName));
    model::Map      parsedMap(parsedMapId, parsedMapName, dogSpeed, bagCapacity);

    //
    //  Затем последовательно добавить в Map все дороги, здания, офисы
    //
    for (auto const &nextRoad : mapRoads) {
        parsedMap.AddRoad(LoadRoad(nextRoad));
    }

    for (auto const &nextBuilding : mapBuildings) {
        parsedMap.AddBuilding(LoadBuilding(nextBuilding));
    }

    for (auto const &nextOffice : mapOffices) {
        parsedMap.AddOffice(LoadOffice(nextOffice));
    }

    //
    //  Еще стоимости трофеев и представление для фронта
    //
    for (auto const& nextLoot : mapLoots) {
        parsedMap.AddLoot(LoadLoot(nextLoot));
    }
    
    parsedMap.AddFrontendData(json::serialize(mapLoots));

    return parsedMap;
}

//
// Загрузить модель игры из уже распарсенного json::value
//
model::Game LoadGame(const json::value& config_json_value) {
    //
    //  Файл содержит JSON-объект со свойствами игры:
    //  maps, defaultDogSpeed, lootGeneratorConfig
    //

    //
    //  maps описывает массив игровых карт или уровней.
    //  А если файл такого не содержит - будет исключение, заодно и 
    //  проверю файл на корректность
    //
    auto const &maps = config_json_value.at(JsonTag::MAPS);
    auto const &mapsArray = maps.as_array();
    if (mapsArray.empty()) {
        throw std::invalid_argument("Config file does not contain any maps");
    }

    //
    //  Скорость персонажей на всех картах задаёт опциональное поле
    //  defaultDogSpeed в корневом JSON-объекте.
    //  Если это поле отсутствует, скорость по умолчанию считается равной 1.
    //
    model::Real defaultDogSpeed = 1.0;
    if (auto const* jsonSpeed = config_json_value.as_object().if_contains(JsonTag::DEFAULT_DOG_SPEED)) {
        defaultDogSpeed = jsonSpeed->as_double();
    }

    //
    //  Время бездействия по умолчанию равно одной минуте.
    //  Чтобы переопределить это значение, нужно указать его 
    //  в конфигурационном JSON-файле в свойстве dogRetirementTime,
    //  которое задаёт время в секундах.
    //
    model::TimeInterval dogRetirementTime{1min};
    if (auto const* jsonRetirementTime = config_json_value.as_object().if_contains(JsonTag::DOG_RETIREMENT_TIME)) {
        dogRetirementTime = std::chrono::milliseconds{static_cast<int64_t>(jsonRetirementTime->as_double() * 1000.0)};
    }

    //
    //  Вместимость рюкзаков на всех картах задаёт опциональное поле
    //  defaultBagCapacity в корневом JSON-объекте.
    //  Если это поле отсутствует, вместимость по умолчанию считается равной 3.
    //
    size_t defaultBagCapacity = 3;
    if (auto const* jsonCapacity = config_json_value.as_object().if_contains(JsonTag::DEFAULT_BAG_CAPACITY)) {
        defaultBagCapacity = jsonCapacity->as_uint64();
    }

    //
    //  Параметры генератора потерянных вещей
    //
    auto const &lootGeneratorConfig = config_json_value.at(JsonTag::LOOT_GENERATOR_CONFIG);
    auto probability = lootGeneratorConfig.at(JsonTag::PROBABILITY).as_double();
    auto period = lootGeneratorConfig.at(JsonTag::PERIOD).as_double();
    auto periodMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>{period});

    //
    //  Собрать всё из конфиг файла и загрузить в игру
    //

    model::Game game{periodMilliseconds, probability, dogRetirementTime};

    for (auto const &nextMap : mapsArray) {
        game.AddMap(LoadMap(nextMap, defaultDogSpeed, defaultBagCapacity));
    }

    return game;
}

//
//  Загрузить модель игры из файла
//
model::Game::Ptr LoadGame(const std::filesystem::path& json_path) {
    //
    // Загрузить содержимое файла json_path, например, в виде строки
    // Из функции возвращается либо строка с какими-то данными, либо
    // летит исключение - какую-то непонятную пустую строку функция не возвращает
    //
    std::string jsonString = LoadFileContent(json_path);

    //
    // Распарсить строку как JSON, используя boost::json::parse
    //
    auto config = json::parse(jsonString);

    //
    // Загрузить модель игры из json::value
    //
    return std::make_shared<model::Game>(LoadGame(config));

}

//
//  Парсинг body для входа в игру
//
std::optional<app::JoinGameRequest> ParseJoinRequest(const std::string& body)
{
    try {
        auto name_and_id = json::parse(body);

        auto const &userName = name_and_id.at(JsonTag::USER_NAME);
        auto const &mapId = name_and_id.at(JsonTag::MAP_ID);

        return std::make_optional<app::JoinGameRequest>(
            json::value_to<std::string>(userName),
            model::Map::Id(json::value_to<std::string>(mapId)));
    }
    catch (...) {
        //
        //  в случае любых ошибок парсинга возвращаю nullopt
        //
        return std::nullopt;
    }
}

//
//  парсинг body для перемещения
//
std::optional<model::Dog::Direction> ParseActionRequest(const std::string& body) {
    try {
        auto move_dir = json::parse(body);

        auto const &dir = move_dir.at(JsonTag::MOVE);
        std::string stringDir = json::value_to<std::string>(dir);
        model::Dog::Direction dogDir = (stringDir.empty()) ? model::Dog::Direction::Stop : static_cast<model::Dog::Direction>(stringDir[0]);
        if (dogDir != model::Dog::Direction::Down &&
            dogDir != model::Dog::Direction::Left &&
            dogDir != model::Dog::Direction::Right &&
            dogDir != model::Dog::Direction::Up &&
            dogDir != model::Dog::Direction::Stop) {
                throw std::invalid_argument("Failed to parse action");
            }

        return dogDir;
    }
    catch (...) {
        //
        //  в случае любых ошибок парсинга возвращаю nullopt
        //
        return std::nullopt;
    }

}

//
//  Парсинг body для дельта time
//
std::optional<model::TimeInterval> ParseTickRequest(const std::string& body) {

    try {
        auto jsonBody = json::parse(body);

        auto const &time_delta = jsonBody.at(JsonTag::TIME_DELTA);

        if (auto dt = json::value_to<model::TimeInterval::rep>(time_delta); dt != 0) {
            return model::TimeInterval{dt};
        }

        return std::nullopt;
    }
    catch (...) {
        //
        //  в случае любых ошибок парсинга возвращаю nullopt
        //
        return std::nullopt;
    }

}


}  // namespace json_loader
