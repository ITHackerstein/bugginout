#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <variant>

template<typename... Ts>
struct overload : Ts... {
	using Ts::operator()...;
};

namespace bo {

namespace Types {

using Id = std::size_t;

#define _BO_ENUMERATE_BUILTIN_TYPES           \
	BO_ENUMERATE_BUILTIN_TYPE(Unknown, unknown) \
	BO_ENUMERATE_BUILTIN_TYPE(Void, void)       \
	BO_ENUMERATE_BUILTIN_TYPE(U8, u8)           \
	BO_ENUMERATE_BUILTIN_TYPE(U16, u16)         \
	BO_ENUMERATE_BUILTIN_TYPE(U32, u32)         \
	BO_ENUMERATE_BUILTIN_TYPE(U64, u64)         \
	BO_ENUMERATE_BUILTIN_TYPE(USize, usize)     \
	BO_ENUMERATE_BUILTIN_TYPE(I8, i8)           \
	BO_ENUMERATE_BUILTIN_TYPE(I16, i16)         \
	BO_ENUMERATE_BUILTIN_TYPE(I32, i32)         \
	BO_ENUMERATE_BUILTIN_TYPE(I64, i64)         \
	BO_ENUMERATE_BUILTIN_TYPE(ISize, isize)     \
	BO_ENUMERATE_BUILTIN_TYPE(Bool, bool)       \
	BO_ENUMERATE_BUILTIN_TYPE(Char, char)

#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name)       \
	struct klass_name {                                          \
		explicit klass_name() {}                                   \
		inline bool operator==(klass_name const&) const = default; \
	};
_BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE

enum TypeIDs : Id {
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) builtin_##type_name##_id,
	_BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE
};

class Pointer {
public:
	enum class Kind {
		Weak,
		Strong
	};

	explicit Pointer(Kind kind, Id inner_type_id)
	  : m_kind(kind), m_inner_type_id(inner_type_id) {}

	inline bool operator==(Pointer const&) const = default;

	Kind kind() const { return m_kind; }
	Id inner_type_id() const { return m_inner_type_id; }

private:
	Kind m_kind;
	Id m_inner_type_id;
};

class Array {
public:
	explicit Array(std::size_t size, Id inner_type_id)
	  : m_size(size), m_inner_type_id(inner_type_id) {}

	inline bool operator==(Array const&) const = default;

	std::size_t size() const { return m_size; }
	Id inner_type_id() const { return m_inner_type_id; }

private:
	std::size_t m_size;
	Id m_inner_type_id;
};

class Slice {
public:
	explicit Slice(Id inner_type_id)
	  : m_inner_type_id(inner_type_id) {}

	inline bool operator==(Slice const&) const = default;

	Id inner_type_id() const { return m_inner_type_id; }

private:
	Id m_inner_type_id;
};

class Type {
private:
	// clang-format off
	using TypeVariant = std::variant<
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) klass_name,
	  _BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE
	  Pointer,
	  Array,
	  Slice>;
	// clang-format on

public:
#define BO_ENUMERATE_BUILTIN_TYPE(klass_name, type_name) \
	static Type builtin_##type_name(bool is_mutable = false) { return Type(klass_name {}, is_mutable); }
	_BO_ENUMERATE_BUILTIN_TYPES
#undef BO_ENUMERATE_BUILTIN_TYPE
	static Type pointer(Pointer::Kind kind, Id inner_type_id, bool is_mutable = false) { return Type(Pointer { kind, inner_type_id }, is_mutable); }
	static Type array(std::size_t size, Id inner_type_id, bool is_mutable = false) { return Type(Array { size, inner_type_id }, is_mutable); }
	static Type slice(Id inner_type_id, bool is_mutable = false) { return Type(Slice { inner_type_id }, is_mutable); }

	static Type apply_mutability(Type const& type, bool is_mutable) {
		if (type.is_mutable() == is_mutable) {
			return type;
		}

		return Type(TypeVariant { type.m_impl }, is_mutable);
	}

	template<typename T>
	bool is() const { return std::holds_alternative<T>(m_impl); }

	template<typename T>
	T const& as() const {
		assert(is<T>());
		return std::get<T>(m_impl);
	}

	bool is_mutable() const { return m_is_mutable; }

	bool is_builtin() const {
		// NOTE: Not using the macro here, because clang-format sucks :)
		auto visitor = overload {
			[](Unknown const&) { return true; },
			[](Void const&) { return true; },
			[](U8 const&) { return true; },
			[](U16 const&) { return true; },
			[](U32 const&) { return true; },
			[](U64 const&) { return true; },
			[](USize const&) { return true; },
			[](I8 const&) { return true; },
			[](I16 const&) { return true; },
			[](I32 const&) { return true; },
			[](I64 const&) { return true; },
			[](ISize const&) { return true; },
			[](Bool const&) { return true; },
			[](Char const&) { return true; },
			[](auto&&) { return false; }
		};

		return std::visit(visitor, m_impl);
	}

	bool is_integer() const {
		auto visitor = overload {
			[](U8 const&) { return true; },
			[](U16 const&) { return true; },
			[](U32 const&) { return true; },
			[](U64 const&) { return true; },
			[](USize const&) { return true; },
			[](I8 const&) { return true; },
			[](I16 const&) { return true; },
			[](I32 const&) { return true; },
			[](I64 const&) { return true; },
			[](ISize const&) { return true; },
			[](auto&&) { return false; }
		};

		return std::visit(visitor, m_impl);
	}

	bool is_signed() const {
		auto visitor = overload {
			[](I8 const&) { return true; },
			[](I16 const&) { return true; },
			[](I32 const&) { return true; },
			[](I64 const&) { return true; },
			[](ISize const&) { return true; },
			[](auto&&) { return false; }
		};

		return std::visit(visitor, m_impl);
	}

	std::size_t size() const {
		auto visitor = overload {
			[](U8 const&) { return sizeof(std::uint8_t); },
			[](U16 const&) { return sizeof(std::uint16_t); },
			[](U32 const&) { return sizeof(std::uint32_t); },
			[](U64 const&) { return sizeof(std::uint64_t); },
			[](USize const&) { return sizeof(std::size_t); },
			[](I8 const&) { return sizeof(std::int8_t); },
			[](I16 const&) { return sizeof(std::int16_t); },
			[](I32 const&) { return sizeof(std::int32_t); },
			[](I64 const&) { return sizeof(std::int64_t); },
			[](ISize const&) { return sizeof(ssize_t); },
			[](auto&&) -> std::size_t { return 0; }
		};

		return std::visit(visitor, m_impl);
	}

	inline bool operator==(Type const&) const = default;

private:
	explicit Type(TypeVariant&& impl, bool is_mutable)
	  : m_impl(std::move(impl)), m_is_mutable(is_mutable) {}

	TypeVariant m_impl;
	bool m_is_mutable;
};

}

}
