#ifndef CPPREST_JSON_CONVERTER_HPP
#define CPPREST_JSON_CONVERTER_HPP

#include <cpprest/json.h>
#include <boost/optional.hpp>

namespace web {
    namespace json {
        namespace utility {
            template<typename T, typename = void>
            class value_converter {
            public:
                static inline web::json::value to_json(const T& value);

                static inline void from_json(const web::json::value& json, T& value);
            };

            template<typename T>
            class value_converter<T, std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value || std::is_same<std::decay_t<T>, std::string>::value>> {
                template<typename I, typename std::enable_if_t<std::is_same<I, int32_t>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = json.as_number().to_int32();
                }
                template<typename I, typename std::enable_if_t<std::is_same<I, uint32_t>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = json.as_number().to_uint32();
                }
                template<typename I, typename std::enable_if_t<std::is_same<I, int64_t>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = json.as_number().to_int64();
                }
                template<typename I, typename std::enable_if_t<std::is_same<I, uint64_t>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = json.as_number().to_uint64();
                }
                template<typename I, typename std::enable_if_t<std::is_floating_point<I>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = static_cast<I>(json.as_double());
                }
                template<typename I, typename std::enable_if_t<std::is_same<I, bool>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = json.as_bool();
                }
                template<typename I, typename std::enable_if_t<std::is_same<I, std::string>::value, int> = 0>
                static inline void parse(const web::json::value& json, I& value) {
                    value = json.as_string();
                }
            public:
                static inline web::json::value to_json(const T& value) {
                    return web::json::value(value);
                }

                static inline void from_json(const web::json::value& json, T& value) {
                    parse(json, value);
                }
            };

            template<>
            class value_converter<const char*> {
            public:
                static inline web::json::value to_json(const char*& value) {
                    return web::json::value(value);
                }
            };

            template<typename T>
            class value_converter<std::vector<T>> {
            public:
                static inline web::json::value to_json(const std::vector<T>& value) {
                    auto arr = web::json::value::array(value.size());
                    for (size_t i=0;i<value.size();i++) {
                        arr[i] = value_converter<T>::to_json(value.at(i));
                    }
                    return arr;
                }

                static inline void from_json(const web::json::value& json, std::vector<T>& value) {
                    value.reserve(json.size());
                    value.resize(0);
                    alignas(alignof(T)) unsigned char buf[sizeof(T)] ;
                    for (const auto& it : json.as_array()) {
                        T* ptr = new(buf) T;
                        value_converter<T>::from_json(it, *ptr);
                        value.push_back(std::move(*ptr));
                    }
                }
            };

            template<typename T, size_t N>
            class value_converter<std::array<T, N>> {
            public:
                static inline web::json::value to_json(const std::array<T, N>& value) {
                    auto arr = web::json::value::array(N);
                    for (size_t i=0;i<N;i++) {
                        arr[i] = value_converter<T>::to_json(value.at(i));
                    }
                    return arr;
                }

                static inline void from_json(const web::json::value& json, std::array<T, N>& value) {
                    value.reserve(N);
                    value.resize(0);
                    alignas(alignof(T)) unsigned char buf[sizeof(T)] ;
                    for (const auto& it : json.as_array()) {
                        T* ptr = new(buf) T;
                        value_converter<T>::from_json(it, *ptr);
                        value.push_back(std::move(*ptr));
                    }
                }
            };

            template<typename T>
            class value_converter<boost::optional<T>> {
            public:
                static inline web::json::value to_json(const boost::optional<T>& value) {
                    if (value) {
                        return value_converter<std::decay_t<T>>::to_json(*value);
                    }
                    return web::json::value::null();
                }

                static inline void from_json(const web::json::value& json, boost::optional<T>& value) {
                    if (!json.is_null()) {
                        alignas(alignof(T)) unsigned char buf[sizeof(T)];
                        auto* ptr = new (buf) T;
                        value_converter<std::decay_t<T>>::from_json(json, *ptr);
                        value = std::move(*ptr);
                    }
                    else {
                        value = boost::none;
                    }
                }
            };

            template<typename T>
            class value_converter<T, std::enable_if_t<std::is_pointer<T>::value>> {
                using V = typename std::pointer_traits<T>::element_type;

                static inline void assign(V& value, V*& pointer) {
                    pointer = new V();
                    *pointer = std::move(value);
                }
                static inline void assign(V& value, std::unique_ptr<V>& pointer) {
                    pointer = std::make_unique<V>(std::move(value));
                }
                static inline void assign(V& value, std::shared_ptr<V>& pointer) {
                    pointer = std::make_shared<V>(std::move(value));
                }
            public:
                static inline web::json::value to_json(const T& value) {
                    if (value) {
                        return value_converter<std::decay_t<V>>(*value);
                    }
                    else {
                        return web::json::value::null();
                    }
                }
                static inline void from_json(const web::json::value& json, T& value) {
                    if (!json.is_null()) {
                        alignas(alignof(T)) unsigned char buf[sizeof(T)];
                        auto* ptr = new (buf) T;
                        value_converter<std::decay_t<T>>::from_json(json, *ptr);
                        assign(*ptr, value);
                    }
                    else {
                        value = nullptr;
                    }
                }
            };

            template<typename T>
            inline web::json::value convert(T&& val) {
                return value_converter<std::decay_t<T>>::to_json(std::forward<T>(val));
            }

            template<typename T>
            inline void convert(const web::json::value& json, T& value) {
                value_converter<std::decay_t<T>>::from_json(json, value);
            }

            template<typename T>
            class field_info {
                template<typename U, typename = void>
                struct getter_setter {};

                template<typename C, typename R>
                struct getter_setter<R C::*> {
                    using TClass = C;
                    using TField = std::decay_t<R>;

                    static constexpr bool field = true;
                    static constexpr bool readable = true;
                    static constexpr bool writeable = true;

                    static inline TField get(const TClass& o, TField TClass::*m) {
                        return o.*m;
                    }

                    static inline void set(TClass& o, TField TClass::*m, TField value) {
                        o.*m = std::move(value);
                    }
                };

                template<typename C, typename R>
                struct getter_setter<R& (C::*)()> {
                    using TClass = C;
                    using TField = std::decay_t<R>;

                    static constexpr bool field = false;
                    static constexpr bool readable = true;
                    static constexpr bool writeable = true;

                    static inline TField get(const TClass& o, R& (TClass::*m)()) {
                        return (const_cast<TClass&>(o).*m)();
                    }

                    static inline void set(TClass& o, R& (TClass::*m)(), TField value) {
                        (o.*m)() = std::move(value);
                    }
                };

                template<typename C, typename R>
                struct getter_setter<R (C::*)()> {
                    using TClass = C;
                    using TField = std::decay_t<R>;

                    static constexpr bool field = false;
                    static constexpr bool readable = true;
                    static constexpr bool writeable = false;

                    static inline TField get(const TClass& o, R (TClass::*m)()) {
                        return (const_cast<TClass&>(o).*m)();
                    }

                    static inline void set(TClass& o, R (TClass::*m)(), TField value) {

                    }
                };

                template<typename C, typename R>
                struct getter_setter<R (C::*)() const> {
                    using TClass = C;
                    using TField = std::decay_t<R>;

                    static constexpr bool field = false;
                    static constexpr bool readable = true;
                    static constexpr bool writeable = false;

                    static inline TField get(const TClass& o, R (TClass::*m)() const) {
                        return (o.*m)();
                    }

                    static inline void set(TClass& o, R (TClass::*m)() const, TField value) {

                    }
                };

                template<typename C, typename R, typename V, typename N>
                struct getter_setter<std::pair<R (C::*)() const, N(C::*)(V)>> {
                    using TClass = C;
                    using TField = std::decay_t<R>;

                    static constexpr bool field = false;
                    static constexpr bool readable = true;
                    static constexpr bool writeable = true;

                    static inline TField get(const TClass& o, const std::pair<R (C::*)() const, N(C::*)(V)>& readwrite) {
                        auto& getter = std::get<0>(readwrite);
                        return (o.*getter)();
                    }

                    static inline void set(const TClass& o, const std::pair<R (C::*)() const, N(C::*)(V)>& readwrite, TField value) {
                        auto& setter = std::get<1>(readwrite);
                        (const_cast<TClass&>(o).*setter)(std::move(value));
                    }
                };
            public:
                using TClass = typename getter_setter<T>::TClass;
                using TField = typename getter_setter<T>::TField;
                static constexpr bool readonly = not getter_setter<T>::writeable;

                std::string name;
                T accessor {};

                inline TField get(const TClass& object) const {
                    return getter_setter<T>::get(object, accessor);
                }

                inline void set(TClass& object, TField value) const {
                    getter_setter<T>::set(object, accessor, std::move(value));
                }
            };

            template<typename T>
            inline auto make_field(std::string name, T field) {
                field_info<T> info;
                info.name = std::move(name);
                info.accessor = field;
                return info;
                static_assert(std::is_member_pointer<T>::value, "not a member pointer");
            }

            template<typename Getter, typename Setter>
            inline auto make_field(std::string name, Getter getter, Setter setter) {
                auto accessor = std::make_pair(std::move(getter), std::move(setter));
                field_info<decltype(accessor)> info;
                info.name = std::move(name);
                info.accessor = std::move(accessor);
                return info;

                static_assert(std::is_member_function_pointer<Getter>::value && std::is_member_function_pointer<Setter>::value, "not a member function pointer");
            }

            template<typename... Ts>
            class mapper {
            public:
                using O = typename field_info<std::tuple_element_t<0, std::tuple<Ts...>>>::TClass;

                std::tuple<field_info<Ts>...> fields;

                mapper() = default;

                explicit mapper(std::tuple<field_info<Ts>...> fields) : fields(std::move(fields)) {}

                web::json::value to_json(const O& value) const {
                    auto object = web::json::value::object();
                    serialize_to(value, object, std::make_index_sequence<sizeof...(Ts)>{});
                    return object;
                }

                void from_json(const web::json::value& object, O& value) {
                    deserialize_from(object, value, std::make_index_sequence<sizeof...(Ts)>{});
                }

            private:
                template<size_t... I>
                inline void serialize_to(const O& value, web::json::value& object, std::index_sequence<I...>) const {
                    volatile bool unused[sizeof...(I)] = {serialize_field_to(value, object, std::get<I>(fields))...};
                    (void)unused;
                }

                template<typename T>
                static inline bool serialize_field_to(const O& value, web::json::value& object, const field_info<T>& info) {
                    object[info.name] = web::json::utility::convert(info.get(value));
                    return true;
                }

                template<size_t...I>
                inline void deserialize_from(const web::json::value& object, O& value, std::index_sequence<I...>) {
                    volatile bool unused[sizeof...(I)] = {deserialize_field_from(object, value, std::get<I>(fields))...};
                    (void)unused;
                }

                template<typename T, std::enable_if_t<!field_info<T>::readonly, int> = 0>
                static inline bool deserialize_field_from(const web::json::value& object, O& value, const field_info<T>& info) {
                    static auto Null = web::json::value::null();
                    using Value = typename field_info<T>::TField;
                    alignas(alignof(Value)) unsigned char buf[sizeof(Value)];
                    auto ptr = new (buf) Value;
                    web::json::utility::convert(object.has_field(info.name) ? object.at(info.name) : Null, *ptr);
                    info.set(value, std::move(*ptr));
                    return true;
                }

                template<typename T, std::enable_if_t<field_info<T>::readonly, int> = 0>
                static inline bool deserialize_field_from(const web::json::value& object, O& value, const field_info<T>& info) {
                    return false;
                }
            };

            template<typename...Ts>
            inline auto make_mapper(field_info<Ts>... props) {
                return mapper<Ts...> {std::make_tuple<field_info<Ts>...>(std::move(props)...)};
            }
        }
    }
}

#define DECLARE_CPPREST_JSON_ENUM_CONVERTER(EnumType, ...)               \
            namespace web{ namespace json { namespace  utility {         \
            template<>                                                   \
            class value_converter<EnumType> {                            \
            static_assert(std::is_enum<EnumType>::value, "not an enum"); \
            public:                                                      \
                static inline web::json::value to_json(const EnumType& value) { \
                    const static std::unordered_map<EnumType, std::string> values = __VA_ARGS__; \
                    return web::json::value(values.at(value));                             \
                }                                                        \
                static inline void from_json(const web::json::value& json, EnumType& value){ \
                    const static std::unordered_map<EnumType, std::string> values = __VA_ARGS__; \
                    const std::string& val = json.as_string();           \
                    for (auto& it : values) {                            \
                        if (it.second == val) {                          \
                            value = it.first;                            \
                            return;                                      \
                        }                                                \
                    }                                                    \
                }                                                        \
            };                                                           \
            }}}

#define DECLARE_CPPREST_JSON_TYPE_CONVERTER(Type, ...)

#endif //CPPREST_JSON_CONVERTER_HPP
