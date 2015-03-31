/*
 * Copyright (c) 2013-2015, Roland Bock
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLPP_CTE_H
#define SQLPP_CTE_H

#include <sqlpp11/table_ref.h>
#include <sqlpp11/select_flags.h>
#include <sqlpp11/result_row.h>
#include <sqlpp11/statement_fwd.h>
#include <sqlpp11/concepts.h>
#include <sqlpp11/parameter_list.h>
#include <sqlpp11/expression.h>
#include <sqlpp11/interpret_tuple.h>
#include <sqlpp11/interpretable_list.h>
#include <sqlpp11/logic.h>

namespace sqlpp
{
	template<typename Flag, typename Lhs, typename Rhs>
		struct cte_union_t
		{
			using _nodes = detail::type_vector<>;
			using _required_ctes = detail::make_joined_set_t<required_ctes_of<Lhs>, required_ctes_of<Rhs>>;
			using _parameters = detail::type_vector_cat_t<parameters_of<Lhs>, parameters_of<Rhs>>;

			cte_union_t(Lhs lhs, Rhs rhs):
				_lhs(lhs),
				_rhs(rhs)
			{}

			cte_union_t(const cte_union_t&) = default;
			cte_union_t(cte_union_t&&) = default;
			cte_union_t& operator=(const cte_union_t&) = default;
			cte_union_t& operator=(cte_union_t&&) = default;
			~cte_union_t() = default;

			Lhs _lhs;
			Rhs _rhs;
		};

	// Interpreters
	template<typename Context, typename Flag, typename Lhs, typename Rhs>
		struct serializer_t<Context, cte_union_t<Flag, Lhs, Rhs>>
		{
			using _serialize_check = serialize_check_of<Context, Lhs, Rhs>;
			using T = cte_union_t<Flag, Lhs, Rhs>;

			static Context& _(const T& t, Context& context)
			{
				serialize(t._lhs, context);
				context << " UNION ";
				serialize(Flag{}, context);
				context << " ";
				serialize(t._rhs, context);
				return context;
			}
		};

	template<AliasProvider Alias, typename Statement, typename... FieldSpecs>
		struct cte_t;

	template<AliasProvider Alias>
		struct cte_ref_t;

	template<AliasProvider Alias, typename Statement, typename... FieldSpecs>
		auto from_table(cte_t<Alias, Statement, FieldSpecs...> t) -> cte_ref_t<Alias>
		{
			return cte_ref_t<Alias>{};
		}

	template<AliasProvider Alias, typename Statement, typename... FieldSpecs>
		struct from_table_impl<cte_t<Alias, Statement, FieldSpecs...>>
		{
			using type = cte_ref_t<Alias>;
		};


	template<typename FieldSpec>
		struct cte_column_spec_t
		{
			using _alias_t = typename FieldSpec::_alias_t;

			using _traits = make_traits<value_type_of<FieldSpec>, 
						tag::must_not_insert, 
						tag::must_not_update,
						tag_if<tag::can_be_null, column_spec_can_be_null_t<FieldSpec>::value>
							>;
		};

	template<AliasProvider Alias, typename Statement, typename ResultRow>
		struct make_cte_impl
		{
			using type = void;
		};

	template<AliasProvider Alias, typename Statement, typename... FieldSpecs>
		struct make_cte_impl<Alias, Statement, result_row_t<void, FieldSpecs...>>
		{
			using type = cte_t<Alias, Statement, FieldSpecs...>;
		};

	template<AliasProvider Alias, typename Statement>
		using make_cte_t = typename make_cte_impl<Alias, Statement, get_result_row_t<Statement>>::type;

	template<AliasProvider Alias, typename Statement, typename... FieldSpecs>
		struct cte_t: public member_t<cte_column_spec_t<FieldSpecs>, column_t<Alias, cte_column_spec_t<FieldSpecs>>>...
		{
			using _traits = make_traits<no_value_t, tag::is_cte, tag::is_table>; // FIXME: is table? really?
			using _nodes = detail::type_vector<>;
			using _required_ctes = detail::make_joined_set_t<required_ctes_of<Statement>, detail::type_set<Alias>>;
			using _parameters = parameters_of<Statement>;

			using _alias_t = typename Alias::_alias_t;
			constexpr static bool _is_recursive = detail::is_element_of<Alias, required_ctes_of<Statement>>::value;

			using _column_tuple_t = std::tuple<column_t<Alias, cte_column_spec_t<FieldSpecs>>...>;

			template<typename... T>
				using _check = logic::all_t<is_statement_t<T>::value...>;

			using _result_row_t = result_row_t<void, FieldSpecs...>; 

			template<typename Rhs>
				auto union_distinct(Rhs rhs) const
				-> typename std::conditional<_check<Rhs>::value, cte_t<Alias, cte_union_t<distinct_t, Statement, Rhs>, FieldSpecs...>, bad_statement>::type
				{
					static_assert(is_statement_t<Rhs>::value, "argument of union call has to be a statement");
					static_assert(has_policy_t<Rhs, is_select_t>::value, "argument of union call has to be a select");
					static_assert(has_result_row_t<Rhs>::value, "argument of a union has to be a (complete) select statement");

					static_assert(std::is_same<_result_row_t, get_result_row_t<Rhs>>::value, "both select statements in a union have to have the same result columns (type and name)");

					return _union_impl<void, distinct_t>(_check<Rhs>{}, rhs);
				}

			template<typename Rhs>
				auto union_all(Rhs rhs) const
				-> typename std::conditional<_check<Rhs>::value, cte_t<Alias, cte_union_t<all_t, Statement, Rhs>, FieldSpecs...>, bad_statement>::type
				{
					static_assert(is_statement_t<Rhs>::value, "argument of union call has to be a statement");
					static_assert(has_policy_t<Rhs, is_select_t>::value, "argument of union call has to be a select");
					static_assert(has_result_row_t<Rhs>::value, "argument of a union has to be a (complete) select statement");

					static_assert(std::is_same<_result_row_t, get_result_row_t<Rhs>>::value, "both select statements in a union have to have the same result columns (type and name)");

					return _union_impl<all_t>(_check<Rhs>{}, rhs);
				}

		private:
			template<typename Flag, typename Rhs>
				auto _union_impl(const std::false_type&, Rhs rhs) const
				-> bad_statement;

			template<typename Flag, typename Rhs>
				auto _union_impl(const std::true_type&, Rhs rhs) const
				-> cte_t<Alias, cte_union_t<Flag, Statement, Rhs>, FieldSpecs...>
				{
					return cte_union_t<Flag, Statement, Rhs>{_statement, rhs};
				}

		public:

			cte_t(Statement statement): _statement(statement){}
			cte_t(const cte_t&) = default;
			cte_t(cte_t&&) = default;
			cte_t& operator=(const cte_t&) = default;
			cte_t& operator=(cte_t&&) = default;
			~cte_t() = default;

			Statement _statement;
		};

	template<typename Context, AliasProvider Alias, typename Statement, typename... ColumnSpecs>
		struct serializer_t<Context, cte_t<Alias, Statement, ColumnSpecs...>>
		{
			using _serialize_check = serialize_check_of<Context, Statement>;
			using T = cte_t<Alias, Statement, ColumnSpecs...>;

			static Context& _(const T& t, Context& context)
			{
				context << name_of<T>::char_ptr() << " AS (";
				serialize(t._statement, context);
				context << ")";
				return context;
			}
		};


// The cte_t is displayed as Alias except within the with:
//    - the with needs the 
//      Alias AS (ColumnNames) (select/union)
// The result row of the select should not have dynamic parts
	template<AliasProvider Alias>
		struct cte_ref_t
		{
			using _traits = make_traits<no_value_t, tag::is_alias, tag::is_cte, tag::is_table>; // FIXME: is table? really?
			using _nodes = detail::type_vector<>;
			using _required_ctes = detail::make_type_set_t<Alias>;
			using _provided_tables = detail::type_set<Alias>;

			using _alias_t = typename Alias::_alias_t;

			template<typename Statement>
				auto as(Statement statement)
				-> make_cte_t<Alias, Statement>
				{
					static_assert(required_tables_of<Statement>::size::value == 0, "common table expression must not use unknown tables");
					static_assert(not detail::is_element_of<Alias, required_ctes_of<Statement>>::value, "common table expression must not self-reference in the first part, use union_all/union_distinct for recursion");
					static_assert(is_static_result_row_t<get_result_row_t<Statement>>::value, "ctes must not have dynamically added columns");

					return { statement };
				}
		};

	template<typename Context, AliasProvider Alias>
		struct serializer_t<Context, cte_ref_t<Alias>>
		{
			using _serialize_check = consistent_t;
			using T = cte_ref_t<Alias>;

			static Context& _(const T& t, Context& context)
			{
				context << name_of<T>::char_ptr();
				return context;
			}
		};

	template<AliasProvider Alias>
		auto cte(const Alias&)
		-> cte_ref_t<Alias>
		{
			return {};
		}

}

#endif
