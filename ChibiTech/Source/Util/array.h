#pragma once

#include <Types.h>

//  TODO(enlynn): Remove this from the project.

// fixed sized array, does not own memory. used for two situations:
// - Immutable Array, acts as a convienient wrapper around a Ptr and length
// - Fixed Sized Array, must free separately with an allocator.
template<class T>
class farray
{
public:
	farray() : mArray(nullptr), mCount(0) {};
	constexpr farray(T* Array, u64 Count) : mArray(Array), mCount(Count)  {}

	farray(u64 Count)
	{
		mArray = new T[Count];
		mCount = Count;
	}

	~farray() { mArray = nullptr; mCount = 0; }

	void Deinit()
	{
        if (mArray) {
            delete[] mArray;
        }

        mArray = nullptr;
		mCount = 0;
	}

	constexpr operator const T* ()           const { return mArray; }
	constexpr operator T* ()                       { return mArray; }

	constexpr u64      Length()              const { return mCount; }
	constexpr const T* Ptr()                 const { return mArray; }
	constexpr       T* Ptr()                       { return mArray; }

	constexpr const T& operator[](u64 Index) const { return Ptr()[Index];     }
	constexpr       T& operator[](u64 Index)       { return Ptr()[Index];     }

	// Legacy iterators
	constexpr const T* begin()               const { return Ptr();            }
	constexpr const T* end()                 const { return Ptr() + Length(); }
	constexpr       T* begin()                     { return Ptr();            }
	constexpr       T* end()                       { return Ptr() + Length(); }

	// Comparison operators. Comparison with marray is implemented inside of marray.
	// Requires the == operator to be implemented for the underlying type.
	inline friend bool operator==(farray Lhs, farray Rhs)
	{
		u64 LeftLength  = Lhs.Length();
		u64 RightLength = Rhs.Length();
		if (LeftLength != RightLength) return false;

		ForRange(u64, i, LeftLength)
		{
			if (Lhs[i] != Rhs[i]) return false;
		}

		return true;
	}

	inline friend bool operator!=(farray Lhs, farray Rhs)  { return !(Lhs == Rhs); }

private:
	T*  mArray = nullptr;
	u64 mCount = 0;
};
