/*
 * Copyright (c) 2013-2014, Roland Bock
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

#ifndef SQLPP_INSERT_H
#define SQLPP_INSERT_H

#include <sqlpp11/type_traits.h>
#include <sqlpp11/parameter_list.h>
#include <sqlpp11/prepared_insert.h>
#include <sqlpp11/default_value.h>
#include <sqlpp11/vendor/noop.h>
#include <sqlpp11/vendor/single_table.h>
#include <sqlpp11/vendor/insert_value_list.h>
#include <sqlpp11/vendor/policy_update.h>

#include <sqlpp11/detail/get_last.h>
#include <sqlpp11/detail/pick_arg.h>

namespace sqlpp
{
	template<typename Db, typename... Policies>
		struct insert_t;

	namespace detail
	{
		template<typename Db, typename... Policies>
			struct insert_policies_t
			{
				using _database_t = Db;

				using _statement_t = insert_t<Db, Policies...>;

				struct _methods_t: public Policies::template _methods_t<insert_policies_t>...
				{};

				template<typename Needle, typename Replacement>
					struct _policies_update_t
					{
						static_assert(detail::is_element_of<Needle, make_type_set_t<Policies...>>::value, "policies update for non-policy class detected");
						using type =  insert_t<Db, vendor::policy_update_t<Policies, Needle, Replacement>...>;
					};

				template<typename Needle, typename Replacement>
					using _new_statement_t = typename _policies_update_t<Needle, Replacement>::type;

				using _all_required_tables = detail::make_joined_set_t<required_tables_of<Policies>...>;
				using _all_provided_tables = detail::make_joined_set_t<provided_tables_of<Policies>...>;
				using _all_extra_tables = detail::make_joined_set_t<extra_tables_of<Policies>...>;

				using _known_tables = detail::make_joined_set_t<_all_provided_tables, _all_extra_tables>;

				template<typename Expression>
					using _no_unknown_tables = detail::is_subset_of<required_tables_of<Expression>, _known_tables>;

				// The tables not covered by the from.
				using _required_tables = detail::make_difference_set_t<
					_all_required_tables,
					_all_provided_tables // Hint: extra_tables are not used here because they are just a helper for dynamic .add_*()
							>;

				using _traits = make_traits<no_value_t>; // FIXME

				struct _recursive_traits
				{
					using _parameters = std::tuple<>; // FIXME
					using _required_tables = _required_tables;
					using _provided_tables = detail::type_set<>;
					using _extra_tables = detail::type_set<>;
				};
			};
	}

	// INSERT
	template<typename Db, typename... Policies>
		struct insert_t:
			public Policies...,
			public detail::insert_policies_t<Db, Policies...>::_methods_t
		{
			using _policies_t = typename detail::insert_policies_t<Db, Policies...>;
			using _database_t = typename _policies_t::_database_t;

			using _is_dynamic = typename std::conditional<std::is_same<_database_t, void>::value, std::false_type, std::true_type>::type;

			using _parameter_tuple_t = std::tuple<Policies...>;
			using _parameter_list_t = typename make_parameter_list_t<insert_t>::type;


			// Constructors
			insert_t()
			{}

			template<typename Statement, typename T>
				insert_t(Statement statement, T term):
					Policies(detail::pick_arg<Policies>(statement, term))...
			{}

			insert_t(const insert_t&) = default;
			insert_t(insert_t&&) = default;
			insert_t& operator=(const insert_t&) = default;
			insert_t& operator=(insert_t&&) = default;
			~insert_t() = default;

			// run and prepare
			static constexpr size_t _get_static_no_of_parameters()
			{
				return _parameter_list_t::size::value;
			}

			size_t _get_no_of_parameters() const
			{
				return _parameter_list_t::size::value;
			}

			void _check_consistency() const
			{
				// FIXME: Read up on what is allowed/prohibited in INSERT
			}

			template<typename Database>
				std::size_t _run(Database& db) const
				{
					_check_consistency();

					static_assert(_get_static_no_of_parameters() == 0, "cannot run insert directly with parameters, use prepare instead");
					return db.insert(*this);
				}

			template<typename Database>
				auto _prepare(Database& db) const
				-> prepared_insert_t<Database, insert_t>
				{
					_check_consistency();

					return {{}, db.prepare_insert(*this)};
				}
		};

	namespace vendor
	{
		template<typename Context, typename Database, typename... Policies>
			struct serializer_t<Context, insert_t<Database, Policies...>>
			{
				using T = insert_t<Database, Policies...>;

				static Context& _(const T& t, Context& context)
				{
					context << "INSERT INTO ";

					using swallow = int[]; 
					(void) swallow{(serialize(static_cast<const Policies&>(t), context), 0)...};
					return context;
				}
			};
	}

	template<typename Database>
		using blank_insert_t = insert_t<Database,
			vendor::no_single_table_t, 
			vendor::no_insert_value_list_t>;

	template<typename Table>
		constexpr auto insert_into(Table table)
		-> decltype(blank_insert_t<void>().into(table))
		{
			return { blank_insert_t<void>().into(table) };
		}

	template<typename Database, typename Table>
		constexpr auto  dynamic_insert_into(const Database&, Table table)
		-> decltype(blank_insert_t<Database>().into(table))
		{
			return { blank_insert_t<Database>().into(table) };
		}
}

#endif
