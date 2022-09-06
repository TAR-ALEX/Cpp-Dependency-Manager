// BSD 3-Clause License

// Copyright (c) 2022, Alex Tarasov
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <memory>
#include <optional>
#include <type_traits>

#define DEFINE_BIN_OP(OPT)                                                                                             \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT(const T2& other) {                                                                     \
		return this->value() OPT other;                                                                                \
	}                                                                                                                  \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT(T2&& other) {                                                                          \
		return this->value() OPT other;                                                                                \
	}                                                                                                                  \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT(const clone_ptr<T2>& other) {                                                          \
		return this->value() OPT other.value();                                                                        \
	}                                                                                                                  \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT(clone_ptr<T2>&& other) {                                                               \
		return this->value() OPT other.value();                                                                        \
	}


#define DEFINE_PRE_OP(OPT)                                                                                             \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT(int) {                                                                                 \
		return OPT this->value();                                                                                      \
	}

#define DEFINE_POST_OP(OPT)                                                                                            \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT() {                                                                                    \
		return this->value() OPT;                                                                                      \
	}

#define DEFINE_UNARY_OP(OPT)                                                                                           \
	template <typename T2>                                                                                             \
	decltype(auto) operator OPT() {                                                                                    \
		return OPT this->value();                                                                                      \
	}


namespace estd {
	class clonable {
	public:
		virtual clonable* clone() const = 0;
		virtual ~clonable() = default;
	};

	// Literally the same as a unique pointer, but you can copy it and it makes
	// two copies of the object, value like behavior.
	template <typename T>
	class clone_ptr : public std::unique_ptr<T> {
		static_assert(
			!std::is_array<T>::value, "Error: clone_ptr not supported on raw arrays, they are not easily copyable."
		);

		using Parent = std::unique_ptr<T>;

	public:
		using std::unique_ptr<T>::unique_ptr;

		inline T& has_value() const { return this->get() != nullptr; }

		inline T& value() const {
			if (this->get() == nullptr) throw std::runtime_error("getting the value of nullptr");
			return *(this->get());
		}

		clone_ptr() {}

		clone_ptr(const T& other) {
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other.clone());
			} else {
				this->reset(new T(other));
			}
		}

		clone_ptr(T&& other) {
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other.clone());
			} else {
				this->reset(new T(other));
			}
		}

		clone_ptr(const clone_ptr& other) {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
		}

		template <typename T2>
		clone_ptr(const clone_ptr<T2>& other) {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
		}

		clone_ptr(clone_ptr&& other) {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
		}

		template <typename T2>
		clone_ptr(clone_ptr<T2>&& other) {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
		}

		clone_ptr(T* other) { this->reset(other); }

		clone_ptr& operator=(const clone_ptr& other) noexcept {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return *this;
				;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
			return *this;
		}

		template <typename T2>
		clone_ptr& operator=(const clone_ptr<T2>& other) noexcept {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return *this;
				;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
			return *this;
		}

		clone_ptr& operator=(clone_ptr&& other) noexcept {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return *this;
				;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
			return *this;
		}

		template <typename T2>
		clone_ptr& operator=(clone_ptr<T2>&& other) noexcept {
			if (other.get() == nullptr) {
				this->reset(nullptr);
				return *this;
				;
			}
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other->clone());
			} else {
				this->reset(new T(*other));
			}
			return *this;
		}

		clone_ptr& operator=(const T& other) noexcept {
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other.clone());
			} else {
				this->reset(new T(other));
			}
			return *this;
		}

		clone_ptr& operator=(T&& other) noexcept {
			if constexpr (std::is_base_of<clonable, T>::value) {
				this->reset((T*)other.clone());
			} else {
				this->reset(new T(other));
			}
			return *this;
		}

		clone_ptr& operator=(T* other) noexcept {
			this->reset(other);
			return *this;
		}

		template <
			typename... Args,
			typename = decltype(typename std::remove_all_extents<T>::type(std::declval<Args>()...))>
		clone_ptr(Args&&... params) : Parent(new T(std::forward<Args>(params)...)) {}

		/////////ITERATOR FORWARDING/////////

		decltype(auto) begin() { return this->value().begin(); }
		decltype(auto) begin() const { this->value().begin(); }
		decltype(auto) end() { return this->value().end(); }
		decltype(auto) end() const { return this->value().end(); }

		/////////OPERATOR FORWARDING/////////

		template <typename T2>
		decltype(auto) operator[](T2 i) const {
			if constexpr (!std::is_array<T>::value) {
				return this->value()[i];
			} else {
				return (this->get())[i];
			}
		}

		DEFINE_BIN_OP(+)
		DEFINE_BIN_OP(-)
		DEFINE_BIN_OP(*)
		DEFINE_BIN_OP(/)
		DEFINE_BIN_OP(%)
		DEFINE_BIN_OP(^)
		DEFINE_BIN_OP(&)
		DEFINE_BIN_OP(|)
		DEFINE_BIN_OP(<)
		DEFINE_BIN_OP(>)

		DEFINE_BIN_OP(+=)
		DEFINE_BIN_OP(-=)
		DEFINE_BIN_OP(*=)
		DEFINE_BIN_OP(/=)
		DEFINE_BIN_OP(%=)
		DEFINE_BIN_OP(^=)
		DEFINE_BIN_OP(&=)
		DEFINE_BIN_OP(|=)

		DEFINE_BIN_OP(<<)
		DEFINE_BIN_OP(>>)
		DEFINE_BIN_OP(<<=)
		DEFINE_BIN_OP(>>=)

		DEFINE_BIN_OP(==)
		DEFINE_BIN_OP(!=)

		DEFINE_BIN_OP(<=)
		DEFINE_BIN_OP(>=)

		DEFINE_BIN_OP(&&)
		DEFINE_BIN_OP(||)

		DEFINE_UNARY_OP(~)
		DEFINE_UNARY_OP(!)

		DEFINE_POST_OP(++)
		DEFINE_POST_OP(--)

		DEFINE_PRE_OP(++)
		DEFINE_PRE_OP(--)

		template <typename T2>
		decltype(auto) operator,(const T2& other) {
			return (this->value()), other;
		}

		template <typename T2>
		decltype(auto) operator,(T2&& other) {
			return (this->value()), other;
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... params) {
			(this->value())(std::forward<Args>(params)...);
		}

		T& operator*() const { return this->value(); }

		bool operator==(std::nullptr_t) { return this->get() == nullptr; }
		bool operator!=(std::nullptr_t) { return this->get() != nullptr; }

