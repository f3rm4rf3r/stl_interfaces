// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_STL_INTERFACES_ITERATOR_INTERFACE_HPP
#define BOOST_STL_INTERFACES_ITERATOR_INTERFACE_HPP

#include <boost/stl_interfaces/fwd.hpp>

#include <utility>
#include <type_traits>
#if 201711L <= __cpp_lib_three_way_comparison
#include <compare>
#endif


namespace boost { namespace stl_interfaces {

    /** A type for granting access to the private members of an iterator
        derived from `iterator_interface`. */
    struct access
    {
#ifndef BOOST_STL_INTERFACES_DOXYGEN

        template<typename Derived>
        static constexpr auto base(
            Derived & d) noexcept -> decltype(d.base_reference())
        {
            return d.base_reference();
        }
        template<typename Derived>
        static constexpr auto base(
            Derived const & d) noexcept -> decltype(d.base_reference())
        {
            return d.base_reference();
        }

#endif
    };

    /** The return type of `operator->()` in a proxy iterator.

        This template is used as the default `Pointer` template parameter in
        the `proxy_iterator_interface` template alias.  Note that the use of
        this template implies a copy or move of the underlying object of type
        `T`. */
    template<typename T>
    struct proxy_arrow_result
    {
        constexpr proxy_arrow_result(T const & value) noexcept(
            noexcept(T(value))) :
            value_(value)
        {}
        constexpr proxy_arrow_result(T && value) noexcept(
            noexcept(T(std::move(value)))) :
            value_(std::move(value))
        {}

        constexpr T const * operator->() const noexcept { return &value_; }
        constexpr T * operator->() noexcept { return &value_; }

    private:
        T value_;
    };

    namespace detail {
        template<typename Pointer, typename T>
        struct ptr_and_ref : std::is_same<
                                 std::add_pointer_t<std::remove_reference_t<T>>,
                                 Pointer>
        {
        };

        template<typename Pointer, typename T>
        auto make_pointer(
            T && value,
            std::enable_if_t<ptr_and_ref<Pointer, T>::value, int> = 0)
        {
            return std::addressof(value);
        }

        template<typename Pointer, typename T>
        auto make_pointer(
            T && value,
            std::enable_if_t<!ptr_and_ref<Pointer, T>::value, int> = 0)
        {
            return Pointer((T &&) value);
        }

        template<typename IteratorConcept>
        struct concept_category
        {
            using type = IteratorConcept;
        };
#if 201703L < __cplusplus && defined(__cpp_lib_ranges)
        template<>
        struct concept_category<std::contiguous_iterator_tag>
        {
            using type = std::random_access_iterator_tag;
        };
#elif 201703L <= __cplusplus && __has_include(<stl2/ranges.hpp>) && \
    !defined(BOOST_STL_INTERFACES_DISABLE_CMCSTL2)
        template<>
        struct concept_category<v2::ranges::contiguous_iterator_tag>
        {
            using type = std::random_access_iterator_tag;
        };
#endif
        template<typename IteratorConcept>
        using concept_category_t =
            typename concept_category<IteratorConcept>::type;

        template<typename Pointer, typename IteratorConcept>
        struct pointer
        {
            using type = Pointer;
        };
        template<typename Pointer>
        struct pointer<Pointer, std::output_iterator_tag>
        {
            using type = void;
        };
        template<typename Pointer, typename IteratorConcept>
        using pointer_t = typename pointer<Pointer, IteratorConcept>::type;
    }

}}

namespace boost { namespace stl_interfaces { inline namespace v1 {

    /** A CRTP template that one may derive from to make defining iterators
        easier. */
    template<
        typename Derived,
        typename IteratorConcept,
        typename ValueType,
        typename Reference = ValueType &,
        typename Pointer = ValueType *,
        typename DifferenceType = std::ptrdiff_t
#ifndef BOOST_STL_INTERFACES_DOXYGEN
        ,
        typename E = std::enable_if_t<
            std::is_class<Derived>::value &&
            std::is_same<Derived, std::remove_cv_t<Derived>>::value>
#endif
        >
    struct iterator_interface;

    namespace v1_dtl {
        template<typename Iterator, typename = void>
        struct ra_iter : std::false_type
        {
        };
        template<typename Iterator>
        struct ra_iter<Iterator, void_t<typename Iterator::iterator_concept>>
            : std::integral_constant<
                  bool,
                  std::is_base_of<
                      std::random_access_iterator_tag,
                      typename Iterator::iterator_concept>::value>
        {
        };

