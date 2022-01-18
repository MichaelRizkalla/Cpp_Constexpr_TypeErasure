#if !defined(CONSTEXPR_FUNCTION_H_INCLUDED_06709B07_384C_42F9_9C94_E11CA87E041D)
    #define CONSTEXPR_FUNCTION_H_INCLUDED_06709B07_384C_42F9_9C94_E11CA87E041D

// Credits: this code mimics the behaviour of std::function from MSVC's STL, with changes to allow for constexpr
// compilation.

    #include <bit>
    #include <cstddef>
    #include <cstdint>
    #include <functional>
    #include <memory>
    #include <type_traits>
    #include <vector>

namespace mr {

    template < class F >
    class function;

    namespace detail {

        template < class Type, template < class... > class Template >
        inline constexpr bool is_specialization_of_v = false;
        template < template < class... > class Template, class... Types >
        inline constexpr bool is_specialization_of_v< Template< Types... >, Template > = true;

        template < class Type, template < class... > class Template >
        struct is_specialization_of : std::bool_constant< is_specialization_of_v< Type, Template > > {};

        template < class >
        inline constexpr bool Eval_function_to_false = false;

        class constexpr_function_empty_class;

        struct constexpr_function_storage_t {
            constexpr constexpr_function_storage_t() noexcept {
            }
            constexpr ~constexpr_function_storage_t() noexcept {
            }
        };

        template < class Callable >
        struct constexpr_function_data_t : constexpr_function_storage_t {
            constexpr constexpr_function_data_t(Callable* callable_) noexcept : callable(callable_) {
            }
            constexpr ~constexpr_function_data_t() noexcept {
                delete callable;
            }

            inline static constexpr std::uint8_t Type_id = 0;

            Callable* callable;
        };

        template < class T >
        constexpr T* get_constexpr_function_data_as(constexpr_function_storage_t* ptr) noexcept {
            return static_cast< constexpr_function_data_t< T >* >(ptr)->callable;
        }
        template < class T >
        constexpr const T* get_constexpr_function_data_as(const constexpr_function_storage_t* ptr) noexcept {
            return static_cast< const constexpr_function_data_t< T >* >(ptr)->callable;
        }

        enum class constexpr_function_op : int
        {
            Copy,
            Get_fn_ptr,
            Destroy,
        };

        class constexpr_function_base {
            using DoFn     = bool (*)(const constexpr_function_base&, constexpr_function_base&, constexpr_function_op);
            using TypeIdFn = const std::uint8_t* (*)();

          public:
            constexpr constexpr_function_base() noexcept = default;

            constexpr ~constexpr_function_base() noexcept {
            }

            inline constexpr bool is_empty() const noexcept {
                return !static_cast< bool >(data);
            }

            template < class Callable, class F >
            static constexpr void init(constexpr_function_base& In, F&& Fn) {
                constexpr_function_base::create< Callable >(In, std::forward< F >(Fn));
            }

            template < class Callable >
            static constexpr Callable* get_function_pointer(constexpr_function_base& In) noexcept {
                return get_constexpr_function_data_as< Callable >(In.data);
            }
            template < class Callable >
            static constexpr const Callable* get_function_pointer(const constexpr_function_base& In) noexcept {
                return get_constexpr_function_data_as< Callable >(In.data);
            }

            template < class Callable, class Fn >
            static constexpr void create(constexpr_function_base& In, Fn&& In_callable) {
                constexpr_function_base::finalize< Callable >(In);
                In.data = new constexpr_function_data_t< Callable > { new Callable(std::forward< Fn >(In_callable)) };
            }

            template < class Callable >
            static constexpr void destroy(constexpr_function_base& In) {
                finalize< Callable >(In);
            }

            template < class Callable >
            static constexpr void finalize(constexpr_function_base& In) {
                if (!In.is_empty()) {
                    delete static_cast< constexpr_function_data_t< Callable >* >(In.data);
                }
            }

