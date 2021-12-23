#if !defined(CONSTEXPR_ANY_H_INCLUDED_DB3AE22A_59A1_4B53_804D_0D0989C7B5FF)
    #define CONSTEXPR_ANY_H_INCLUDED_DB3AE22A_59A1_4B53_804D_0D0989C7B5FF

    #include <any>
    #include <concepts>
    #include <memory>
    #include <new>

namespace mr {

    namespace detail {

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
        struct any_type : public any_type_base {
          public:
            using Base = any_type_base;

            template < class... Args >
            explicit constexpr any_type(Args&&... args) : data(std::forward< Args >(args)...) {
            }

            constexpr virtual ~any_type() {
            }

            constexpr Base* do_copy() const override {
                static_assert(std::copy_constructible< T >);
                any_type* new_type = new any_type { data };
                return static_cast< Base* >(new_type);
            }

            constexpr void do_destroy() noexcept override {
                delete this;
            }

            constexpr T* get_data() noexcept {
                return &data;
            }

          private:
            T data;
        };

    } // namespace detail

    class constexpr_any {
      public:
        constexpr constexpr_any() = default;

        constexpr constexpr_any(const constexpr_any& rhs) {
            ptr = rhs.ptr->do_copy();
        }

        constexpr constexpr_any(constexpr_any&& rhs) noexcept {
            ptr = std::exchange(rhs.ptr, nullptr);
        }

        template < class Type,
                   std::enable_if_t<
                       std::conjunction_v<
                           std::negation< std::is_same< std::decay_t< Type >, constexpr_any > >,
                           std::negation< detail::is_specialization< std::decay_t< Type >, std::in_place_type_t > >,
                           std::is_copy_constructible< std::decay_t< Type > > >,
                       int > = 0 >
        constexpr constexpr_any(Type&& value) {
            do_emplace< std::decay_t< Type > >(std::forward< Type >(value));
        }

        template < class Type, class... Args,
                   std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >, Args... >,
                                                         std::is_copy_constructible< std::decay_t< Type > > >,
                                     int > = 0 >
        constexpr explicit constexpr_any(std::in_place_type_t< Type >, Args&&... args) {
            do_emplace< std::decay_t< Type > >(std::forward< Args >(args)...);
        }

        template <
            class Type, class Elem, class... Args,
            std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >,
                                                                         std::initializer_list< Elem >&, Args... >,
                                                  std::is_copy_constructible< std::decay_t< Type > > >,
                              int > = 0 >
        constexpr explicit constexpr_any(std::in_place_type_t< Type >, std::initializer_list< Elem > il,
                                         Args&&... args) {
            do_emplace< std::decay_t< Type > >(il, std::forward< Args >(args)...);
        }

        constexpr ~constexpr_any() noexcept {
            reset();
        }

        constexpr constexpr_any& operator=(const constexpr_any& rhs) {
            *this = constexpr_any { rhs };
            return *this;
        }

        constexpr constexpr_any& operator=(constexpr_any&& rhs) noexcept {
            reset();
            ptr = std::exchange(rhs.ptr, nullptr);
            return *this;
        }

        template < class Type, class... Args,
                   std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >, Args... >,
                                                         std::is_copy_constructible< std::decay_t< Type > > >,
                                     int > = 0 >
        constexpr std::decay_t< Type >& emplace(Args&&... args) {
            reset();
            return do_emplace< std::decay_t< Type > >(std::forward< Args >(args)...);
        }

        template <
            class Type, class Elem, class... Args,
            std::enable_if_t< std::conjunction_v< std::is_constructible< std::decay_t< Type >,
                                                                         std::initializer_list< Elem >&, Args... >,
                                                  std::is_copy_constructible< std::decay_t< Type > > >,
                              int > = 0 >
        constexpr std::decay_t< Type >& emplace(std::initializer_list< Elem > il, Args&&... args) {
            reset();
            return do_emplace< std::decay_t< Type > >(il, std::forward< Args >(args)...);
        }

        constexpr void reset() {
            if (ptr) {
                ptr->do_destroy();
                ptr = nullptr;
            }
        }

        constexpr void swap(constexpr_any& rhs) noexcept {
            rhs = std::exchange(*this, std::move(rhs));
        }

        constexpr bool has_value() const noexcept {
            return static_cast< bool >(ptr);
        }

        template < class T >
        constexpr const T* cast() noexcept {
            return static_cast< detail::any_type< T >* >(ptr)->get_data();
        }

      private:
        template < class DType, class... Args >
        constexpr DType& do_emplace(Args&&... args) {
            detail::any_type< DType >* type = new detail::any_type< DType >(std::forward< Args >(args)...);
            ptr                             = type;
            return *(type->get_data());
        }

        detail::any_type_base* ptr { nullptr };
    };

} // namespace mr

#endif // !defined(CONSTEXPR_ANY_H_INCLUDED_DB3AE22A_59A1_4B53_804D_0D0989C7B5FF)
