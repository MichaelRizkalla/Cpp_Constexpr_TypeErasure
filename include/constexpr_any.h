#if !defined(CONSTEXPR_ANY_H_INCLUDED_DB3AE22A_59A1_4B53_804D_0D0989C7B5FF)
    #define CONSTEXPR_ANY_H_INCLUDED_DB3AE22A_59A1_4B53_804D_0D0989C7B5FF

    #include <any>
    #include <cstddef>
    #include <memory>
    #include <new>
    #include <type_traits>

namespace mr {

    namespace detail {

        template < class >
        inline constexpr bool Eval_any_to_false = false;

        template < class Type, template < class... > class _Template >
        inline constexpr bool is_specialization_v = false;
        template < template < class... > class Template, class... Types >
        inline constexpr bool is_specialization_v< Template< Types... >, Template > = true;

        template < class Type, template < class... > class Template >
        struct is_specialization : std::bool_constant< is_specialization_v< Type, Template > > {};

        struct any_type_base {
            constexpr any_type_base() = default;
            constexpr virtual ~any_type_base() {
            }

            constexpr virtual any_type_base* do_copy() const       = 0;
            constexpr virtual void           do_destroy() noexcept = 0;
        };

        template < class T >
        struct [[nodiscard]] any_type : public any_type_base {
          public:
            using Base = any_type_base;

            template < class... Args >
            explicit constexpr any_type(Args&&... args) : data(std::forward< Args >(args)...) {
            }

            constexpr virtual ~any_type() {
            }

            [[nodiscard]] constexpr Base* do_copy() const override {
                static_assert(std::copy_constructible< T >);
                any_type* new_type = new any_type { data };
                return static_cast< Base* >(new_type);
            }

            [[nodiscard]] constexpr T* get_data() noexcept {
                return std::addressof(data);
            }

            constexpr void do_destroy() noexcept override {
                delete this;
            }

          private:
            T data;
        };

    } // namespace detail

    class [[nodiscard]] any {
      public:
        constexpr any() = default;

        constexpr any(const any& rhs) {
            if (std::addressof(rhs) != this) {
                if (std::is_constant_evaluated()) {
                    if (rhs.has_value()) {
                        data.ptr = rhs.data.ptr->do_copy();
                    }
                } else {
                    std::construct_at(std::addressof(get_std_any()), rhs.get_std_any());
                }
            }
        }

        constexpr any(any&& rhs) noexcept {
            if (std::addressof(rhs) != this) {
                move_from(std::move(rhs));
            }
        }

        template < class Type,
                   std::enable_if_t< std::conjunction_v< std::negation< std::is_same< std::decay_t< Type >, any > >,
                                                         std::negation< detail::is_specialization<
                                                             std::decay_t< Type >, std::in_place_type_t > >,
                                                         std::is_copy_constructible< std::decay_t< Type > > >,
                                     int > = 0 >
        constexpr any(Type&& value) {
            if (std::is_constant_evaluated()) {
                do_emplace< std::decay_t< Type > >(std::forward< Type >(value));
            } else {
                std::construct_at(std::addressof(get_std_any()), std::forward< Type >(value));
            }
        }

        template < class Type, class... Args,
                   std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >, Args... >,
                                                         std::is_copy_constructible< std::decay_t< Type > > >,
                                     int > = 0 >
        constexpr explicit any(std::in_place_type_t< Type >, Args&&... args) {
            if (std::is_constant_evaluated()) {
                do_emplace< std::decay_t< Type > >(std::forward< Args >(args)...);
            } else {
                std::construct_at(std::addressof(get_std_any()), std::in_place_type< Type >,
                                  std::forward< Args >(args)...);
            }
        }

        template <
            class Type, class Elem, class... Args,
            std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >,
                                                                         std::initializer_list< Elem >&, Args... >,
                                                  std::is_copy_constructible< std::decay_t< Type > > >,
                              int > = 0 >
        constexpr explicit any(std::in_place_type_t< Type >, std::initializer_list< Elem > il, Args&&... args) {
            if (std::is_constant_evaluated()) {
                do_emplace< std::decay_t< Type > >(il, std::forward< Args >(args)...);
            } else {
                std::construct_at(std::addressof(get_std_any()), std::in_place_type< Type >, il,
                                  std::forward< Args >(args)...);
            }
        }

