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

#ifndef SQLPP_LIMIT_H
#define SQLPP_LIMIT_H

#include <sqlpp11/concepts.h>
#include <sqlpp11/policy_update.h>
#include <sqlpp11/detail/type_set.h>

namespace sqlpp
{
	// LIMIT DATA
	template<typename Limit>
		struct limit_data_t
		{
			limit_data_t(Limit value):
				_value(value)
			{}

			limit_data_t(const limit_data_t&) = default;
			limit_data_t(limit_data_t&&) = default;
			limit_data_t& operator=(const limit_data_t&) = default;
			limit_data_t& operator=(limit_data_t&&) = default;
			~limit_data_t() = default;

			Limit _value;
		};

	// LIMIT
	template<typename Limit>
		struct limit_t
		{
			using _traits = make_traits<no_value_t, tag::is_limit>;
			using _nodes = detail::type_vector<Limit>;

			// Data
			using _data_t = limit_data_t<Limit>;

			// Member implementation with data and methods
			template <typename Policies>
				struct _impl_t
				{
					_data_t _data;
				};

			// Base template to be inherited by the statement
			template<typename Policies>
				struct _base_t
				{
					using _data_t = limit_data_t<Limit>;

					_impl_t<Policies> limit;
					_impl_t<Policies>& operator()() { return limit; }
					const _impl_t<Policies>& operator()() const { return limit; }

					template<typename T>
						static auto _get_member(T t) -> decltype(t.limit)
						{
							return t.limit;
						}

					using _consistency_check = consistent_t;
				};
		};

	// DYNAMIC LIMIT DATA
	template<typename Database>
		struct dynamic_limit_data_t
		{
			dynamic_limit_data_t():
				_value(noop())
			{
			}

			template<typename Limit>
				dynamic_limit_data_t(Limit value):
					_initialized(true),
					_value(wrap_operand_t<Limit>(value))
			{
			}

			dynamic_limit_data_t(const dynamic_limit_data_t&) = default;
			dynamic_limit_data_t(dynamic_limit_data_t&&) = default;
			dynamic_limit_data_t& operator=(const dynamic_limit_data_t&) = default;
			dynamic_limit_data_t& operator=(dynamic_limit_data_t&&) = default;
			~dynamic_limit_data_t() = default;

			bool _initialized = false;
			interpretable_t<Database> _value;
		};

	// DYNAMIC LIMIT
	template<typename Database>
		struct dynamic_limit_t
		{
			using _traits = make_traits<no_value_t, tag::is_limit>;
			using _nodes = detail::type_vector<>;

			// Data
			using _data_t = dynamic_limit_data_t<Database>;

			// Member implementation with data and methods
			template <typename Policies>
				struct _impl_t
				{
					template<typename Limit>
						void set(Limit value)
						{
							// FIXME: Make sure that Limit does not require external tables? Need to read up on SQL
							using arg_t = wrap_operand_t<Limit>;
							static_assert(is_integral_t<arg_t>::value, "limit requires an integral value or integral parameter");
							_data._value = arg_t{value};
							_data._initialized = true;
						}
				public:
					_data_t _data;
				};

			// Base template to be inherited by the statement
			template<typename Policies>
				struct _base_t
				{
					using _data_t = dynamic_limit_data_t<Database>;

					_impl_t<Policies> limit;
					_impl_t<Policies>& operator()() { return limit; }
					const _impl_t<Policies>& operator()() const { return limit; }

					template<typename T>
						static auto _get_member(T t) -> decltype(t.limit)
						{
							return t.limit;
						}

					using _consistency_check = consistent_t;
				};
		};

	struct no_limit_t
	{
		using _traits = make_traits<no_value_t, tag::is_noop>;
		using _nodes = detail::type_vector<>;

		// Data
		using _data_t = no_data_t;

		// Member implementation with data and methods
		template<typename Policies>
			struct _impl_t
			{
				_data_t _data;
			};

		// Base template to be inherited by the statement
		template<typename Policies>
			struct _base_t
			{
				using _data_t = no_data_t;

				_impl_t<Policies> no_limit;
				_impl_t<Policies>& operator()() { return no_limit; }
				const _impl_t<Policies>& operator()() const { return no_limit; }

				template<typename T>
					static auto _get_member(T t) -> decltype(t.no_limit)
					{
						return t.no_limit;
					}

				using _database_t = typename Policies::_database_t;

				template<typename T>
					using _check = is_integral_t<wrap_operand_t<T>>;

				template<typename Check, typename T>
					using _new_statement_t = new_statement_t<Check::value, Policies, no_limit_t, T>;

				using _consistency_check = consistent_t;

				template<typename Arg>
					auto limit(Arg arg) const
					-> _new_statement_t<_check<Arg>, limit_t<wrap_operand_t<Arg>>>
					{
						static_assert(_check<Arg>::value, "limit requires an integral value or integral parameter");
						return _limit_impl(_check<Arg>{}, wrap_operand_t<Arg>{arg});
					}

				auto dynamic_limit() const
					-> _new_statement_t<std::true_type, dynamic_limit_t<_database_t>>
					{
						return { static_cast<const derived_statement_t<Policies>&>(*this), dynamic_limit_data_t<_database_t>{} };
					}

			private:
				template<typename Arg>
					auto _limit_impl(const std::false_type&, Arg arg) const
					-> bad_statement;

				template<typename Arg>
					auto _limit_impl(const std::true_type&, Arg arg) const
					-> _new_statement_t<std::true_type, limit_t<Arg>>
					{
						return { static_cast<const derived_statement_t<Policies>&>(*this), limit_data_t<Arg>{arg} };
					}

			};
	};

	// Interpreters
	template<typename Context, typename Database>
		struct serializer_t<Context, dynamic_limit_data_t<Database>>
		{
			using _serialize_check = consistent_t;
			using T = dynamic_limit_data_t<Database>;

			static Context& _(const T& t, Context& context)
			{
				if (t._initialized)
				{
					context << " LIMIT ";
					serialize(t._value, context);
				}
				return context;
			}
		};

	template<typename Context, typename Limit>
		struct serializer_t<Context, limit_data_t<Limit>>
		{
			using _serialize_check = serialize_check_of<Context, Limit>;
			using T = limit_data_t<Limit>;

			static Context& _(const T& t, Context& context)
			{
				context << " LIMIT ";
				serialize_operand(t._value, context);
				return context;
			}
		};
}

#endif
