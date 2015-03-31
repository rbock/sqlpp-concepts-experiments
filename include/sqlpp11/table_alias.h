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

#ifndef SQLPP_TABLE_ALIAS_H
#define SQLPP_TABLE_ALIAS_H

#include <sqlpp11/column_fwd.h>
#include <sqlpp11/interpret.h>
#include <sqlpp11/concepts.h>
#include <sqlpp11/alias.h>
#include <sqlpp11/detail/type_set.h>

namespace sqlpp
{
	template<AliasProvider Alias, typename Table, typename... ColumnSpec>
		struct table_alias_t:
			public member_t<ColumnSpec, column_t<Alias, ColumnSpec>>...
	{
		//FIXME: Need to add join functionality
		using _traits = make_traits<value_type_of<Table>, tag::is_table, tag::is_alias, tag_if<tag::is_selectable, is_expression_t<Table>::value>>;

		using _nodes = detail::type_vector<>;
		using _required_ctes = required_ctes_of<Table>;
		using _provided_tables = detail::type_set<Alias>;

		static_assert(required_tables_of<Table>::size::value == 0, "table aliases must not depend on external tables");

		using _alias_t = typename Alias::_alias_t;
		using _column_tuple_t = std::tuple<column_t<Alias, ColumnSpec>...>;

		table_alias_t(Table table):
			_table(table)
		{}

		Table _table;
	};

	template<typename Context, AliasProvider Alias, typename Table, typename... ColumnSpec>
		struct serializer_t<Context, table_alias_t<Alias, Table, ColumnSpec...>>
		{
			using _serialize_check = serialize_check_of<Context, Table>;
			using T = table_alias_t<Alias, Table, ColumnSpec...>;

			static Context& _(const T& t, Context& context)
			{
				if (requires_braces_t<T>::value)
					context << "(";
				serialize(t._table, context);
				if (requires_braces_t<T>::value)
					context << ")";
				context << " AS " << name_of<T>::char_ptr();
				return context;
			}
		};

}

#endif