        constexpr ~any() noexcept {
            reset();
        }

        constexpr any& operator=(const any& rhs) {
            if (std::addressof(rhs) != this) {
                *this = any { rhs };
            }
            return *this;
        }

        constexpr any& operator=(any&& rhs) noexcept {
            if (std::addressof(rhs) != this) {
                reset();
                move_from(std::move(rhs));
            }
            return *this;
        }

        template < class Type, class... Args,
                   std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >, Args... >,
                                                         std::is_copy_constructible< std::decay_t< Type > > >,
                                     int > = 0 >
        constexpr std::decay_t< Type >& emplace(Args&&... args) {
            reset();
            if (std::is_constant_evaluated()) {
                return do_emplace< std::decay_t< Type > >(std::forward< Args >(args)...);
            } else {
                return get_std_any().emplace< std::decay_t< Type > >(std::forward< Args >(args)...);
            }
        }

        template <
            class Type, class Elem, class... Args,
            std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >,
                                                                         std::initializer_list< Elem >&, Args... >,
                                                  std::is_copy_constructible< std::decay_t< Type > > >,
                              int > = 0 >
        constexpr std::decay_t< Type >& emplace(std::initializer_list< Elem > il, Args&&... args) {
            reset();
            if (std::is_constant_evaluated()) {
                return do_emplace< std::decay_t< Type > >(il, std::forward< Args >(args)...);
            } else {
                return get_std_any().emplace< std::decay_t< Type > >(il, std::forward< Args >(args)...);
            }
        }

        constexpr void reset() noexcept {
            if (std::is_constant_evaluated()) {
                if (data.ptr) {
                    data.ptr->do_destroy();
                    data.ptr = nullptr;
                }
            } else {
                get_std_any().reset();
            }
        }

        constexpr void swap(any& rhs) noexcept {
            rhs = std::exchange(*this, std::move(rhs));
        }

        [[nodiscard]] constexpr bool has_value() const noexcept {
            if (std::is_constant_evaluated()) {
                return static_cast< bool >(data.ptr);
            } else {
                return get_std_any().has_value();
            }
        }

        template < class T >
        [[nodiscard]] constexpr const T* cast_to() const noexcept {
            if (std::is_constant_evaluated()) {
                if (has_value()) {
                    return static_cast< detail::any_type< T >* >(data.ptr)->get_data();
                }
                return nullptr;
            } else {
                return std::any_cast< T >(std::addressof(get_std_any()));
            }
        }

        template < class T >
        [[nodiscard]] constexpr T* cast_to() noexcept {
            if (std::is_constant_evaluated()) {
                if (has_value()) {
                    return static_cast< detail::any_type< T >* >(data.ptr)->get_data();
                }
                return nullptr;
            } else {
                return std::any_cast< T >(std::addressof(get_std_any()));
            }
        }

        // non-constexpr as std::any is not constexpr
        [[nodiscard]] operator const std::any&() const noexcept {
            return get_std_any();
        }

        [[nodiscard]] operator std::any&() noexcept {
            return get_std_any();
        }

        [[nodiscard]] operator const std::any() const noexcept {
            return get_std_any();
        }

        [[nodiscard]] operator std::any() noexcept {
            return get_std_any();
        }

        [[nodiscard]] const std::type_info& type() const noexcept {
            return get_std_any().type();
        }

      private:
        template < class DType, class... Args >
        inline constexpr DType& do_emplace(Args&&... args) {
            detail::any_type< DType >* type = new detail::any_type< DType >(std::forward< Args >(args)...);
            data.ptr                        = type;
            return *(type->get_data());
        }

        inline constexpr void move_from(any&& val) noexcept {
            if (std::is_constant_evaluated()) {
                data.ptr = std::exchange(val.data.ptr, nullptr);
            } else {
                get_std_any() = std::move(val.data.any);
            }
        }