            template < class Callable >
            static constexpr bool Do_op(const constexpr_function_base& In, constexpr_function_base& Output,
                                        constexpr_function_op Op) {
                switch (Op) {
                    case constexpr_function_op::Copy:
                        constexpr_function_base::create< Callable >(
                            Output, *get_constexpr_function_data_as< Callable >(In.data));
                        break;
                    case constexpr_function_op::Destroy:
                        constexpr_function_base::destroy< Callable >(Output);
                        break;
                    case constexpr_function_op::Get_fn_ptr:
                        Output.data = In.data;
                        break;
                    default:
                        return false;
                        break;
                }
                return true;
            }

            template < class Callable >
            static constexpr const std::uint8_t* GetTypeID() noexcept {
                return std::addressof(constexpr_function_data_t< Callable >::Type_id);
            }

            template < class F >
            static constexpr bool is_null_function(F* Fn) {
                return Fn == nullptr;
            }

            template < class Class, class F >
            static constexpr bool is_null_function(F Class::*Fn) {
                return Fn == nullptr;
            }

            template < class F >
            static constexpr bool is_null_function(const F& Fn) {
                (void)Fn;
                return false;
            }

            template < class F, std::enable_if_t< is_specialization_of_v< F, function >, int > = 0 >
            static constexpr bool is_null_function(const F& Fn) {
                return !static_cast< bool >(Fn);
            }

            constexpr_function_storage_t* data { nullptr };
            DoFn                          do_op { nullptr };
            TypeIdFn                      get_typeId { nullptr };
        };

        template < class Ret, class... Args >
        class constexpr_function_impl : public constexpr_function_base {
          private:
            using Base    = constexpr_function_base;
            using Call_fn = Ret (*)(const constexpr_function_base*, Args&&...);

          public:
            template < class T, class U >
            static constexpr bool is_valid_v =
                std::conjunction_v< std::negation< std::is_same< std::decay_t< T >, U > >,
                                    std::is_invocable_r< Ret, T, Args... > >;

            template < class Callable >
            using constexpr_func_impl_callable_t = std::decay_t< Callable >;

            using Res = Ret;

            template < class Callable >
            static constexpr Ret Do_call(const Base* In, Args&&... Types) {
                return std::invoke(*Base::get_function_pointer< Callable >(*In), std::forward< Args >(Types)...);
            }

            constexpr Ret operator()(Args... Types) const {
                return call(this, std::forward< Args >(Types)...);
            }

            Call_fn call { nullptr };
        };

        template < class NotFn >
        struct get_constexpr_function_base {
            static_assert(Eval_function_to_false< NotFn >,
                          "Incorrect behaviour: non-function type was passed to std::function!");
        };
        template < class Ret, class... Args >
        struct get_constexpr_function_base< Ret(Args...) > {
            using type = constexpr_function_impl< Ret, Args... >;
        };
        template < class Ret, class... Args >
        struct get_constexpr_function_base< Ret(Args...) noexcept > {
            using type = constexpr_function_impl< Ret, Args... >; // TODO: add noexcept
        };

        namespace {

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

        } // namespace

        template < class F, class = void >
        struct constexpr_function_deduce {}; // Invalid deduction
        template < class F >
        struct constexpr_function_deduce< F, std::void_t< decltype(&F::operator()) > >
            : get_memfun_ptr_deduction< decltype(&F::operator()) >::guide_type {};

    } // namespace detail

    template < class F >
    class function : public detail::get_constexpr_function_base< F >::type {
        using Base = typename detail::get_constexpr_function_base< F >::type;

      public:
        using result_type = typename Base::Res;

        constexpr function(std::nullptr_t) noexcept {};

        constexpr function(const function& Val) {
            if (static_cast< bool >(Val)) {
                Val.do_op(*static_cast< const detail::constexpr_function_base* >(std::addressof(Val)),
                          *static_cast< detail::constexpr_function_base* >(this), detail::constexpr_function_op::Copy);
                Base::do_op      = Val.do_op;
                Base::call       = Val.call;
                Base::get_typeId = Val.get_typeId;
            }
        }

        constexpr function(function&& Val) noexcept {
            Base::call       = Val.call;
            Base::do_op      = Val.do_op;
            Base::get_typeId = Val.get_typeId;
            Base::data       = std::move(Val.data);
        }

