//
// Created by Harry Chen on 2019/6/4.
//

#ifndef HERMES_UTILS_H
#define HERMES_UTILS_H

#include <type_traits>

// token concatenation
#define STR(S) #S
#define _CAT(A, B) A##B
#define CAT(A, B) _CAT(A, B)
#define CHECKER_PREFIX __has_member_function_

// non-generic member functions
#define HAS_MEMBER_FUNC_CHECKER_NAME(FUNC) CAT(CHECKER_PREFIX, FUNC)

#define HAS_MEMBER_FUNC_CHECKER(FUNC) template <typename T> \
struct HAS_MEMBER_FUNC_CHECKER_NAME(FUNC) \
{ \
    template <typename C> static std::true_type test(decltype(&C::FUNC)); \
    template <typename C> static std::false_type test(...); \
    static constexpr bool value = std::is_same<decltype(test<T>(nullptr)), std::true_type>::value; \
};

#define HAS_MEMBER_FUNC(TYPE, FUNC) (HAS_MEMBER_FUNC_CHECKER_NAME(FUNC)<TYPE>::value)
#define NO_MEMBER_FUNC_ERROR_MESSAGE(TYPE, FUNC) "Type " STR(TYPE) " does not implement function " STR(FUNC)
#define ASSERT_HAS_MEMBER_FUNC(TYPE, FUNC) static_assert(HAS_MEMBER_FUNC(TYPE, FUNC), \
    NO_MEMBER_FUNC_ERROR_MESSAGE(TYPE, FUNC));

// generic member function with one type
#define HAS_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC) CAT(HAS_MEMBER_FUNC_CHECKER_NAME(FUNC), __generic)

#define HAS_GENERIC_MEMBER_FUNC_CHECKER(FUNC) template <typename T, typename R> \
struct HAS_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC) \
{ \
    template <typename C> static std::true_type test(decltype(&C::template FUNC<R>)); \
    template <typename C> static std::false_type test(...); \
    static constexpr bool value = std::is_same<decltype(test<T>(nullptr)), std::true_type>::value; \
};

#define HAS_GENERIC_MEMBER_FUNC(TYPE, FUNC, ARG) (HAS_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC)<TYPE, ARG>::value)
#define ASSERT_HAS_GENERIC_MEMBER_FUNC(TYPE, FUNC, ARG) static_assert(HAS_GENERIC_MEMBER_FUNC(TYPE, FUNC, ARG), \
    NO_MEMBER_FUNC_ERROR_MESSAGE(TYPE, FUNC) "<" STR(ARG) ">");


// generic member-functions with several possible types
#define HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC) CAT(HAS_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC), __multiple)

#define HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECK(FUNC) \
HAS_GENERIC_MEMBER_FUNC_CHECKER(FUNC); \
template <typename T, typename T1, typename ... Args> struct HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC); \
template <typename T, typename T1> struct HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC) <T, T1> \
{ \
    static constexpr bool value = HAS_GENERIC_MEMBER_FUNC(T, FUNC, T1); \
}; \
template <typename T, typename T1, typename ... Args> struct HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC) \
{ \
    static constexpr bool value = std::disjunction_v<HAS_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC)<T, T1>, \
        HAS_MULTIPLE_GENERIC_MEMBER_FUNC_CHECKER_NAME(FUNC)<T, Args...>>; \
};


#endif //HERMES_UTILS_H
