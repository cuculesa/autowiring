/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE:        wc_regex_traits.cpp
  *   VERSION:     see <autoboost/version.hpp>
  *   DESCRIPTION: Implements out of line members for c_regex_traits<wchar_t>
  */


#define AUTOBOOST_REGEX_SOURCE

#include <autoboost/detail/workaround.hpp>
#include <memory>
#include <string>
#include "internals.hpp"

#if defined(_DLL_CPPLIB) && !defined(_M_CEE_PURE) && defined(_NATIVE_WCHAR_T_DEFINED) \
   && !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION) || defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER))\
   && AUTOBOOST_WORKAROUND(AUTOBOOST_MSVC, <1600)
//
// This is a horrible workaround, but without declaring these symbols extern we get
// duplicate symbol errors when linking if the application is built without
// /Zc:wchar_t
//
#ifdef _CRTIMP2_PURE
#  define AUTOBOOST_REGEX_STDLIB_DECL _CRTIMP2_PURE
#else
#  define AUTOBOOST_REGEX_STDLIB_DECL _CRTIMP2
#endif

namespace std{

#if AUTOBOOST_WORKAROUND(AUTOBOOST_MSVC, >= 1400)
template class AUTOBOOST_REGEX_STDLIB_DECL allocator<unsigned short>;
template class AUTOBOOST_REGEX_STDLIB_DECL _String_val<unsigned short, allocator<unsigned short> >;
template class AUTOBOOST_REGEX_STDLIB_DECL basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >;
#endif

#if AUTOBOOST_WORKAROUND(AUTOBOOST_MSVC, > 1300) && AUTOBOOST_WORKAROUND(AUTOBOOST_MSVC, AUTOBOOST_TESTED_AT(1400))
template<> AUTOBOOST_REGEX_STDLIB_DECL std::size_t __cdecl char_traits<unsigned short>::length(unsigned short const*);
#endif

template AUTOBOOST_REGEX_STDLIB_DECL bool __cdecl operator==(
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template AUTOBOOST_REGEX_STDLIB_DECL bool __cdecl operator==(
   const unsigned short *,
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template AUTOBOOST_REGEX_STDLIB_DECL bool __cdecl operator==(
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
   const unsigned short *);
template AUTOBOOST_REGEX_STDLIB_DECL bool __cdecl operator<(
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template AUTOBOOST_REGEX_STDLIB_DECL bool __cdecl operator>(
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
   const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
}
#endif

#include <autoboost/regex/config.hpp>
#include <autoboost/detail/workaround.hpp>

#if !AUTOBOOST_WORKAROUND(__BORLANDC__, < 0x560)

#include <autoboost/regex/v4/c_regex_traits.hpp>
#ifndef AUTOBOOST_NO_WREGEX
#include <autoboost/regex/v4/primary_transform.hpp>
#include <autoboost/regex/v4/regex_traits_defaults.hpp>

#if defined(AUTOBOOST_NO_STDC_NAMESPACE)
namespace std{
   using ::wcstol;
}
#endif

namespace autoboost{

c_regex_traits<wchar_t>::string_type AUTOBOOST_REGEX_CALL c_regex_traits<wchar_t>::transform(const wchar_t* p1, const wchar_t* p2)
{
   std::size_t r;
   std::size_t s = 10;
   std::wstring src(p1, p2);
   std::wstring result(s, L' ');
   while(s < (r = std::wcsxfrm(&*result.begin(), src.c_str(), s)))
   {
#if defined(_CPPLIB_VER)
      //
      // A bug in VC11 and 12 causes the program to hang if we pass a null-string
      // to std::strxfrm, but only for certain locales :-(
      // Probably effects Intel and Clang or any compiler using the VC std library (Dinkumware).
      //
      if(r == INT_MAX)
      {
         result.erase();
         result.insert(result.begin(), static_cast<wchar_t>(0));
         return result;
      }
#endif
      result.append(r - s + 3, L' ');
      s = result.size();
   }
   result.erase(r);
   return result;
}

c_regex_traits<wchar_t>::string_type AUTOBOOST_REGEX_CALL c_regex_traits<wchar_t>::transform_primary(const wchar_t* p1, const wchar_t* p2)
{
   static wchar_t s_delim;
   static const int s_collate_type = ::autoboost::re_detail::find_sort_syntax(static_cast<const c_regex_traits<wchar_t>*>(0), &s_delim);
   std::wstring result;
   //
   // What we do here depends upon the format of the sort key returned by
   // sort key returned by this->transform:
   //
   switch(s_collate_type)
   {
   case ::autoboost::re_detail::sort_C:
   case ::autoboost::re_detail::sort_unknown:
      // the best we can do is translate to lower case, then get a regular sort key:
      {
         result.assign(p1, p2);
         for(std::wstring::size_type i = 0; i < result.size(); ++i)
            result[i] = (std::towlower)(result[i]);
         result = c_regex_traits<wchar_t>::transform(&*result.begin(), &*result.begin() + result.size());
         break;
      }
   case ::autoboost::re_detail::sort_fixed:
      {
         // get a regular sort key, and then truncate it:
         result = c_regex_traits<wchar_t>::transform(&*result.begin(), &*result.begin() + result.size());
         result.erase(s_delim);
         break;
      }
   case ::autoboost::re_detail::sort_delim:
         // get a regular sort key, and then truncate everything after the delim:
         result = c_regex_traits<wchar_t>::transform(&*result.begin(), &*result.begin() + result.size());
         if(result.size() && (result[0] == s_delim))
            break;
         std::size_t i;
         for(i = 0; i < result.size(); ++i)
         {
            if(result[i] == s_delim)
               break;
         }
         result.erase(i);
         break;
   }
   if(result.empty())
      result = std::wstring(1, char(0));
   return result;
}

c_regex_traits<wchar_t>::char_class_type AUTOBOOST_REGEX_CALL c_regex_traits<wchar_t>::lookup_classname(const wchar_t* p1, const wchar_t* p2)
{
   static const char_class_type masks[] =
   {
      0,
      char_class_alnum,
      char_class_alpha,
      char_class_blank,
      char_class_cntrl,
      char_class_digit,
      char_class_digit,
      char_class_graph,
      char_class_horizontal,
      char_class_lower,
      char_class_lower,
      char_class_print,
      char_class_punct,
      char_class_space,
      char_class_space,
      char_class_upper,
      char_class_unicode,
      char_class_upper,
      char_class_vertical,
      char_class_alnum | char_class_word,
      char_class_alnum | char_class_word,
      char_class_xdigit,
   };

   int idx = ::autoboost::re_detail::get_default_class_id(p1, p2);
   if(idx < 0)
   {
      std::wstring s(p1, p2);
      for(std::wstring::size_type i = 0; i < s.size(); ++i)
         s[i] = (std::towlower)(s[i]);
      idx = ::autoboost::re_detail::get_default_class_id(&*s.begin(), &*s.begin() + s.size());
   }
   AUTOBOOST_ASSERT(idx+1 < static_cast<int>(sizeof(masks) / sizeof(masks[0])));
   return masks[idx+1];
}

bool AUTOBOOST_REGEX_CALL c_regex_traits<wchar_t>::isctype(wchar_t c, char_class_type mask)
{
   return
      ((mask & char_class_space) && (std::iswspace)(c))
      || ((mask & char_class_print) && (std::iswprint)(c))
      || ((mask & char_class_cntrl) && (std::iswcntrl)(c))
      || ((mask & char_class_upper) && (std::iswupper)(c))
      || ((mask & char_class_lower) && (std::iswlower)(c))
      || ((mask & char_class_alpha) && (std::iswalpha)(c))
      || ((mask & char_class_digit) && (std::iswdigit)(c))
      || ((mask & char_class_punct) && (std::iswpunct)(c))
      || ((mask & char_class_xdigit) && (std::iswxdigit)(c))
      || ((mask & char_class_blank) && (std::iswspace)(c) && !::autoboost::re_detail::is_separator(c))
      || ((mask & char_class_word) && (c == '_'))
      || ((mask & char_class_unicode) && (c & ~static_cast<wchar_t>(0xff)))
      || ((mask & char_class_vertical) && (::autoboost::re_detail::is_separator(c) || (c == L'\v')))
      || ((mask & char_class_horizontal) && (std::iswspace)(c) && !::autoboost::re_detail::is_separator(c) && (c != L'\v'));
}

c_regex_traits<wchar_t>::string_type AUTOBOOST_REGEX_CALL c_regex_traits<wchar_t>::lookup_collatename(const wchar_t* p1, const wchar_t* p2)
{
#if !defined(AUTOBOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !AUTOBOOST_WORKAROUND(AUTOBOOST_MSVC, < 1300)\
               && !AUTOBOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
   std::string name(p1, p2);
#else
   std::string name;
   const wchar_t* p0 = p1;
   while(p0 != p2)
      name.append(1, char(*p0++));
#endif
   name = ::autoboost::re_detail::lookup_default_collate_name(name);
#if !defined(AUTOBOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !AUTOBOOST_WORKAROUND(AUTOBOOST_MSVC, < 1300)\
               && !AUTOBOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
   if(name.size())
      return string_type(name.begin(), name.end());
#else
   if(name.size())
   {
      string_type result;
      typedef std::string::const_iterator iter;
      iter b = name.begin();
      iter e = name.end();
      while(b != e)
         result.append(1, wchar_t(*b++));
      return result;
   }
#endif
   if(p2 - p1 == 1)
      return string_type(1, *p1);
   return string_type();
}

int AUTOBOOST_REGEX_CALL c_regex_traits<wchar_t>::value(wchar_t c, int radix)
{
#ifdef __BORLANDC__
   // workaround for broken wcstol:
   if((std::iswxdigit)(c) == 0)
      return -1;
#endif
   wchar_t b[2] = { c, '\0', };
   wchar_t* ep;
   int result = std::wcstol(b, &ep, radix);
   if(ep == b)
      return -1;
   return result;
}

#ifdef AUTOBOOST_REGEX_HAS_OTHER_WCHAR_T
c_regex_traits<unsigned short>::string_type AUTOBOOST_REGEX_CALL c_regex_traits<unsigned short>::transform(const unsigned short* p1, const unsigned short* p2)
{
   std::wstring result = c_regex_traits<wchar_t>::transform((const wchar_t*)p1, (const wchar_t*)p2);
   return string_type(result.begin(), result.end());
}

c_regex_traits<unsigned short>::string_type AUTOBOOST_REGEX_CALL c_regex_traits<unsigned short>::transform_primary(const unsigned short* p1, const unsigned short* p2)
{
   std::wstring result = c_regex_traits<wchar_t>::transform_primary((const wchar_t*)p1, (const wchar_t*)p2);
   return string_type(result.begin(), result.end());
}

c_regex_traits<unsigned short>::char_class_type AUTOBOOST_REGEX_CALL c_regex_traits<unsigned short>::lookup_classname(const unsigned short* p1, const unsigned short* p2)
{
   return c_regex_traits<wchar_t>::lookup_classname((const wchar_t*)p1, (const wchar_t*)p2);
}

c_regex_traits<unsigned short>::string_type AUTOBOOST_REGEX_CALL c_regex_traits<unsigned short>::lookup_collatename(const unsigned short* p1, const unsigned short* p2)
{
   std::wstring result = c_regex_traits<wchar_t>::lookup_collatename((const wchar_t*)p1, (const wchar_t*)p2);
   return string_type(result.begin(), result.end());
}

bool AUTOBOOST_REGEX_CALL c_regex_traits<unsigned short>::isctype(unsigned short c, char_class_type m)
{
   return c_regex_traits<wchar_t>::isctype(c, m);
}

int AUTOBOOST_REGEX_CALL c_regex_traits<unsigned short>::value(unsigned short c, int radix)
{
   return c_regex_traits<wchar_t>::value(c, radix);
}

#endif

}

#endif // AUTOBOOST_NO_WREGEX

#endif // __BORLANDC__

