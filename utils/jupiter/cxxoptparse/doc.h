#ifndef JUPITER_CXXOPTPARSE_DOC_H
#define JUPITER_CXXOPTPARSE_DOC_H

#include <jupiter/cxxoptparse/types.h>
#include <ostream>

namespace JUPITER
{
namespace option_parser
{
/**
 * This is for documenting. Unused.
 */
namespace doc
{
/**
 * Option metadata class
 *
 * Usually, it is good to start with inheriting value type class.
 *
 * Examples for actual accepting format for each conditions are:
 *
 * - `has_no_long_option = true`
 * - `require_value = true`
 * - `no_option_require_value = true`
 *   * `--foo=VALUE`
 *   * `--foo VALUE` (accepts but discouraged because confusing)
 *   * `--no-foo=VALUE`
 *   * `--no-foo VALUE`
 *   * `-fVALUE`
 *   * `-f VALUE`
 *
 * - `has_no_long_option = true`
 * - `require_value = true`
 * - `no_option_require_value = false`
 * - `no_option_accept_value = false`
 *   * `--foo=VALUE`
 *   * `--foo VALUE`
 *   * `--no-foo`
 *   * `-fVALUE`
 *   * `-f VALUE`
 *
 * - `has_no_long_option = true`
 * - `require_value = true`
 * - `no_option_require_value = false`
 * - `no_option_accept_value = true`
 *   * `--foo=VALUE`
 *   * `--foo VALUE`
 *   * `--no-foo`
 *   * `--no-foo=VALUE`
 *   * `-fVALUE`
 *   * `-f VALUE`
 *
 * - `has_no_long_option = true`
 * - `require_value = false`
 * - `accept_value = true`
 * - `no_option_require_value = false`
 * - `no_option_accept_value = true`
 *   * `--foo`
 *   * `--foo=VALUE`
 *   * `--no-foo`
 *   * `--no-foo=VALUE`
 *   * `-fVALUE` (short option cannot omit VALUE)
 *   * `-f VALUE`
 *
 * - `has_no_long_option = true`
 * - `require_value = false`
 * - `accept_value = true`
 * - `no_option_require_value = false`
 * - `no_option_accept_value = false`
 *   * `--foo`
 *   * `--foo=VALUE`
 *   * `--no-foo`
 *   * `-fVALUE` (short option cannot omit VALUE)
 *   * `-f VALUE`
 *
 * - `has_no_long_option = true`
 * - `require_value = false`
 * - `accept_value = false`
 * - `no_option_require_value = false`
 * - `no_option_accept_value = false`
 *   * `--foo`
 *   * `--no-foo`
 *   * `-f`
 */
class option1 : public JUPITER::option_parser::bool_option
{
public:
  /**
   * Specifies long option name. In this case `--foo`.
   *
   * This is optional, but least one of long or short option
   * is required.
   */
  static constexpr auto &long_option_name = "foo";

  /**
   * Specifies short option name. In this case `-f`.
   *
   * This is optional, but least one of long or short option
   * is required.
   */
  static constexpr char short_option_name = 'f';

  /**
   * Whether negative form of long option (`--no-foo` in this case) will also be
   * accepted.
   *
   * If not specified, treated as false.
   */
  static constexpr bool has_no_long_option = false;

  /**
   * Explicitly specify negative long option name, rather than `--no-foo`
   *
   * If this is specified, has_no_long_option will be ignored.
   *
   * This is for something like `verbose` and `quiet`. Not using antonyms may
   * confuse users.
   */
  static constexpr auto &no_long_option_name = "bar";

  /**
   * Specifies negative short option name. (same behavior as positive option,
   * but passes `false` while parsing).
   *
   * This is for something like `v` (for verbose) and `q` (for quiet). Not using
   * antonyms may confuse users.
   */
  static constexpr char no_short_option_name = 'b';

  /**
   * The type of value.
   *
   * This is mandatory.
   */
  using value_type = bool;

  /**
   * Specifies default (initial) value.
   *
   * For dynamic default values, you can set them before parse. `reset()`-ing
   * parser sets this value.
   */
  static constexpr value_type default_value() { return false; }