        template<typename Iterator, typename DifferenceType, typename = void>
        struct plus_eq : std::false_type
        {
        };
        template<typename Iterator, typename DifferenceType>
        struct plus_eq<
            Iterator,
            DifferenceType,
            void_t<decltype(
                std::declval<Iterator &>() += std::declval<DifferenceType>())>>
            : std::true_type
        {
        };

        template<
            typename Derived,
            typename IteratorConcept,
            typename ValueType,
            typename Reference,
            typename Pointer,
            typename DifferenceType>
        void derived_iterator(iterator_interface<
                              Derived,
                              IteratorConcept,
                              ValueType,
                              Reference,
                              Pointer,
                              DifferenceType> const &);
    }

    template<
        typename Derived,
        typename IteratorConcept,
        typename ValueType,
        typename Reference,
        typename Pointer,
        typename DifferenceType
#ifndef BOOST_STL_INTERFACES_DOXYGEN
        ,
        typename E
#endif
        >
    struct iterator_interface
    {
#ifndef BOOST_STL_INTERFACES_DOXYGEN
    private:
        constexpr Derived & derived() noexcept
        {
            return static_cast<Derived &>(*this);
        }
        constexpr Derived const & derived() const noexcept
        {
            return static_cast<Derived const &>(*this);
        }
#endif

    public:
        using iterator_concept = IteratorConcept;
        using iterator_category = detail::concept_category_t<iterator_concept>;
        using value_type = ValueType;
        using reference = Reference;
        using pointer = detail::pointer_t<Pointer, iterator_concept>;
        using difference_type = DifferenceType;

        template<typename D = Derived>
        constexpr auto operator*() const
            noexcept(noexcept(*access::base(std::declval<D const &>())))
                -> decltype(*access::base(std::declval<D const &>()))
        {
            return *access::base(derived());
        }

        template<typename D = Derived>
        constexpr pointer operator->() const noexcept(
            noexcept(detail::make_pointer<pointer>(*std::declval<D const &>())))
        {
            return detail::make_pointer<pointer>(*derived());
        }

        template<typename D = Derived>
        constexpr auto operator[](difference_type i) const noexcept(noexcept(
            D(std::declval<D const &>()),
            std::declval<D &>() += i,
            *std::declval<D &>()))
            -> decltype(std::declval<D &>() += i, *std::declval<D &>())
        {
            D retval = derived();
            retval += i;
            return *retval;
        }

        template<
            typename D = Derived,
            typename Enable =
                std::enable_if_t<!v1_dtl::plus_eq<D, difference_type>::value>>
        constexpr auto
        operator++() noexcept(noexcept(++access::base(std::declval<D &>())))
            -> decltype(++access::base(std::declval<D &>()))
        {
            return ++access::base(derived());
        }

        template<typename D = Derived>
        constexpr auto operator++() noexcept(
            noexcept(std::declval<D &>() += difference_type(1)))
            -> decltype(
                std::declval<D &>() += difference_type(1), std::declval<D &>())
        {
            derived() += difference_type(1);
            return derived();
        }
        template<typename D = Derived>
        constexpr auto operator++(int)noexcept(
            noexcept(D(std::declval<D &>()), ++std::declval<D &>()))
            -> std::remove_reference_t<decltype(
                D(std::declval<D &>()),
                ++std::declval<D &>(),
                std::declval<D &>())>
        {
            D retval = derived();
            ++derived();
            return retval;
        }

        template<typename D = Derived>
        constexpr auto operator+=(difference_type n) noexcept(
            noexcept(access::base(std::declval<D &>()) += n))
            -> decltype(access::base(std::declval<D &>()) += n)
        {
            return access::base(derived()) += n;
        }

        template<typename D = Derived>
        constexpr D operator+(difference_type i) noexcept(
            noexcept(D(std::declval<D &>()), std::declval<D &>() += i))
        {
            D retval = derived();
            retval += i;
            return retval;
        }
        friend BOOST_STL_INTERFACES_HIDDEN_FRIEND_CONSTEXPR Derived
        operator+(difference_type i, Derived it) noexcept(noexcept(it + i))
        {
            return it + i;
        }