#if __cplusplus >= 202002L
		DEFINE_BIN_OP(<=>)
#endif
	};


	// Literally the same as a shared pointer, but it forwards operators and has a forwarding constructor
	template <typename T>
	class joint_ptr : public std::shared_ptr<T> {
		using Parent = std::shared_ptr<T>;

	public:
		using std::shared_ptr<T>::shared_ptr;

		template <
			typename... Args,
			typename = decltype(typename std::remove_all_extents<T>::type(std::declval<Args>()...))>
		joint_ptr(Args&&... params) : Parent(new T(std::forward<Args>(params)...)) {}

		inline T& has_value() const { return this->get() != nullptr; }

		inline T& value() const {
			if (this->get() == nullptr) throw std::runtime_error("getting the value of nullptr");
			return *(this->get());
		}

		/////////ITERATOR FORWARDING/////////

		decltype(auto) begin() { return this->value().begin(); }
		decltype(auto) begin() const { this->value().begin(); }
		decltype(auto) end() { return this->value().end(); }
		decltype(auto) end() const { return this->value().end(); }

		/////////OPERATOR FORWARDING/////////

		template <typename T2>
		decltype(auto) operator[](T2 i) const {
			if constexpr (!std::is_array<T>::value) {
				return this->value()[i];
			} else {
				return (this->get())[i];
			}
		}

		DEFINE_BIN_OP(+)
		DEFINE_BIN_OP(-)
		DEFINE_BIN_OP(*)
		DEFINE_BIN_OP(/)
		DEFINE_BIN_OP(%)
		DEFINE_BIN_OP(^)
		DEFINE_BIN_OP(&)
		DEFINE_BIN_OP(|)
		DEFINE_BIN_OP(<)
		DEFINE_BIN_OP(>)

		DEFINE_BIN_OP(+=)
		DEFINE_BIN_OP(-=)
		DEFINE_BIN_OP(*=)
		DEFINE_BIN_OP(/=)
		DEFINE_BIN_OP(%=)
		DEFINE_BIN_OP(^=)
		DEFINE_BIN_OP(&=)
		DEFINE_BIN_OP(|=)

		DEFINE_BIN_OP(<<)
		DEFINE_BIN_OP(>>)
		DEFINE_BIN_OP(<<=)
		DEFINE_BIN_OP(>>=)

		DEFINE_BIN_OP(==)
		DEFINE_BIN_OP(!=)

		DEFINE_BIN_OP(<=)
		DEFINE_BIN_OP(>=)

		DEFINE_BIN_OP(&&)
		DEFINE_BIN_OP(||)

		DEFINE_UNARY_OP(~)
		DEFINE_UNARY_OP(!)

		DEFINE_POST_OP(++)
		DEFINE_POST_OP(--)

		DEFINE_PRE_OP(++)
		DEFINE_PRE_OP(--)

		template <typename T2>
		decltype(auto) operator,(const T2& other) {
			return (this->value()), other;
		}

		template <typename T2>
		decltype(auto) operator,(T2&& other) {
			return (this->value()), other;
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... params) {
			(this->value())(std::forward<Args>(params)...);
		}

		T& operator*() const { return this->value(); }

		bool operator==(std::nullptr_t) { return this->get() == nullptr; }
		bool operator!=(std::nullptr_t) { return this->get() != nullptr; }

#if __cplusplus >= 202002L
		DEFINE_BIN_OP(<=>)
#endif
	};

	template <typename T>
	class stack_ptr : public std::optional<T> {
		using Parent = std::optional<T>;

	public:
		using std::optional<T>::optional;

		template <
			typename... Args,
			typename = decltype(typename std::remove_all_extents<T>::type(std::declval<Args>()...))>
		stack_ptr(Args&&... params) : Parent(T(std::forward<Args>(params)...)) {}

		inline T& value() const { return *(const_cast<T*>(&(Parent::value()))); }

		inline T* get() const {
			if (Parent::has_value()) return &value();
			return nullptr;
		}

		/////////ITERATOR FORWARDING/////////

		decltype(auto) begin() { return this->value().begin(); }
		decltype(auto) begin() const { this->value().begin(); }
		decltype(auto) end() { return this->value().end(); }
		decltype(auto) end() const { return this->value().end(); }

		/////////OPERATOR FORWARDING/////////

		template <typename T2>
		decltype(auto) operator[](T2 i) const {
			if constexpr (!std::is_array<T>::value) {
				return this->value()[i];
			} else {
				return (this->get())[i];
			}
		}

		DEFINE_BIN_OP(+)
		DEFINE_BIN_OP(-)
		DEFINE_BIN_OP(*)
		DEFINE_BIN_OP(/)
		DEFINE_BIN_OP(%)
		DEFINE_BIN_OP(^)
		DEFINE_BIN_OP(&)
		DEFINE_BIN_OP(|)
		DEFINE_BIN_OP(<)
		DEFINE_BIN_OP(>)

		DEFINE_BIN_OP(+=)
		DEFINE_BIN_OP(-=)
		DEFINE_BIN_OP(*=)
		DEFINE_BIN_OP(/=)
		DEFINE_BIN_OP(%=)
		DEFINE_BIN_OP(^=)
		DEFINE_BIN_OP(&=)
		DEFINE_BIN_OP(|=)

		DEFINE_BIN_OP(<<)
		DEFINE_BIN_OP(>>)
		DEFINE_BIN_OP(<<=)
		DEFINE_BIN_OP(>>=)

		DEFINE_BIN_OP(==)
		DEFINE_BIN_OP(!=)

		DEFINE_BIN_OP(<=)
		DEFINE_BIN_OP(>=)

		DEFINE_BIN_OP(&&)
		DEFINE_BIN_OP(||)

		DEFINE_UNARY_OP(~)
		DEFINE_UNARY_OP(!)

		DEFINE_POST_OP(++)
		DEFINE_POST_OP(--)

		DEFINE_PRE_OP(++)
		DEFINE_PRE_OP(--)

		template <typename T2>
		decltype(auto) operator,(const T2& other) {
			return (this->value()), other;
		}

		template <typename T2>
		decltype(auto) operator,(T2&& other) {
			return (this->value()), other;
		}

		template <typename... Args>
		decltype(auto) operator()(Args&&... params) {
			(this->value())(std::forward<Args>(params)...);
		}

		T& operator*() const { return *(this->get()); }

		bool operator==(std::nullptr_t) { return this->get() == nullptr; }
		bool operator!=(std::nullptr_t) { return this->get() != nullptr; }

#if __cplusplus >= 202002L
		DEFINE_BIN_OP(<=>)
#endif
	};

	namespace shortnames {
		template <typename T>
		using cptr = clone_ptr<T>;
		template <typename T>
		using jptr = joint_ptr<T>;
		template <typename T>
		using sptr = stack_ptr<T>;
	};// namespace shortnames
};	  // namespace estd

#undef DEFINE_PRE_OP
#undef DEFINE_POST_OP
#undef DEFINE_UNARY_OP
#undef DEFINE_BIN_OP
