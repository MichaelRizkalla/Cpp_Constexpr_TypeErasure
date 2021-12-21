#if !defined(CONSTEXPR_FUNCTION_H_INCLUDED_06709B07_384C_42F9_9C94_E11CA87E041D)
    #define CONSTEXPR_FUNCTION_H_INCLUDED_06709B07_384C_42F9_9C94_E11CA87E041D

// Warning: the code in this header is not of a production code quality, please be careful if you decided to use it.

// Credits: this code mimics the behaviour of std::function from MSVC's STL, with changes to allow for constexpr compilation.

    #include <bit>
    #include <cstddef>
    #include <cstdint>
    #include <functional>
    #include <memory>
    #include <type_traits>
    #include <vector>

namespace mr {

    template < class Ret, class... Args >
    class constexpr_function_ptr_base {
      public:
        constexpr constexpr_function_ptr_base()                                   = default;
        constexpr constexpr_function_ptr_base(const constexpr_function_ptr_base&) = delete;
        constexpr constexpr_function_ptr_base& operator=(const constexpr_function_ptr_base&) = delete;

        constexpr virtual ~constexpr_function_ptr_base() {};

        constexpr virtual constexpr_function_ptr_base* copy() const           = 0;
        constexpr virtual void                         destroy(bool) noexcept = 0;
        constexpr virtual Ret                          call(Args&&...)        = 0;
    };

    template < class Callable, class Ret, class... Args >
    class constexpr_function_impl final : public constexpr_function_ptr_base< Ret, Args... > {
      public:
        using Base         = constexpr_function_ptr_base< Ret, Args... >;
        using nothrow_move = std::is_nothrow_move_constructible< Callable >;

        template < class T, std::enable_if_t< !std::is_same_v< constexpr_function_impl, std::decay_t< T > >, int > = 0 >
        explicit constexpr constexpr_function_impl(T&& func) : callable(std::forward< T >(func)) {
        }

        constexpr virtual ~constexpr_function_impl() {
        }

        constexpr Base* copy() const override {
            constexpr_function_impl* new_function =
                new constexpr_function_impl { callable }; // TODO: account for alignments
            return static_cast< Base* >(new_function);
        }

        constexpr Ret call(Args&&... args) override {
            return std::invoke(callable, std::forward< Args >(args)...); // invoke_r C++23
        }

        constexpr void destroy(bool dealloc) noexcept override {
            delete this;
        }

      private:
        Callable callable;
    };

    template < class Ret, class... Args >
    class constexpr_function_base {
      public:
        using result_type = Ret;
        using Func        = constexpr_function_ptr_base< Ret, Args... >;

        constexpr constexpr_function_base() noexcept {
            ptr = nullptr;
        }

        constexpr Ret operator()(Args&&... args) const {
            if (!ptr) {
                throw std::bad_function_call {};
            }
            const auto m_constexpr_function_impl = ptr;
            return m_constexpr_function_impl->call(std::forward< Args >(args)...);
        }

        constexpr ~constexpr_function_base() noexcept {
            finalise();
        }

      protected:
        template < class F, class Function >
        using Enable_if_callable_t =
            std::enable_if_t< std::conjunction_v< std::negation< std::is_same< std::decay_t< F >, Function > >,
                                                  std::is_invocable_r< Ret, F, Args... > >,
                              int >;

        constexpr void do_copy(const constexpr_function_base& rhs) {
            if (rhs.ptr) {
                ptr = rhs.ptr->copy();
            }
        }

        constexpr void do_move(constexpr_function_base&& rhs) noexcept {
            if (rhs.ptr) {
                ptr = std::exchange(rhs.ptr, nullptr);
                rhs.finalise();
            }
        }

        template < class F >
        constexpr void do_reset(F&& func) {
            // if not callable do nothing
            // todo

            using Impl = constexpr_function_impl< std::decay_t< F >, Ret, Args... >;
            auto impl  = new Impl { std::forward< F >(func) };
            ptr        = impl;
        }

        constexpr void finalise() noexcept {
            if (ptr) {
                ptr->destroy(true);
                ptr = nullptr;
            }
        }

        constexpr void do_swap(constexpr_function_base& rhs) noexcept {
            auto* tmp = ptr;
            ptr       = rhs.ptr;
            rhs.ptr   = tmp;
        }

      private:
        Func* ptr;
    };

    namespace detail {
        template < class >
        inline constexpr bool Eval_to_false = false;