        template<
            typename D = Derived,
            typename Enable =
                std::enable_if_t<!v1_dtl::plus_eq<D, difference_type>::value>>
        constexpr auto
        operator--() noexcept(noexcept(--access::base(std::declval<D &>())))
            -> decltype(--access::base(std::declval<D &>()))
        {
            return --access::base(derived());
        }

        template<typename D = Derived>
        constexpr auto operator--() noexcept(noexcept(
            D(std::declval<D &>()), std::declval<D &>() += -difference_type(1)))
            -> decltype(
                std::declval<D &>() += -difference_type(1), std::declval<D &>())
        {
            derived() += -difference_type(1);
            return derived();
        }
        template<typename D = Derived>
        constexpr auto operator--(int)noexcept(
            noexcept(D(std::declval<D &>()), --std::declval<D &>()))
            -> std::remove_reference_t<decltype(
                D(std::declval<D &>()),
                --std::declval<D &>(),
                std::declval<D &>())>
        {
            D retval = derived();
            --derived();
            return retval;
        }

        template<typename D = Derived>
        constexpr D & operator-=(difference_type i) noexcept(
            noexcept(std::declval<D &>() += -i))
        {
            derived() += -i;
            return derived();
        }

        template<typename D = Derived>
        constexpr auto operator-(D other) const noexcept(noexcept(
            access::base(std::declval<D const &>()) - access::base(other)))
            -> decltype(
                access::base(std::declval<D const &>()) - access::base(other))
        {
            return access::base(derived()) - access::base(other);
        }

        friend BOOST_STL_INTERFACES_HIDDEN_FRIEND_CONSTEXPR Derived operator-(
            Derived it,
            difference_type i) noexcept(noexcept(Derived(it), it += -i))
        {
            Derived retval = it;
            retval += -i;
            return retval;
        }
    };

    /** Implementation of `operator!=()` for all iterators derived from
        `iterator_interface`, except those with an iterator category derived
        from `std::random_access_iterator_tag`.  */
    template<
        typename IteratorInterface,
        typename Enable =
            std::enable_if_t<!v1_dtl::ra_iter<IteratorInterface>::value>>
    constexpr auto operator!=(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs == rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs == rhs)
    {
        return !(lhs == rhs);
    }

    /** Implementation of `operator==()`, implemented in terms of the iterator
        underlying IteratorInterface, for all iterators derived from
        `iterator_interface`, except those with an iterator category derived
        from `std::random_access_iterator_tag`.  */
    template<
        typename IteratorInterface,
        typename Enable = std::enable_if_t<!v1_dtl::plus_eq<
            IteratorInterface,
            typename IteratorInterface::difference_type>::value>>
    constexpr auto
    operator==(IteratorInterface lhs, IteratorInterface rhs) noexcept(noexcept(
        access::base(std::declval<IteratorInterface &>()) ==
        access::base(std::declval<IteratorInterface &>())))
        -> decltype(
            access::base(std::declval<IteratorInterface &>()) ==
            access::base(std::declval<IteratorInterface &>()))
    {
        return access::base(lhs) == access::base(rhs);
    }


    /** Implementation of `operator==()` for all iterators derived from
        `iterator_interface` that have an iterator category derived from
        `std::random_access_iterator_tag`.  */
    template<typename IteratorInterface>
    constexpr auto operator==(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs - rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs - rhs)
    {
        return (lhs - rhs) == 0;
    }

    /** Implementation of `operator!=()` for all iterators derived from
        `iterator_interface` that have an iterator category derived from
        `std::random_access_iterator_tag`.  */
    template<
        typename IteratorInterface,
        typename Enable =
            std::enable_if_t<v1_dtl::ra_iter<IteratorInterface>::value>>
    constexpr auto operator!=(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs - rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs - rhs)
    {
        return (lhs - rhs) != 0;
    }

    /** Implementation of `operator<()` for all iterators derived from
        `iterator_interface` that have an iterator category derived from
        `std::random_access_iterator_tag`.  */
    template<typename IteratorInterface>
    constexpr auto operator<(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs - rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs - rhs)
    {
        return (lhs - rhs) < 0;
    }

    /** Implementation of `operator<=()` for all iterators derived from
        `iterator_interface` that have an iterator category derived from
        `std::random_access_iterator_tag`.  */
    template<typename IteratorInterface>
    constexpr auto operator<=(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs - rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs - rhs)
    {
        return (lhs - rhs) <= 0;
    }

