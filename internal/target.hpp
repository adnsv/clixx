#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <optional>

namespace cli {

using target_ref = std::variant<                        //
    std::reference_wrapper<bool>,                       // boolean flag
    std::reference_wrapper<std::optional<bool>>,        // boolean flag
    std::reference_wrapper<std::string>,                // string option
    std::reference_wrapper<std::optional<std::string>>, // string option
    std::reference_wrapper<std::vector<std::string>>    // string list option
    >;

struct target : public target_ref {
    bool required = false; // must be in use

    target(bool& v, bool required = true)
        : target_ref{v}
        , required{required}
    {
    }
    target(std::optional<bool>& v, bool required = false)
        : target_ref{v}
        , required{required}
    {
    }
    target(std::string& v, bool required = true)
        : target_ref{v}
        , required{required}
    {
    }
    target(std::optional<std::string>& v, bool required = false)
        : target_ref{v}
        , required{required}
    {
    }
    target(std::vector<std::string>& v, bool required = true)
        : target_ref{v}
        , required{required}
    {
    }

    auto is_bool() const -> bool
    {
        return std::holds_alternative<std::reference_wrapper<bool>>(*this) ||
               std::holds_alternative<
                   std::reference_wrapper<std::optional<bool>>>(*this);
    }

    auto is_vector() const -> bool
    {
        return std::holds_alternative<
            std::reference_wrapper<std::vector<std::string>>>(*this);
    }

    void write(bool value)
    {
        if (auto v = std::get_if<std::reference_wrapper<bool>>(this)) {
            v->get() = value;
        }
        else if (auto v =
                     std::get_if<std::reference_wrapper<std::optional<bool>>>(
                         this)) {
            v->get() = value;
        }
    }

    void write(std::string_view value)
    {
        if (auto v = std::get_if<std::reference_wrapper<bool>>(this)) {
            if (value == "true")
                v->get() = true;
            else if (value == "false")
                v->get() = false;
            else
                throw error{std::string{"expected 'true' or 'false', got: '"} +
                            std::string{value} + "' instead"};
        }
        else if (auto v =
                     std::get_if<std::reference_wrapper<std::optional<bool>>>(
                         this)) {
            if (value == "true")
                v->get() = true;
            else if (value == "false")
                v->get() = false;
            else
                throw error{std::string{"expected 'true' or 'false', got: '"} +
                            std::string{value} + "' instead"};
        }
        else if (auto v =
                     std::get_if<std::reference_wrapper<std::string>>(this)) {
            v->get() = value;
        }
        else if (auto v = std::get_if<
                     std::reference_wrapper<std::optional<std::string>>>(
                     this)) {
            v->get() = value;
        }
        else if (auto v = std::get_if<
                     std::reference_wrapper<std::vector<std::string>>>(this)) {
            v->get().push_back(std::string{value});
        }
    }
};

} // namespace cli