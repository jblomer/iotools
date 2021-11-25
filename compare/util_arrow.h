/**
 * Copyright CERN; j.lopez@cern.ch
 */

#ifndef UTIL_ARROW_H_
#define UTIL_ARROW_H_

#include <arrow/builder.h>
#include <arrow/table.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/// \brief Adapter to use list fields (i.e. `arrow::list`) in ArrowTableBuilder
///
template <class T>
class ListAdapter {
  using ConstValuePtr_t = typename std::conditional<std::is_same<T, arrow::BooleanBuilder>::value,
				      const uint8_t*,
				      const typename T::value_type*>::type;

  arrow::MemoryPool *pool = arrow::default_memory_pool();
  arrow::ListBuilder listBuilder;
  T *valueBuilder;
public:
  using value_type = std::pair<const void* /*data*/, int /*size*/>;

  ListAdapter() : listBuilder(pool, std::make_shared<T>(pool)) {
    valueBuilder = static_cast<T*>(listBuilder.value_builder());
  }

  arrow::Status Append(const value_type &val) {
    ARROW_RETURN_NOT_OK(listBuilder.Append());
    return valueBuilder->AppendValues(reinterpret_cast<ConstValuePtr_t>(val.first), val.second);
  }

  arrow::Result<std::shared_ptr<arrow::Array>> Finish()
  { return listBuilder.Finish(); }
};

/// \brief Helper class to build a `arrow::Table` when a given row limit is
/// reached. The constructed table is handed over to a user-specified function.
///
/// This class is templated over the array builder types, e.g. `UInt32Builder`.
template <class... Ts>
class ArrowTableBuilder {
  using Callback_t = std::function<void(std::shared_ptr<arrow::Table>)>;

  std::tuple<Ts...> arrayBuilder;
  std::shared_ptr<arrow::Schema> schema;
  size_t rowLimit, count = 0;
  Callback_t doSomethingWithTable;

  void MakeTable() {
    std::vector<std::shared_ptr<arrow::Array>> v;

    // FIXME: should check the value of `args.Finish().ok()`
    std::apply([&v](Ts&... args) { ((v.push_back(*args.Finish())), ...); },
	       arrayBuilder);

    auto table = arrow::Table::Make(schema, v);
    doSomethingWithTable(table);
    count = 0;
  }

  template <std::size_t... Is>
  void AppendImpl(const std::tuple<typename Ts::value_type...>& values,
		  std::index_sequence<Is...>)
  {
    ((std::get<Is>(arrayBuilder).Append(std::get<Is>(values))), ...);
  }

public:
  ArrowTableBuilder(std::shared_ptr<arrow::Schema> schema, size_t rowLimit, Callback_t cb)
    : schema(schema), rowLimit(rowLimit), doSomethingWithTable(cb) {}
  ~ArrowTableBuilder() { if (count) MakeTable(); }

  size_t GetCount() const { return count; }

  /// \brief Append the given values to the corresponding array builder, i.e.
  /// i-th value in `values` shall be appended to `std::get<i>(arrayBuilder)`
  void Append(const std::tuple<typename Ts::value_type...>& values) {
    AppendImpl(values, std::index_sequence_for<Ts...>{});
    if (++count == rowLimit)
      MakeTable();
  }
};

template <class... Ts>
ArrowTableBuilder<Ts...>& operator<<(ArrowTableBuilder<Ts...> &builder,
				     const std::tuple<typename Ts::value_type...>& values)
{
  builder.Append(values);
  return builder;
}

/// \brief Return the indices in `schema` for the columns specified in `names`.
static inline std::vector<int>
GetColumnIndices(const std::shared_ptr<arrow::Schema> schema, const std::vector<std::string>& names)
{
  auto status = schema->CanReferenceFieldsByNames(names);
  if (!status.ok())
    throw std::runtime_error(status.message());

  std::vector<int> ret;
  for (auto &I : names)
    ret.push_back(schema->GetFieldIndex(I));
  return ret;
}

#endif // UTIL_ARROW_H_