    /** Implementation of `operator>()` for all iterators derived from
        `iterator_interface` that have an iterator category derived from
        `std::random_access_iterator_tag`.  */
    template<typename IteratorInterface>
    constexpr auto operator>(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs - rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs - rhs)
    {
        return (lhs - rhs) > 0;
    }

    /** Implementation of `operator>=()` for all iterators derived from
        `iterator_interface` that have an iterator category derived from
        `std::random_access_iterator_tag`.  */
    template<typename IteratorInterface>
    constexpr auto operator>=(
        IteratorInterface lhs,
        IteratorInterface rhs) noexcept(noexcept(lhs - rhs))
        -> decltype(v1_dtl::derived_iterator(lhs), lhs - rhs)
    {
        return (lhs - rhs) >= 0;
    }


    /** A template alias useful for defining proxy iterators.  \see
        `iterator_interface`. */
    template<
        typename Derived,
        typename IteratorConcept,
        typename ValueType,
        typename Reference = ValueType,
        typename DifferenceType = std::ptrdiff_t>
    using proxy_iterator_interface = iterator_interface<
        Derived,
        IteratorConcept,
        ValueType,
        Reference,
        proxy_arrow_result<Reference>,
        DifferenceType>;

}}}


namespace boost { namespace stl_interfaces { namespace v2 {

    // This is only here to satisfy clag-format.
    namespace v2_dtl {
    }

#if 201703L < __cplusplus && defined(__cpp_lib_concepts) || BOOST_STL_INTERFACES_DOXYGEN

    // clang-format off
    /** A CRTP template that one may derive from to make defining iterators
        easier. */
    template<
      typename Derived,
      typename IteratorConcept,
      typename ValueType,
      typename Reference = ValueType &,
      typename Pointer = ValueType *,
      typename DifferenceType = std::ptrdiff_t>
      requires std::is_class_v<Derived> &&
               std::same_as<Derived, std::remove_cv_t<Derived>>
    struct iterator_interface {
    private:
      constexpr Derived& derived() noexcept {
        return static_cast<Derived&>(*this);
      }
      constexpr const Derived& derived() const noexcept {
        return static_cast<const Derived&>(*this);
      }

    public:
      using iterator_concept = IteratorConcept;
      using iterator_category = detail::concept_category_t<iterator_concept>;
      using value_type = ValueType;
      using reference = Reference;
      using pointer = detail::pointer_t<Pointer, iterator_concept>;
      using difference_type = DifferenceType;

      constexpr decltype(auto) operator*()
        requires requires { *access::base(derived()); } {
          return *access::base(derived());
        }
      constexpr decltype(auto) operator*() const
        requires requires { *access::base(derived()); } {
          return *access::base(derived());
        }

      constexpr auto operator->()
        requires requires { *derived(); } {
          return detail::make_pointer<pointer>(*derived());
        }
      constexpr auto operator->() const
        requires requires { *derived(); } {
          return detail::make_pointer<pointer>(*derived());
        }

      constexpr decltype(auto) operator[](difference_type n) const
        requires requires { derived() += n; } {
        Derived retval = derived();
        retval += n;
        return *retval;
      }

      constexpr decltype(auto) operator++()
        requires requires { ++access::base(derived()); } &&
          !requires { derived() += difference_type(1); } {
            ++access::base(derived());
            return derived();
          }
      constexpr decltype(auto) operator++()
        requires requires { derived() += difference_type(1); } {
          return derived() += difference_type(1);
        }
      constexpr auto operator++(int) requires requires { ++derived(); } {
        Derived retval = derived();
        ++derived();
        return retval;
      }
      constexpr decltype(auto) operator+=(difference_type n)
        requires requires { access::base(derived()) += n; } {
          access::base(derived()) += n;
          return derived();
        }
      friend constexpr auto operator+(Derived it, difference_type n)
        requires requires { it += n; } {
          return it += n;
        }
      friend constexpr auto operator+(difference_type n, Derived it)
        requires requires { it += n; } {
          return it ++ n;
        }

      constexpr decltype(auto) operator--()
        requires requires { --access::base(derived()); } &&
          !requires { derived() += difference_type(1); } {
          --access::base(derived());
          return derived();
        }
      constexpr decltype(auto) operator--()
        requires requires { derived() += -difference_type(1); } {
          return derived() += -difference_type(1);
        }
      constexpr auto operator--(int) requires requires { --derived(); } {
        Derived retval = derived();
        --derived();
        return retval;
      }
      constexpr decltype(auto) operator-=(difference_type n)
        requires requires { derived() += -n; } {
          return derived() += -n;
        }
      friend constexpr auto operator-(Derived lhs, Derived rhs)
        requires requires { access::base(lhs) - access::base(rhs); } {
          return access::base(lhs) - access::base(rhs);
        }
      friend constexpr auto operator-(Derived it, difference_type n)
        requires requires { it += -n; } {
          return it += -n;
        }

