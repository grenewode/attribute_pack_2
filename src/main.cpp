#include <iostream>
#include <type_traits>

template <typename U, typename T>
decltype(auto) forward_cast(std::remove_reference_t<T>& value) {
    // TODO: Handle volatile
    if constexpr (std::is_lvalue_reference_v<T>) {
        if constexpr (std::is_const_v<T>) {
            return static_cast<const U&>(value);
        } else {
            return static_cast<U&>(value);
        }
    } else {
        if constexpr (std::is_const_v<T>) {
            return static_cast<const U&&>(value);
        } else {
            return static_cast<U&&>(value);
        }
    }
}

#define MAKE_ATTRIBUTE(NAME)                                                  \
    struct NAME##_t {                                                         \
        template <typename T, T D = {}>                                       \
        struct Value {                                                        \
            using attribute_t = NAME##_t;                                     \
            T value;                                                          \
                                                                              \
            static Value& Get(Value& object) { return object; }               \
            static Value&& Get(Value&& object) { return std::move(object); }  \
            static const Value& Get(const Value& object) { return object; }   \
            static const Value&& Get(const Value&& object) {                  \
                return std::move(object);                                     \
            }                                                                 \
            template <typename HEAD, typename HEAD2, typename... TAIL>        \
            static decltype(auto) Get(HEAD&& head, HEAD2&& head2,             \
                                      TAIL&&... tail) {                       \
                if constexpr (std::is_base_of_v<Value, std::decay_t<HEAD>>) { \
                    return Get(std::forward<HEAD>(head));                     \
                } else {                                                      \
                    return Get(std::forward<HEAD2>(head2),                    \
                               std::forward<TAIL>(tail)...);                  \
                }                                                             \
            }                                                                 \
            static Value Get(...) { return {D}; }                             \
                                                                              \
            std::ostream& print(std::ostream& out) const {                    \
                return out << #NAME " = " << value;                           \
            }                                                                 \
        };                                                                    \
        template <typename T>                                                 \
        Value<T> operator=(T&& value) const {                                 \
            return {std::forward<T>(value)};                                  \
        }                                                                     \
    } NAME;                                                                   \
    std::ostream& operator<<(std::ostream& out, const NAME##_t&) {            \
        return out << #NAME;                                                  \
    }

template <typename... ATTRIBUTE_VALUES>
struct Attributes : public ATTRIBUTE_VALUES... {
    template <typename... ARGS>
    Attributes(ARGS&&... args)
      : ATTRIBUTE_VALUES(
          ATTRIBUTE_VALUES::Get(std::forward<ARGS>(args)...))... {}
};

template <typename... ATTRIBUTE_VALUES>
Attributes(ATTRIBUTE_VALUES&&... values)
  ->Attributes<std::decay_t<ATTRIBUTE_VALUES>...>;

template <typename... ATTRIBUTE_VALUES>
auto make_pack(ATTRIBUTE_VALUES&&... values) {
    return Attributes<ATTRIBUTE_VALUES...>(
      std::forward<ATTRIBUTE_VALUES>(values)...);
}

template <typename... ATTRIBUTE_VALUES>
std::ostream& operator<<(std::ostream& out,
                         const Attributes<ATTRIBUTE_VALUES...>& attributes) {
    out << "{ ";
    ((ATTRIBUTE_VALUES::Get(attributes).print(out) << ", "), ...);
    return out << " }";
}

MAKE_ATTRIBUTE(Foo);
MAKE_ATTRIBUTE(Bar);

int main(int, char**) {
    Attributes data{Foo = 1, Bar = 2};

    Attributes<Foo_t::Value<int>, Bar_t::Value<int>> data2{Bar = 20, Foo = 20};
    Attributes<Foo_t::Value<int, 100>, Bar_t::Value<int>> data3{Bar = 20};

    std::cout << data << std::endl;
    std::cout << data2 << std::endl;
    std::cout << data3 << std::endl;
}