        template < class NotFn >
        struct get_constexpr_function_base {
            static_assert(Eval_to_false< NotFn >,
                          "Incorrect behaviour: non-function type was passed to std::function!");
        };
        template < class Ret, class... Args >
        struct get_constexpr_function_base< Ret(Args...) > {
            using type = constexpr_function_base< Ret, Args... >;
        };
        template < class Ret, class... Args >
        struct get_constexpr_function_base< Ret(Args...) noexcept > {
            static_assert(Eval_to_false< Ret(Args...) noexcept >,
                          "Incorrect behaviour: noexcept function type was passed to std::function!");
        };

        template < class T >
        struct get_memfun_ptr_deduction {};
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...)& > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) && > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const& > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const&& > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const volatile > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const volatile& > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const volatile&& > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) volatile > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) volatile& > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) volatile&& > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...)& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...)&& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const&& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const volatile noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const volatile& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) const volatile&& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) volatile noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) volatile& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&, int&& >, Ret(Args...) >;
        };
        template < class Ret, class TClass, class... Args >
        struct get_memfun_ptr_deduction< Ret (TClass::*)(Args...) volatile&& noexcept > {
            using guide_type = std::enable_if< !std::is_same_v< int&&, int&& >, Ret(Args...) >;
        };

        template < class F, class = void >
        struct constexpr_function_deduce {}; // Invalid deduction
        template < class F >
        struct constexpr_function_deduce< F, std::void_t< decltype(&F::operator()) > >
            : get_memfun_ptr_deduction< decltype(&F::operator()) >::guide_type {};
    } // namespace detail

    template < class F >
    class constexpr_function : public detail::get_constexpr_function_base< F >::type {
      private:
        using Base = typename detail::get_constexpr_function_base< F >::type;

      public:
        constexpr constexpr_function() noexcept {
        }

        constexpr constexpr_function(std::nullptr_t) noexcept {
        }

        constexpr constexpr_function(const constexpr_function& rhs) {
            this->do_copy(rhs);
        }

        constexpr ~constexpr_function() noexcept {
        }

        template < class Fx, typename Base::template Enable_if_callable_t< Fx, constexpr_function > = 0 >
        constexpr constexpr_function(Fx func) {
            this->do_reset(std::move(func));
        }

        constexpr constexpr_function& operator=(const constexpr_function& rhs) {
            constexpr_function(rhs).swap(*this);
            return *this;
        }

        constexpr constexpr_function(constexpr_function&& rhs) noexcept {
            this->do_move(std::move(rhs));
        }

        constexpr constexpr_function& operator=(constexpr_function&& rhs) noexcept {
            if (this != std::addressof(rhs)) {
                this->finalise();
                this->do_move(std::move(rhs));
            }
            return *this;
        }

        template < class Fx, typename Base::template Enable_if_callable_t< Fx, constexpr_function > = 0 >
        constexpr constexpr_function& operator=(Fx&& func) {
            constexpr_function(std::forward< Fx >(func)).swap(*this);
            return *this;
        }

        constexpr constexpr_function& operator=(std::nullptr_t) noexcept {
            this->finalise();
            return *this;
        }

        template < class Fx >
        constexpr constexpr_function& operator=(std::reference_wrapper< Fx > func) noexcept {
            this->finalise();
            this->do_reset(func);
            return *this;
        }

        constexpr void swap(constexpr_function& _Right) noexcept {
            this->do_swap(_Right);
        }

        constexpr explicit operator bool() const noexcept {
            return !this->empty();
        }
    };

    template < class Ret, class... Args >
    constexpr_function(Ret (*)(Args...)) -> constexpr_function< Ret(Args...) >;
    template < class F >
    constexpr_function(F) -> constexpr_function< typename detail::constexpr_function_deduce< F >::type >;

    template < class Ret, class... Args >
    constexpr void swap(constexpr_function< Ret(Args...) >& lhs, constexpr_function< Ret(Args...) >& rhs) noexcept {
        lhs.swap(rhs);
    }

    template < class Ret, class... Args >
    constexpr bool operator==(const constexpr_function< Ret(Args...) >& F, std::nullptr_t) noexcept {
        return !static_cast< bool >(F);
    }

} // namespace mr

#endif // !defined(CONSTEXPR_FUNCTION_H_INCLUDED_06709B07_384C_42F9_9C94_E11CA87E041D)