        template < class Callable, std::enable_if_t< Base::template is_valid_v< Callable, function >, int > = 0 >
        constexpr function(Callable&& Val) {
            static_assert(std::is_copy_constructible_v< std::decay_t< Callable > >,
                          "Callable must be copy-constructible");
            static_assert(std::is_constructible_v< std::decay_t< Callable >, Callable >,
                          "Callable must be constructible from itself");

            using Callable_t = typename Base::template constexpr_func_impl_callable_t< Callable >;

            if (!detail::constexpr_function_base::is_null_function< Callable_t >(Val)) {
                detail::constexpr_function_base::init< Callable_t >(
                    *static_cast< detail::constexpr_function_base* >(this), std::forward< Callable >(Val));
                Base::do_op      = detail::constexpr_function_base::template Do_op< Callable_t >;
                Base::call       = Base::template Do_call< Callable_t >;
                Base::get_typeId = Base::template GetTypeID< Callable_t >;
            }
        }

        constexpr ~function() {
            if (!Base::is_empty()) {
                Base::do_op(*this, *this, detail::constexpr_function_op::Destroy);
            }
        }

        constexpr function& operator=(const function& Val) {
            function(Val).swap(*this);
            return *this;
        }

        constexpr function& operator=(function&& Val) {
            function(std::move(Val)).swap(*this);
            return *this;
        }

        constexpr function& operator=(std::nullptr_t) noexcept {
            if (!Base::is_empty()) {
                Base::do_op(*static_cast< detail::constexpr_function_base* >(this),
                            *static_cast< detail::constexpr_function_base* >(this),
                            detail::constexpr_function_op::Destroy);
                Base::do_op      = nullptr;
                Base::call       = nullptr;
                Base::get_typeId = nullptr;
            }
            return *this;
        }

        template < class Callable, std::enable_if_t< Base::template is_valid_v< Callable, function >, int > = 0 >
        constexpr function& operator=(Callable&& Val) {
            function(std::forward< Callable >(Val)).swap(*this);
            return *this;
        }

        template < class Callable >
        constexpr function& operator=(std::reference_wrapper< Callable > Val) noexcept {
            function(Val).swap(*this);
            return *this;
        }

        constexpr void swap(function& Val) noexcept {
            std::swap(Base::data, Val.data);
            std::swap(Base::do_op, Val.do_op);
            std::swap(Base::get_typeId, Val.get_typeId);
            std::swap(Base::call, Val.call);
        }

        constexpr explicit operator bool() const noexcept {
            return !Base::is_empty();
        }

        template < typename Callable >
        constexpr Callable* target() noexcept {
            const function* C_function = this;
            const Callable* Fn         = C_function->template target< Callable >();
            return *const_cast< Callable** >(&Fn);
        }

        template < typename Callable >
        constexpr const Callable* target() const noexcept {
            if (Base::get_typeId() == std::addressof(detail::constexpr_function_data_t< Callable >::Type_id) &&
                !Base::is_empty()) {
                detail::constexpr_function_base Res {};
                Base::do_op(*static_cast< detail::constexpr_function_base* >(const_cast< function< F >* >(this)), Res,
                            detail::constexpr_function_op::Get_fn_ptr);
                return detail::get_constexpr_function_data_as< Callable >(
                    static_cast< const detail::constexpr_function_storage_t* >(Res.data));
            }
            return nullptr;
        }
    };

    // Deduction guide
    template < class Ret, class... Args >
    function(Ret (*)(Args...)) -> function< Ret(Args...) >;
    template < class F >
    function(F) -> function< typename detail::constexpr_function_deduce< F >::type >;

    template < class Ret, class... Args >
    void swap(function< Ret(Args...) >& lhs, function< Ret(Args...) >& rhs) noexcept {
        lhs.swap(rhs);
    }

    template < class Ret, class... Args >
    bool operator==(const function< Ret(Args...) >& F, std::nullptr_t) noexcept {
        return !static_cast< bool >(F);
    }

} // namespace mr

#endif // !defined(CONSTEXPR_FUNCTION_H_INCLUDED_06709B07_384C_42F9_9C94_E11CA87E041D)