  /**
   * Printer for printing possible values in help, for positive or if common
   *
   * Following format recommended, but can print anything.
   *
   * ```
   *  * Option1: description
   *  * Option2: description
   *  * Option3: description
   * ```
   */
  static std::ostream &option_value_help(std::ostream &os) { return os; }

  /**
   * Printer for printing possible value in help, for negative option and if
   * different set of values are accepted
   */
  static std::ostream &no_option_value_help(std::ostream &os) { return os; }

  /**
   * Whether option requires value(s) for positive form (`-f`, `--foo`)
   *
   * Usually all options other than boolean type needs value to set, but the API
   * does not constrain to it.
   *
   * The value must be specified.
   *
   * If not speciified, treat as false.
   */
  static constexpr bool require_value = false;

  /**
   * Whether option accepts value(s) for positive form (`-f`, `--foo`)
   *
   * The value can be given, but not required.
   *
   * If not specified, treat as false.
   */
  static constexpr bool accept_value = false;

  /**
   * Whether option requires value(s) for negative form (`--no-foo`)
   *
   * Usually all options other than boolean type needs value to set, but the API
   * does not constrain to it.
   *
   * The value must be specified.
   *
   * If not speciified, treat as false.
   */
  static constexpr bool no_option_require_value = false;

  /**
   * Whether option accepts value(s) for negative form (`--no-foo`)
   *
   * The value can be given, but not required.
   *
   * If not specified, treat as false.
   */
  static constexpr bool no_option_accept_value = false;

  /**
   * Allow combine short options with others.
   *
   * For example, for option `-b` and if `allow_combine_short_option` is
   * `false`:
   *
   * - `-ab` will be rejected.
   * - If `-b` does not accept value, `-bc` will also be rejected.
   * - If `-b` accepts value, `-bc` in `c` will be treated as value.
   *
   * And if `allow_combine_short_option` is `true`:
   *
   * - `-ab` will be accepted.
   * - If `-b` accepts value, `c` in `-bc` will be treated as value and
   *   `c` in `-abc` will be treated as value.
   *
   * If not specified, treat as true.
   */
  static constexpr bool allow_combine_short_option = true;

  /**
   * @brief Set parameter (with no value)
   * @param positive or negative form used
   *
   * This function is called when values are not spcified.
   *
   * If the option always require values, this function can be omitted.
   */
  static void parse_value(value_type &value, bool positive)
  {
    value = positive;
  }

  /**
   * @brief Set parameter (with value)
   * @param positive or negative form used
   * @param beg string iterator of start position of value
   * @param end string iterator of end position of value
   *
   * This function is called when values are spcified.
   *
   * If the option always does not require values, this function can be omitted.
   */
  template <typename InIt>
  static void parse_value(value_type &value, bool positive, InIt beg, InIt end)
  {
    value = positive;
  }

  /**
   * Help text for accept or require value
   */
  static std::string value_name() { return "VALUE"; }

  /**
   * Description of option
   *
   * Multiline supported, but should not end with newline '\n'.
   */
  static std::ostream &description(std::ostream &os)
  {
    return os << "foo option";
  }

  /**
   * Description of no-option
   *
   * Multiline supported, but should not end with newline '\n'.
   *
   * If not specified, shares description with positive option.
   */
  static std::ostream &no_description(std::ostream &os)
  {
    return os << "no foo option";
  }
};

enum enum_type
{
  one
};

template <enum_type value> class enum_value_trait;

/**
 * Enum value metadata class
 *
 * Recommended to define with template specializations, but is not strictly
 * required.
 *
 * The shorthand macro,
 *
 * ```
 * DEFINE_JUPITER_CXXOPTPARSE_ENUM2STR(template_class_name, enum_type,
 *                                     enum_value, name, description)
 * ```
 *
 * defines specializations on template_class_name with enum_value.
 */
template <> class enum_value_trait<one>
{
public:
  /**
   * Enum type name
   */
  using enum_type = enum_type;

  /**
   * Value of enum
   */
  static constexpr enum_type value = one;

  /**
   * Name to print or parse
   */
  static constexpr auto &name = "Foo";

  /**
   * Description to print in value help
   */
  static std::ostream &description(std::ostream &os) { return os << "..."; }
};
} // namespace doc

} // namespace option_parser
} // namespace JUPITER

#error "this file is only for document"

#endif
