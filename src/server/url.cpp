#include "../sdk.h"
#include <cctype>
#include "url.h"


//
//  Классы и функции для работы с URL строкой
//

namespace url {
namespace details {

using InputIterator = std::string_view::const_iterator;
using OutputIterator = std::back_insert_iterator<std::string>;


char letter_to_hex(char in) {

  if ((in >= '0') && (in <= '9')) {
    return in - '0';
  }

  if ((in >= 'a') && (in <= 'f')) {
    return in + 10 - 'a';
  }

  if ((in >= 'A') && (in <= 'F')) {
    return in + 10 - 'A';
  }

  return in;
}

char nibbles_to_char(char high, char low)
{
    //
    //  ну или так кому наглядней: ((0x10 * high) + low)
    //
    return static_cast<char>((high << 4) | low);
}

InputIterator copy_char(InputIterator in, OutputIterator out)
{
    *out++ = (*in == '+') ? ' ' : *in;
    return in + 1;
}

InputIterator decode_char(InputIterator inBegin, InputIterator inEnd, OutputIterator outBegin)
{
    //
    //  сразу двигаюсь на следующий символ, внутри функции я точно знаю,
    //  что первый символ - это всегда процент
    //
    auto it = inBegin + 1;

    //
    //  проверка, что я не уперся в конец строки
    //  и что за символом '%' идут две hex цифры
    //
    if ((std::distance(it, inEnd) < 2) || (!std::isxdigit(it[0]) || !std::isxdigit(it[1])))
    {
        //
        //  либо уже конец строки близко, либо после процента какой-то мусор
        //  в этом случае просто копирую в выходной поток '%'
        //  продвигаюсь на один символ и выхожу из функции
        //  Я мог бы эти случаи разобрать отдельно и если конец строки - то 
        //  скопировать остаток строки и вернуть inEnd - но выглядит некрасиво, слишком
        //  много разных кейсов, хотя по факту это один кейс
        //
        return *outBegin++ = *inBegin, it;
    }

    //
    //  Здесь я уже продвинулся на следующий символ после процента
    //  и знаю, что следующие два символа - HEX цифры
    //

    auto v0 = letter_to_hex(*it++); // (и продвинуть итератор)
    auto v1 = letter_to_hex(*it++); // (и продвинуть итератор)
  
    *outBegin++ = nibbles_to_char(v0, v1);

    return it;

}

void decode(InputIterator inBegin, InputIterator inEnd, OutputIterator outBegin)
{
    auto it = inBegin;
    auto out = outBegin;

    while (it != inEnd)
    {
        if (*it == '%') {
            //
            //  это может быть либо валидная эскейп последовательность,
            //  которую нужно декодировать, либо какой-то случайный символ процента -
            //  его я просто копирую и двигаюсь дальше по входному потоку
            //
            it = decode_char(it, inEnd, out);
        }
        else {
            //
            //  это может быть либо символ, который нужно просто скопировать,
            //  либо '+', который нужно заменить на пробел
            //
            it = copy_char(it, out);
        }
    }
}

} // namespace details

//
//  Декодирует URL строку (убирает '%' и '+')
//  Функция толерантна к случаям '%N' - т.е. один символ вместо двух
//  в этом случае в результирующей строке так и останется '%N'
//  Некоторые парсеры кидают исключения, но некоторые не заменяют такие последовательности
//  Кирилличные utf-8 символы нормально обрабатывает ("%D0%AFndex")
//
std::string Decode(std::string_view source) {
    //
    //  зарезервирую место в строке - чтобы не было лишних реаллокаций
    //  места зарезервируеся немного больше чем нужно, потому что
    //  каждые три кодированных символа декодируются в один
    //
    std::string unencoded;

    unencoded.reserve(source.size());
    details::decode(source.cbegin(), source.cend(), std::back_inserter(unencoded));

    return unencoded;
}


} // namespace url