        [[nodiscard]] inline const std::any& get_std_any() const noexcept {
            return *reinterpret_cast< const std::any* >(&data.any);
        }

        [[nodiscard]] inline std::any& get_std_any() noexcept {
            return *reinterpret_cast< std::any* >(&data.any);
        }

        union Data {
            std::max_align_t       align;
            detail::any_type_base* ptr;
            char                   any[sizeof(std::any)];

            constexpr Data() {
                if (std::is_constant_evaluated()) {
                    ptr = nullptr;
                } else {
                    std::construct_at(reinterpret_cast< std::any* >(&any));
                }
            }

            constexpr ~Data() {
            }
        };

        Data data {};
    };

    inline constexpr void swap(any& lhs, any& rhs) noexcept {
        lhs.swap(rhs);
    }

    template < class T, class... Args >
    [[nodiscard]] constexpr any make_any(Args&&... args) {
        return any { std::in_place_type< T >, std::forward< Args >(args)... };
    }

    template < class T, class Elem, class... Args >
    [[nodiscard]] constexpr any make_any(std::initializer_list< Elem > il, Args&&... args) {
        return any { std::in_place_type< T >, il, std::forward< Args >(args)... };
    }

    template < class T >
    [[nodiscard]] constexpr const T* any_cast(const any* const value) noexcept {
        static_assert(!std::is_void_v< T >, "mr::any cannot contain void type");

        if constexpr (std::is_function_v< T > || std::is_array_v< T >) {
            return nullptr;
        } else {
            if (!value) {
                return nullptr;
            }

            return value->cast_to< std::remove_cvref_t< T > >();
        }
    }

    template < class T >
    [[nodiscard]] constexpr T* any_cast(any* const value) noexcept {
        static_assert(!std::is_void_v< T >, "mr::any cannot contain void type");

        if constexpr (std::is_function_v< T > || std::is_array_v< T >) {
            return nullptr;
        } else {
            if (!value) {
                return nullptr;
            }

            return value->cast_to< std::remove_cvref_t< T > >();
        }
    }

    template < class T >
    [[nodiscard]] constexpr std::remove_cv_t< T > any_cast(const any& value) {
        static_assert(std::is_constructible_v< std::remove_cv_t< T >, const std::remove_cvref_t< T >& >,
                      "any_cast<T>(const any&) requires std::remove_cv_t<T> to be constructible from "
                      "const std::remove_cv_t<std::remove_reference_t<T>>&");

        const auto ptr = any_cast< std::remove_cvref_t< T > >(&value);
        if (!ptr) {
            throw std::bad_any_cast {};
        }

        return static_cast< std::remove_cv_t< T > >(*ptr);
    }

    template < class T >
    [[nodiscard]] constexpr std::remove_cv_t< T > any_cast(any& value) {
        static_assert(std::is_constructible_v< std::remove_cv_t< T >, std::remove_cvref_t< T >& >,
                      "any_cast<T>(any&) requires std::remove_cv_t<T> to be constructible from "
                      "std::remove_cv_t<std::remove_reference_t<T>>&");

        const auto ptr = any_cast< std::remove_cvref_t< T > >(&value);
        if (!ptr) {
            throw std::bad_any_cast {};
        }

        return static_cast< std::remove_cv_t< T > >(*ptr);
    }

    template < class T >
    [[nodiscard]] constexpr std::remove_cv_t< T > any_cast(any&& value) {
        static_assert(std::is_constructible_v< std::remove_cv_t< T >, std::remove_cvref_t< T > >,
                      "any_cast<T>(any&&) requires std::remove_cv_t<T> to be constructible from "
                      "std::remove_cv_t<std::remove_reference_t<T>>");

        const auto ptr = any_cast< std::remove_cvref_t< T > >(&value);
        if (!ptr) {
            throw std::bad_any_cast {};
        }

        return static_cast< std::remove_cv_t< T > >(std::move(*ptr));
    }

} // namespace mr

#endif // !defined(CONSTEXPR_ANY_H_INCLUDED_DB3AE22A_59A1_4B53_804D_0D0989C7B5FF)