      friend constexpr std::strong_equality operator<=>(Derived lhs, Derived rhs)
        requires requires { access::base(lhs) == access::base(rhs); } &&
          !requires { access::base(lhs) <=> access::base(rhs); } &&
          !requires { lhs - rhs; } {
            return access::base(lhs) == access::base(rhs) ?
                std::strong_equality::equal : std::strong_equality::unequal;
          }
      friend constexpr std::strong_ordering operator<=>(Derived lhs, Derived rhs)
        requires requires { access::base(lhs) <=> access::base(rhs); } ||
          requires { lhs - rhs; } {
            if constexpr (requires { access::base(lhs) <=> access::base(rhs); }) {
              return access::base(lhs) <=> access::base(rhs);
            } else {
              auto delta = lhs - rhs;
              if (delta < 0)
                  return std::strong_ordering::less;
              if (0 < delta)
                  return std::strong_ordering::greater;
              return  std::strong_ordering::equal;
            }
          }
    };

#elif 201703L <= __cplusplus && __has_include(<stl2/ranges.hpp>) && \
    !defined(BOOST_STL_INTERFACES_DISABLE_CMCSTL2)

    namespace v1_dt2 {
        // These named concepts are used to work around
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=82740
        template<typename Derived>
        concept bool base_incr =
            requires (Derived & d) { ++access::base(d); };

        template<typename Derived>
        concept bool base_deref =
            requires (Derived & d) { *access::base(d); };

        template<typename Derived>
        concept bool deref = requires (Derived & d) { *d; };

        template<typename Derived>
        concept bool plus_eq =
            requires (Derived & d) { d += typename Derived::difference_type(1); };

        template<typename Derived>
        concept bool base_plus_eq =
            requires (Derived & d) {
                access::base(d) += typename Derived::difference_type(1); };

        template<typename Derived>
        concept bool incr = requires (Derived & d) { ++d; };

        template<typename Derived>
        concept bool base_decr =
            requires (Derived & d) { --access::base(d); };

        template<typename Derived>
        concept bool decr = requires (Derived & d) { --d; };

        template<typename Derived>
        concept bool base_sub =
            requires (Derived & d) { access::base(d) - access::base(d); };

        template<typename Derived>
        concept bool base_eq =
            requires (Derived & d) { access::base(d) == access::base(d); };

        template<typename Derived>
        concept bool sub = requires (Derived & d) { d - d; };

        template<typename Derived>
        concept bool eq = requires (Derived & d) { d == d; };
    }

    /** A CRTP template that one may derive from to make defining iterators
        easier. */
    template<
      typename Derived,
      typename IteratorConcept,
      typename ValueType,
      typename Reference = ValueType &,
      typename Pointer = ValueType *,
      typename DifferenceType = std::ptrdiff_t>
      requires std::is_class_v<Derived> &&
               ranges::same_as<Derived, std::remove_cv_t<Derived>>
    struct iterator_interface {
    private:
      constexpr Derived& derived() noexcept {
        return static_cast<Derived&>(*this);
      }
      constexpr const Derived& derived() const noexcept {
        return static_cast<const Derived&>(*this);
      }

    public:
      using iterator_concept = IteratorConcept;
      using iterator_category = detail::concept_category_t<iterator_concept>;
      using value_type = ValueType;
      using reference = Reference;
      using pointer = detail::pointer_t<Pointer, iterator_concept>;
      using difference_type = DifferenceType;

      constexpr decltype(auto) operator*()
        requires v1_dt2::base_deref<Derived> {
          return *access::base(derived());
        }
      constexpr decltype(auto) operator*() const
        requires v1_dt2::base_deref<const Derived> {
          return *access::base(derived());
        }

      constexpr auto operator->()
        requires v1_dt2::deref<Derived> {
          return detail::make_pointer<pointer>(*derived());
        }
      constexpr auto operator->() const
        requires v1_dt2::deref<Derived> {
          return detail::make_pointer<pointer>(*derived());
        }

