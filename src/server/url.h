#pragma once
#include <string>
#include <string_view>

//
//  Классы и функции для работы с URL строкой
//

namespace url {

//
//  Декодирует URL строку (убирает '%' и '+')
//  Функция толерантна к случаям '%N' - т.е. один символ вместо двух
//  в этом случае в результирующей строке так и останется '%N'
//  Некоторые парсеры кидают исключения, но некоторые не заменяют такие последовательности
//  Кирилличные utf-8 символы нормально обрабатывает ("%D0%AFndex")
//
std::string Decode(std::string_view encoded);

} // url
