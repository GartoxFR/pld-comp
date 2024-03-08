#pragma once

#include "Instructions.h"
#include "Terminators.h"

namespace ir {
    namespace detail {

        template <typename T, typename... Ts>
            requires std::derived_from<T, Visitable> && (... && std::derived_from<Ts, Visitable>)
        class TemplateVisitor : TemplateVisitor<Ts...> {
          public:
            virtual void visit(T& t) {}

            using TemplateVisitor<Ts...>::visit;
        };

        template <typename T>
            requires std::derived_from<T, Visitable>
        class TemplateVisitor<T> {
          public:
            virtual void visit(T& t) {}
        };
    }

    class Visitor : public detail::TemplateVisitor<BinaryOp, Copy, Terminator> {};

    template <class T>
    struct decompose;

    template <class T, class Ret, class Arg>
    struct decompose<Ret (T::*)(Arg) const> {
        using ret_type = Ret;
        using arg_type = Arg;
    };

    template <class T, class... Ts>
    concept AllSameReturn =
        (... and
         std::same_as<
             typename decompose<decltype(&T::operator())>::ret_type,
             typename decompose<decltype(&Ts::operator())>::ret_type>);

    template <class T>
    class visitor_return {
      public:
        static constexpr bool has_return = false;
    };

    template <class T>
        requires(!std::is_void_v<T>)
    class visitor_return<T> {
      public:
        static constexpr bool has_return = true;
        auto&& getRet() {
            return std::move(ret);
        }

      protected:
        T ret;
    };

    template <class T, class... Ts>
        requires AllSameReturn<T, Ts...>
    class visitor : public visitor<Ts...>, T {
      public:
        visitor(T t, Ts... ts) : T(t), visitor<Ts...>(ts...) {}

        void visit(decompose<decltype(&T::operator())>::arg_type arg) override {
            if constexpr (has_return) {
                visitor<Ts...>::ret = T::operator()(arg);
            } else {
                T::operator()(arg);
            }
        }

        using visitor<Ts...>::has_return;
        using visitor<Ts...>::visit;
    };

    template <class T>
    class visitor<T>
        : public Visitor, public visitor_return<typename decompose<decltype(&T::operator())>::ret_type>, T {

      public:
        using visitor_return<typename decompose<decltype(&T::operator())>::ret_type>::has_return;
        visitor(T t) : T(t) {}
        void visit(decompose<decltype(&T::operator())>::arg_type arg) override {
            if constexpr (has_return) {
                visitor_return<typename decompose<decltype(&T::operator())>::ret_type>::ret = T::operator()(arg);
            } else {
                T::operator()(arg);
            }
        }
    };

    template <class... Ts>
    visitor(Ts...) -> visitor<Ts...>;
    template <class T>
    visitor(T) -> visitor<T>;

    template <class VisitorT, class VisitableT>
        requires std::derived_from<VisitableT, Visitable> &&
        std::derived_from<std::remove_reference_t<VisitorT>, Visitor>
    auto visit(VisitorT&& visitor, VisitableT& visited) {
        visited.accept(visitor);

        // If VisitorT is a visitor from the overloaded template
        if constexpr (requires { std::remove_reference_t<VisitorT>::has_return; }) {
            // And all the lamda have a return type
            if constexpr (std::remove_reference_t<VisitorT>::has_return) {
                return visitor.getRet();
            }
        }
    }

}