      constexpr decltype(auto) operator[](difference_type n) const
        requires v1_dt2::plus_eq<Derived> {
          Derived retval = derived();
          retval += n;
          return *retval;
        }

      constexpr decltype(auto) operator++()
        requires v1_dt2::base_incr<Derived> && !v1_dt2::plus_eq<Derived> {
          ++access::base(derived());
          return derived();
        }
      constexpr decltype(auto) operator++()
        requires v1_dt2::plus_eq<Derived> {
          return derived() += difference_type(1);
        }
      constexpr auto operator++(int) requires v1_dt2::incr<Derived> {
        Derived retval = derived();
        ++derived();
        return retval;
      }
      constexpr decltype(auto) operator+=(difference_type n)
        requires v1_dt2::base_plus_eq<Derived> {
          access::base(derived()) += n;
          return derived();
        }
      friend constexpr auto operator+(Derived it, difference_type n)
        requires v1_dt2::plus_eq<Derived> {
          return it += n;
        }
      friend constexpr auto operator+(difference_type n, Derived it)
        requires v1_dt2::plus_eq<Derived> {
          return it += n;
        }

      constexpr decltype(auto) operator--()
        requires v1_dt2::base_decr<Derived> && !v1_dt2::plus_eq<Derived> {
          --access::base(derived());
          return derived();
        }
      constexpr decltype(auto) operator--()
        requires v1_dt2::plus_eq<Derived> {
          return derived() += -difference_type(1);
        }
      constexpr auto operator--(int) requires v1_dt2::decr<Derived> {
        Derived retval = derived();
        --derived();
        return retval;
      }
      constexpr decltype(auto) operator-=(difference_type n)
        requires v1_dt2::plus_eq<Derived> {
          return derived() += -n;
        }
      friend constexpr auto operator-(Derived lhs, Derived rhs)
        requires v1_dt2::base_sub<Derived> {
          return access::base(lhs) - access::base(rhs);
        }
      friend constexpr auto operator-(Derived it, difference_type n)
        requires v1_dt2::plus_eq<Derived> {
          return it += -n;
        }

#if 201711L <= __cpp_lib_three_way_comparison
      friend constexpr std::strong_equality operator<=>(Derived lhs, Derived rhs)
        requires requires { access::base(lhs) == access::base(rhs); } &&
          !requires { access::base(lhs) == access::base(rhs); } &&
          !requires { lhs - rhs; } {
            return access::base(lhs) == access::base(rhs) ?
                std::strong_equality::equal : std::strong_equality::unequal;
          }
      friend constexpr std::strong_ordering operator<=>(Derived lhs, Derived rhs)
        requires requires { access::base(lhs) <=> access::base(rhs); } ||
          requires { lhs - rhs; } {
            if constexpr (requires { access::base(lhs) <=> access::base(rhs); }) {
              return access::base(lhs) <=> access::base(rhs);
            } else {
              auto delta = lhs - rhs;
              if (delta < 0)
                  return std::strong_ordering::less;
              if (0 < delta)
                  return std::strong_ordering::greater;
              return  std::strong_ordering::equal;
            }
          }
#else
      friend constexpr bool operator==(Derived lhs, Derived rhs)
        requires v1_dt2::base_eq<Derived> || v1_dt2::sub<Derived> {
          if constexpr (v1_dt2::base_eq<Derived>) {
            return (access::base(lhs) == access::base(rhs));
          } else if constexpr (v1_dt2::sub<Derived>) {
            return (lhs - rhs) == typename Derived::difference_type(0);
          }
        }
      friend constexpr bool operator!=(Derived lhs, Derived rhs)
        requires v1_dt2::base_eq<Derived> || v1_dt2::eq<Derived> || v1_dt2::sub<Derived> {
          if constexpr (v1_dt2::base_eq<Derived>) {
            return !(access::base(lhs) == access::base(rhs));
          } else if constexpr (v1_dt2::eq<Derived>) {
            return !(lhs == rhs);
          } else if constexpr (v1_dt2::sub<Derived>) {
            return (lhs - rhs) != typename Derived::difference_type(0);
          }
        }
      friend constexpr bool operator<(Derived lhs, Derived rhs)
        requires v1_dt2::eq<Derived> {
          return (lhs - rhs) < typename Derived::difference_type(0);
        }
      friend constexpr bool operator<=(Derived lhs, Derived rhs)
        requires v1_dt2::eq<Derived> {
          return (lhs - rhs) <= typename Derived::difference_type(0);
        }
      friend constexpr bool operator>(Derived lhs, Derived rhs)
        requires v1_dt2::eq<Derived> {
          return (lhs - rhs) > typename Derived::difference_type(0);
        }
      friend constexpr bool operator>=(Derived lhs, Derived rhs)
        requires v1_dt2::eq<Derived> {
          return (lhs - rhs) >= typename Derived::difference_type(0);
        }
#endif
    };

