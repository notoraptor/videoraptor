//
// Created by notoraptor on 23/07/2018.
//

#ifndef VIDEORAPTORBATCH_UNICODE_HPP
#define VIDEORAPTORBATCH_UNICODE_HPP

#include <cstddef>
#include <lib/utf/utf.hpp>

template <typename Character>
static size_t stringLength(const Character* s) {
	size_t len = 0;
	while (*s) {
		++len;
		++s;
	}
	return len;
}

template <typename ContainerIn, typename ContainerOut>
void unicode_convert(const ContainerIn& in, ContainerOut& out) {
	typedef typename std::iterator_traits<decltype(in.begin())>::value_type in_element_type;
	typedef typename std::iterator_traits<decltype(out.begin())>::value_type out_element_type;
	static_assert(std::is_integral<in_element_type>{}, "unicode: input must contain integral types.");
	static_assert(std::is_integral<out_element_type >{}, "unicode: output must contain integral types.");

	typedef typename std::conditional<
			sizeof(in_element_type) == 1,
			utf::utf8,
			typename std::conditional<
					sizeof(in_element_type) == 2,
					utf::utf16,
					typename std::conditional<sizeof(in_element_type) == 4, utf::utf32, bool>::type
			>::type
	>::type unicode_in_type;

	typedef typename std::conditional<
			sizeof(out_element_type) == 1,
			utf::utf8,
			typename std::conditional<
					sizeof(out_element_type) == 2,
					utf::utf16,
					typename std::conditional<sizeof(out_element_type) == 4, utf::utf32, bool>::type
			>::type
	>::type unicode_out_type;

	static_assert(!std::is_same<unicode_in_type, bool>{}, "unicode: cannot handle input type.");
	static_assert(!std::is_same<unicode_out_type, bool>{}, "unicode: cannot handle output type.");

	auto sv = utf::make_stringview(in.begin(), in.end());
	sv.template to<unicode_out_type>(std::back_inserter(out));
};

template <typename Character, typename ContainerOut>
void unicode_convert(const Character* in, ContainerOut& out, size_t inLength = SIZE_MAX) {
	typedef typename std::iterator_traits<decltype(out.begin())>::value_type out_element_type;
	static_assert(std::is_integral<Character>{}, "unicode: input must contain integral types.");
	static_assert(std::is_integral<out_element_type >{}, "unicode: output must contain integral types.");
	typedef typename std::conditional<
			sizeof(out_element_type) == 1,
			utf::utf8,
			typename std::conditional<
					sizeof(out_element_type) == 2,
					utf::utf16,
					typename std::conditional<sizeof(out_element_type) == 4, utf::utf32, bool>::type
			>::type
	>::type unicode_out_type;
	static_assert(!std::is_same<Character, bool>{}, "unicode: cannot handle bool type as input type.");
	static_assert(!std::is_same<unicode_out_type, bool>{}, "unicode: cannot handle bool type as output type.");
	inLength = std::min(inLength, stringLength(in));
	auto sv = utf::make_stringview(in, in + inLength);
	sv.template to<unicode_out_type>(std::back_inserter(out));
};

#endif //VIDEORAPTORBATCH_UNICODE_HPP
