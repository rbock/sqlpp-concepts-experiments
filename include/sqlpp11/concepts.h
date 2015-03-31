/*
 * Copyright (c) 2015-2015, Roland Bock
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

#ifndef SQLPP_CONCEPTS_H
#define SQLPP_CONCEPTS_H

#include <sqlpp11/type_traits.h>
#include <sqlpp11/detail/sum.h>

namespace sqlpp
{
	namespace detail
	{
		template<typename... Tables>
		constexpr std::size_t get_number_of_tables()
		{
			return detail::sum(provided_tables_of<Tables>::size::value...);
		}

		template<typename... Tables>
		constexpr std::size_t get_number_of_unique_tables()
		{
			using _unique_tables = detail::make_joined_set_t<provided_tables_of<Tables>...>;
			using _unique_table_names = detail::transform_set_t<name_of, _unique_tables>;
			return _unique_table_names::size::value;
		}
	}
	template<typename T>
	concept bool Table()
	{
		return is_table_t<T>::value;
	}

  template<Table... Tables>
  concept bool UniqueTableNames()
  {
		return detail::get_number_of_tables<Tables...>() == detail::get_number_of_unique_tables<Tables...>();
  }

	template<typename T>
	concept bool AliasProvider()
	{
		return requires(){
			typename T::_name_t::template _member_t<int>;
		};
	}

}

#endif