    /** A template alias useful for defining proxy iterators.  \see
        `iterator_interface`. */
    template<
        typename Derived,
        typename IteratorConcept,
        typename ValueType,
        typename Reference = ValueType,
        typename DifferenceType = std::ptrdiff_t>
    using proxy_iterator_interface = iterator_interface<
        Derived,
        IteratorConcept,
        ValueType,
        Reference,
        proxy_arrow_result<Reference>,
        DifferenceType>;

#endif
    // clang-format on

}}}


#ifdef BOOST_STL_INTERFACES_DOXYGEN

/** `static_asserts` that type `type` models concept `concept_name`.  This is
    useful for checking that an iterator, view, etc. that you write using one
    ofthe *`_interface` templates models the right C++ concept.

    For example: `BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(my_iter,
    std::input_iterator)`.

    \note This macro exapnds to nothing when `__cpp_lib_concepts` is not
    defined. */
#define BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(type, concept_name)

/** `static_asserts` that the types of all typedefs in
    `std::iterator_traits<iter>` match the remaining macro parameters.  This
    is useful for checking that an iterator you write using
    `iterator_interface` has the correct iterator traits.

    For example: `BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS(my_iter,
    std::input_iterator_tag, std::input_iterator_tag, int, int &, int *, std::ptrdiff_t)`.

    \note This macro ignores the `concept` parameter when `__cpp_lib_concepts`
    is not defined. */
#define BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS(                    \
    iter, category, concept, value_type, reference, pointer, difference_type)

#else

#define BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_CONCEPT_IMPL(              \
    type, concept_name)                                                        \
    static_assert(concept_name<type>, "");

#if 201703L < __cplusplus && defined(__cpp_lib_concepts) ||                    \
    201703L <= __cplusplus && __has_include(<stl2/ranges.hpp>) &&              \
    !defined(BOOST_STL_INTERFACES_DISABLE_CMCSTL2)
#define BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(iter, concept_name)         \
    BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_CONCEPT_IMPL(iter, concept_name)
#else
#define BOOST_STL_INTERFACES_STATIC_ASSERT_CONCEPT(iter, concept_name)
#endif

#define BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS_IMPL(               \
    iter, category, value_t, ref, ptr, diff_t)                                 \
    static_assert(                                                             \
        std::is_same<                                                          \
            typename std::iterator_traits<iter>::iterator_category,            \
            category>::value,                                                  \
        "");                                                                   \
    static_assert(                                                             \
        std::is_same<                                                          \
            typename std::iterator_traits<iter>::value_type,                   \
            value_t>::value,                                                   \
        "");                                                                   \
    static_assert(                                                             \
        std::is_same<typename std::iterator_traits<iter>::reference, ref>::    \
            value,                                                             \
        "");                                                                   \
    static_assert(                                                             \
        std::is_same<typename std::iterator_traits<iter>::pointer, ptr>::      \
            value,                                                             \
        "");                                                                   \
    static_assert(                                                             \
        std::is_same<                                                          \
            typename std::iterator_traits<iter>::difference_type,              \
            diff_t>::value,                                                    \
        "");

#if 201703L < __cplusplus && defined(__cpp_lib_ranges)
#define BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS(                    \
    iter, category, concept, value_type, reference, pointer, difference_type)  \
    static_assert(                                                             \
        std::is_same<                                                          \
            typename std::iterator_traits<iter>::iterator_concept,             \
            concept>::value,                                                   \
        "");                                                                   \
    BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS_IMPL(                   \
        iter, category, value_type, reference, pointer, difference_type)
#else
#define BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS(                    \
    iter, category, concept, value_type, reference, pointer, difference_type)  \
    BOOST_STL_INTERFACES_STATIC_ASSERT_ITERATOR_TRAITS_IMPL(                   \
        iter, category, value_type, reference, pointer, difference_type)
#endif

#endif

#endif
