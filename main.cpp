/*
 MIT License
 Copyright (c) 2020 Ahmet Bilgili

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include <tuple>
#include <iostream>

template<const char *text, typename T>
struct KeyTypePair
{
  static constexpr const char *key = text;
  using Type = T;
};

template<const char *text, typename Tuple>
struct MapKeyIndexHelper;

template<const char *text>
struct MapKeyIndexHelper<text, std::tuple<>>
{ static constexpr std::size_t value = 0; };

template<const char *text, typename T, typename... Rest>
struct MapKeyIndexHelper<text, std::tuple<KeyTypePair<text, T>, Rest...>>
{
  static constexpr size_t value = 0;
  using RestTuple = std::tuple<Rest...>;
  static_assert(MapKeyIndexHelper<text, RestTuple>::value == std::tuple_size<RestTuple>::value,
                "type appears more than once in tuple");
};

template<const char *text, typename First, typename... Rest>
struct MapKeyIndexHelper<text, std::tuple<First, Rest...>>
{
  using RestTuple = std::tuple<Rest...>;
  static constexpr size_t value = 1 + MapKeyIndexHelper<text, RestTuple>::value;
};

template<const char *text, typename Tuple>
struct MapKeyIndex
{
  static constexpr size_t value = MapKeyIndexHelper<text, Tuple>::value;
  static_assert(value < std::tuple_size<Tuple>::value, "type does not appear in tuple");
};

template<typename...>
struct IsOneOf;

template<typename F>
struct IsOneOf<F>
{ static constexpr bool value = false; };

template<typename F, typename S, typename... T>
struct IsOneOf<F, S, T...>
{ static constexpr bool value = std::is_same<F, S>::value || IsOneOf<F, T...>::value; };

template<typename...>
struct IsUnique;

template<>
struct IsUnique<>
{ static constexpr bool value = true; };

template<typename F, typename... T>
struct IsUnique<F, T...>
{ static constexpr bool value = IsUnique<T...>::value && !IsOneOf<F, T...>::value; };

size_t constexpr getConstHash(char const *input) {
  return *input ? static_cast<size_t>(*input) + 33 * getConstHash(input + 1) : 5381;
}

template<typename Tuple>
struct CheckUniqueness;

template<const char *...text, typename... type>
struct CheckUniqueness<std::tuple<KeyTypePair<text, type>...>>
{
  template<size_t Size>
  struct SizeHelper
  {};
  template<const char *>
  struct TextHelper
  {};
  static constexpr bool value = IsUnique<SizeHelper<getConstHash(text)>...>::value
                                && IsUnique<TextHelper<text>...>::value;
};

/** Definition of the safe type map */
/** Keys */
constexpr char hello[] = "hellow";
constexpr char world[] = "world";
constexpr char anotherWorld[] = "world";
constexpr char is[] = "is";
constexpr char empty[] = "empty";
constexpr char nowhere[] = "nowhere";

/** keys to types */
using CompileTimeMap = std::tuple<KeyTypePair<hello, float>,
                                  KeyTypePair<world, std::true_type>,
                                  KeyTypePair<is, double>,
                                  KeyTypePair<empty, bool>>;

/** Keys to map */
using HelloParam = KeyTypePair<hello, CompileTimeMap>;
using WorldParam = KeyTypePair<world, CompileTimeMap>;
using IsParam = KeyTypePair<is, CompileTimeMap>;
using EmptyParam = KeyTypePair<empty, CompileTimeMap>;
using NoWhereParam = KeyTypePair<nowhere, CompileTimeMap>; // Is defined but cant be used

/** Static assert to check uniqueness of the key map, neither keys nor key type pairs can be repeated */
static_assert(CheckUniqueness<CompileTimeMap>::value, "is not unique");

/** world and anotherWorld variables are both "world" */
using NonUniqueTypeMap = std::tuple<KeyTypePair<world, float>,
                                    KeyTypePair<anotherWorld, std::true_type>>;
static_assert(!CheckUniqueness<NonUniqueTypeMap>::value, "is unique");

/** world is replicated with different types */
using OtherNonUniqueTypeMap = std::tuple<KeyTypePair<world, float>,
                                         KeyTypePair<world, std::true_type>>;
static_assert(!CheckUniqueness<OtherNonUniqueTypeMap>::value, "is unique");

template<typename T>
class GetType;

template<const char *text, typename Tuple>
class GetType<KeyTypePair<text, Tuple>>
{
private:
  static constexpr size_t index = MapKeyIndex<text, Tuple>::value;
  using TupleElement = typename std::tuple_element<index, Tuple>::type;

public:
  using Type = typename TupleElement::Type;
  static constexpr const char *key = text;
};

/** Example functions with compile type safe get/set methods */
template<typename T>
typename GetType<T>::Type get()
{
  std::cout << GetType<T>::key << std::endl;
  return typename GetType<T>::Type{};
};

template<typename T, typename ValueType>
std::enable_if_t<std::is_same<std::decay_t<ValueType>, typename GetType<T>::Type>::value> set(ValueType)
{
  std::cout << GetType<T>::key << std::endl;
};

int main()
{
  auto ret = get<WorldParam>();
  static_assert (std::is_same<decltype(ret), std::true_type>::value, "Return types dont match");
  set<IsParam>(5.0);
  // set<NoWhereParam>(5.0); does not compile because NoWhereParam does not exist in CompileTimeMap
  // set<IsParam>(true); does not compile because IsParam type is double in CompileTimeMap
}
