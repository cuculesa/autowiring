# /* **************************************************************************
#  *                                                                          *
#  *     (C) Copyright Paul Mensonides 2002.
#  *     Distributed under the Boost Software License, Version 1.0. (See
#  *     accompanying file LICENSE_1_0.txt or copy at
#  *     http://www.boost.org/LICENSE_1_0.txt)
#  *                                                                          *
#  ************************************************************************** */
#
# /* See http://www.boost.org for most recent version. */
#
# ifndef AUTOBOOST_PREPROCESSOR_SEQ_FOR_EACH_PRODUCT_HPP
# define AUTOBOOST_PREPROCESSOR_SEQ_FOR_EACH_PRODUCT_HPP
#
# include <autoboost/preprocessor/arithmetic/dec.hpp>
# include <autoboost/preprocessor/config/config.hpp>
# include <autoboost/preprocessor/control/if.hpp>
# include <autoboost/preprocessor/repetition/for.hpp>
# include <autoboost/preprocessor/seq/seq.hpp>
# include <autoboost/preprocessor/seq/size.hpp>
# include <autoboost/preprocessor/tuple/elem.hpp>
# include <autoboost/preprocessor/tuple/rem.hpp>
#
# /* AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT */
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT(macro, sets) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_E(AUTOBOOST_PP_FOR, macro, sets)
#
# /* AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_R */
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_R(r, macro, sets) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_E(AUTOBOOST_PP_FOR_ ## r, macro, sets)
#
# if ~AUTOBOOST_PP_CONFIG_FLAGS() & AUTOBOOST_PP_CONFIG_EDG()
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_E(impl, macro, sets) impl((AUTOBOOST_PP_SEQ_HEAD(sets)(nil), AUTOBOOST_PP_SEQ_TAIL(sets)(nil), (nil), macro), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_0)
# else
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_E(impl, macro, sets) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_E_I(impl, macro, sets)
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_E_I(impl, macro, sets) impl((AUTOBOOST_PP_SEQ_HEAD(sets)(nil), AUTOBOOST_PP_SEQ_TAIL(sets)(nil), (nil), macro), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_0)
# endif
#
# if AUTOBOOST_PP_CONFIG_FLAGS() & AUTOBOOST_PP_CONFIG_STRICT()
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P_I data
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P_I(cset, rset, res, macro) AUTOBOOST_PP_DEC(AUTOBOOST_PP_SEQ_SIZE(cset))
# else
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P(r, data) AUTOBOOST_PP_DEC(AUTOBOOST_PP_SEQ_SIZE(AUTOBOOST_PP_TUPLE_ELEM(4, 0, data)))
# endif
#
# if ~AUTOBOOST_PP_CONFIG_FLAGS() & AUTOBOOST_PP_CONFIG_MWCC()
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O_I data
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O_I(cset, rset, res, macro) (AUTOBOOST_PP_SEQ_TAIL(cset), rset, res, macro)
# else
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O(r, data) (AUTOBOOST_PP_SEQ_TAIL(AUTOBOOST_PP_TUPLE_ELEM(4, 0, data)), AUTOBOOST_PP_TUPLE_ELEM(4, 1, data), AUTOBOOST_PP_TUPLE_ELEM(4, 2, data), AUTOBOOST_PP_TUPLE_ELEM(4, 3, data))
# endif
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, i) AUTOBOOST_PP_IF(AUTOBOOST_PP_DEC(AUTOBOOST_PP_SEQ_SIZE(AUTOBOOST_PP_TUPLE_ELEM(4, 1, data))), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_ ## i, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I)
#
# if ~AUTOBOOST_PP_CONFIG_FLAGS() & AUTOBOOST_PP_CONFIG_EDG()
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I_I(r, AUTOBOOST_PP_TUPLE_ELEM(4, 0, data), AUTOBOOST_PP_TUPLE_ELEM(4, 1, data), AUTOBOOST_PP_TUPLE_ELEM(4, 2, data), AUTOBOOST_PP_TUPLE_ELEM(4, 3, data))
# else
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I_IM(r, AUTOBOOST_PP_TUPLE_REM_4 data)
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I_IM(r, im) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I_I(r, im)
# endif
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_I_I(r, cset, rset, res, macro) macro(r, AUTOBOOST_PP_SEQ_TAIL(res (AUTOBOOST_PP_SEQ_HEAD(cset))))
#
# if ~AUTOBOOST_PP_CONFIG_FLAGS() & AUTOBOOST_PP_CONFIG_MWCC()
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H_I data
# else
#    define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H_I(AUTOBOOST_PP_TUPLE_ELEM(4, 0, data), AUTOBOOST_PP_TUPLE_ELEM(4, 1, data), AUTOBOOST_PP_TUPLE_ELEM(4, 2, data), AUTOBOOST_PP_TUPLE_ELEM(4, 3, data))
# endif
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H_I(cset, rset, res, macro) (AUTOBOOST_PP_SEQ_HEAD(rset)(nil), AUTOBOOST_PP_SEQ_TAIL(rset), res (AUTOBOOST_PP_SEQ_HEAD(cset)), macro)
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_0(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 0)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_1(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 1)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_2(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 2)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_3(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 3)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_4(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 4)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_5(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 5)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_6(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 6)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_7(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 7)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_8(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 8)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_9(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 9)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_10(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 10)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_11(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 11)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_12(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 12)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_13(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 13)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_14(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 14)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_15(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 15)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_16(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 16)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_17(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 17)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_18(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 18)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_19(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 19)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_20(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 20)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_21(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 21)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_22(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 22)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_23(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 23)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_24(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 24)(r, data)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_25(r, data) AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_C(data, 25)(r, data)
#
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_0(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_1)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_1(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_2)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_2(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_3)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_3(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_4)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_4(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_5)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_5(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_6)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_6(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_7)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_7(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_8)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_8(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_9)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_9(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_10)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_10(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_11)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_11(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_12)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_12(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_13)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_13(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_14)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_14(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_15)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_15(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_16)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_16(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_17)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_17(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_18)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_18(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_19)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_19(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_20)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_20(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_21)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_21(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_22)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_22(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_23)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_23(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_24)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_24(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_25)
# define AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_N_25(r, data) AUTOBOOST_PP_FOR_ ## r(AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_H(data), AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_P, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_O, AUTOBOOST_PP_SEQ_FOR_EACH_PRODUCT_M_26)
#
# endif
