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

#ifndef SQLPP_VENDOR_SINGLE_TABLE_H
#define SQLPP_VENDOR_SINGLE_TABLE_H

#include <sqlpp11/type_traits.h>
#include <sqlpp11/detail/type_set.h>

namespace sqlpp
{
	namespace vendor
	{
		// A SINGLE TABLE
		template<typename Database, typename Table>
			struct single_table_t
			{
				using _traits = make_traits<no_value_t, ::sqlpp::tag::single_table>;
				using _recursive_traits = make_recursive_traits<Table>;

				static_assert(is_table_t<Table>::value, "argument has to be a table");
				static_assert(required_tables_of<Table>::size::value == 0, "table depends on another table");

				single_table_t(Table table):
					_table(table)
				{}

				single_table_t(const single_table_t&) = default;
				single_table_t(single_table_t&&) = default;
				single_table_t& operator=(const single_table_t&) = default;
				single_table_t& operator=(single_table_t&&) = default;
				~single_table_t() = default;

				template<typename Policies>
					struct _methods_t
					{
					};

				Table _table;
			};

		struct no_single_table_t
		{
			using _traits = make_traits<no_value_t, ::sqlpp::tag::noop>;
			using _recursive_traits = make_recursive_traits<>;

			template<typename Policies>
				struct _methods_t
				{
					using _database_t = typename Policies::_database_t;
					template<typename T>
					using _new_statement_t = typename Policies::template _new_statement_t<no_single_table_t, T>;

					template<typename... Args>
						auto into(Args... args)
						-> _new_statement_t<single_table_t<void, Args...>>
						{
							return { *static_cast<typename Policies::_statement_t*>(this), single_table_t<void, Args...>{args...} };
						}
				};
		};

		// Interpreters
		template<typename Context, typename Database, typename Table>
			struct serializer_t<Context, single_table_t<Database, Table>>
			{
				using T = single_table_t<Database, Table>;

				static Context& _(const T& t, Context& context)
				{
					serialize(t._table, context);
					return context;
				}
			};

	}
}

#endif
