#pragma once

#include <vector>
#include <tuple>
#include <type_traits>
#include <cassert>
#include <deque>
#include <optional>

#include "Types.h"

// TODO(enlynn):
// - Iterator.

//
// struct Foo{};
// struct Goo{};
// struct Zoo{};
//
// using ExamplePool = util::SparseStorage<56, 8, Foo, Foo, Goo, Zoo>;
// auto pool = ExamplePool();
//
// Foo f{}; Goo g{}; Zoo z{};
//
// SomeId id = pool.Create(f, g, z);
// const Foo& foo = pool.getReadable<Foo>(id);
// const auto [foo, goo] = pool.getReadable<Foo, Goo>(id);
//
// for (const auto& foo : pool.iterator<Foo>()) {}
//

namespace util {
    template<u8 IndexBits, u8 GenBits, class UniqueId, class... Args>
    class SparseStorage
    {
    public:
        using Self = SparseStorage<IndexBits, GenBits, UniqueId, Args...>;
        using BackingStorage = std::tuple<std::vector<Args>...>;

    public:
        SparseStorage() = default;
        explicit SparseStorage(size_t tMaxObjects) { setMaxObjects(tMaxObjects); }

        // Copies are disallowed
        SparseStorage(Self&) = delete;
        SparseStorage& operator=(Self&) = delete;

    private:
        // ID Masks and Helper Functions
        //
        static_assert(IndexBits + GenBits < 64, "Sparse Storage IDs are less than 64bits. Provided Index and Gen Bits are greater than 64.");

        // Deduce the ID types and sizes
        using GenerationType = std::conditional_t<GenBits <= 16,             std::conditional_t<GenBits <= 8,               u8, u16>, u32>;
        using IndexType      = std::conditional_t<IndexBits <= 32,           std::conditional_t<IndexBits <= 16,           u16, u32>, u64>;
        using MaskType       = std::conditional_t<IndexBits + GenBits <= 32, std::conditional_t<IndexBits + GenBits <= 16, u16, u32>, u64>;

        union IdMask {
            struct { MaskType mIdx:IndexBits; MaskType mGen:GenBits; };
            MaskType mMask = 0;
        };

        // Define Invalid Masks for each ID variation.
        static constexpr MaskType cInvalidIdMask = static_cast<MaskType>(-1);

        static constexpr IndexType getIndex(IdMask tMask)
        {
            return tMask.mIdx;
        }

        static constexpr GenerationType getGeneration(IdMask tMask)
        {
            return tMask.mGen;
        }

        static constexpr IdMask setGeneration(IdMask tMask, GenerationType tGen)
        {
            tMask.mGen = tGen;
            return tMask;
        }

        static constexpr IdMask setIndex(IdMask tMask, IndexType tIndex)
        {
            tMask.mIdx = tIndex;
            return tMask;
        }

        static constexpr bool isLiveGeneration(GenerationType tGeneration)
        {
            // Odd-Numbered Generations are considered Live handles
            return (tGeneration & 1) == 1;
        }

        static constexpr bool isFreeGeneration(GenerationType tGeneration)
        {
            // Even-Numbered Generations are considered Live handles
            return (tGeneration & 1) == 0;
        }

        static constexpr GenerationType markGenerationAsFree(GenerationType tGeneration)
        {
            if (isLiveGeneration(tGeneration))
            {
                return tGeneration + 1;
            }

            assert(false && "Tried to mark generation as free, but was already free");
            return tGeneration;
        }

        static constexpr GenerationType markGenerationAsAlive(GenerationType tGeneration)
        {
            if (isFreeGeneration(tGeneration))
            {
                return tGeneration + 1;
            }

            assert(false && "Tried to mark generation as alive, but was already alive");
            return tGeneration;
        }

    public:
        // Creates a strongly typed ID based on the user provided "UniqueId"
        //
        template<class T>
        struct IdTypeBase final
        {
            constexpr explicit IdTypeBase(const IdMask tId) : mId(tId) {}
            constexpr explicit IdTypeBase() : mId(cInvalidIdMask) {}
            constexpr explicit operator IdMask() const { return mId; }

            constexpr IdMask get() const { return mId; }
        private:
            const IdMask mId;
        };

        using IdType = IdTypeBase<UniqueId>;

    private:
        static constexpr size_t cMinFreeIndices = 10;

        // Tuple of Vectors for each argument type
        //
        // NOTE(enlynn): GenerationType is loosely types. Let's say a user requests
        //
        template <size_t cColIdx>
        using NthColType = typename std::tuple_element<cColIdx, BackingStorage>::type;

        template <size_t cColIdx>
        using ColType = typename NthColType<cColIdx>::value_type;

        BackingStorage                   mStorage{};   // Data Storage for user defined types
        std::vector<GenerationType>      mGenCycles{}; // Backing storage to determine if IDs are Valid, Alive, or Free
        std::deque<IndexType>            mFreeList{};  // Free list of available indices.

        // Determines if the storage system can resize. Is set to false, when "setStorageLimit()" is called.
        bool                             mIsResizable{true};

    public:

        template <typename... Xs>
        [[nodiscard]] IdType create(Xs&&... tValues)
        {
            // Construct a valid id for the new object.
            IdMask mask{};
            if (mFreeList.size() > cMinFreeIndices)
            {
                IndexType index = mFreeList.front();
                mFreeList.pop_front();

                // There is a logic error in create/destroy. Any index in the free list should have a corresponding
                // generation that is free.
                GenerationType& gen = mGenCycles[index];
                assert(isFreeGeneration(gen));

                // Let's mark the generation as "alive"
                markGenerationAsAlive(gen);
                assert(isLiveGeneration(gen));

                setIndex(mask, index);
                setGeneration(mask, gen);

                // Extra error checking to make sure the index and generation has been set properly
                assert(getIndex(mask) == index);
                assert(getGeneration(mask) == gen);

                // Insert into the back of the storage.
                this->replace(index, std::forward<Xs>(tValues)...);
            }
            else
            {
                const IndexType index = this->size();
                const GenerationType gen = 1;

                if (mIsResizable)
                {
                    // If we have set an upper bound, do not allow for anymore objects to be added to the storage.
                    assert(index < this->cap());

                    // NOTE(enlynn): should the caller be responsible for always checking to make sure a valid id is returned?
                    return IdType({ .mMask = cInvalidIdMask });
                }

                mGenCycles.push_back(gen);

                mask = setIndex(mask, index);
                mask = setGeneration(mask, gen);

                // Extra error checking to make sure the index and generation has been set properly
                assert(getIndex(mask) == index);
                assert(getGeneration(mask) == gen);

                // Insert into the back of the storage.
                this->insert(std::forward<Xs>(tValues)...);
            }

            return IdType(mask);
        }

        void destroy(IdType tId)
        {
            if (!isIdValid(tId))
                return;

            const IndexType index = getIndex(tId.get());
            mGenCycles[index] = markGenerationAsFree(mGenCycles[index]);

            // NOTE(enlynn): Keep an eye on this usage pattern. This will require users destruct their types.
            // Or assumes data in storage is simple POD types.
            //destruct(index);

            mFreeList.push_back(index);
        }

        [[nodiscard]] size_t size() const {
            return mGenCycles.size();
        }

        [[nodiscard]] size_t cap() const {
            return mGenCycles.capacity();
        }

        [[nodiscard]] bool empty() const {
            return mGenCycles.empty();
        }

        [[nodiscard]] size_t getNumLiveObjects() const
        {
            return size() - mFreeList.size();
        }

        [[nodiscard]] bool isIdValid(IdType tId) const
        {
            // 1. Cannot be an invalid id mask
            // 2. Cannot be an index out of bounds
            // 3. Must be a Live Id
            const IdMask mask = tId.get();
            const size_t index = getIndex(mask);

            return (mask.mMask != cInvalidIdMask) && (index < this->size()) && (isLiveGeneration(mGenCycles[index]));
        }

        template <size_t... I>
        auto getReadableByIndex(const IdType tId) const
        {
            // NOTE(enlynn): Will need to see how this function gets used in-practice. I can imagine situations where
            // objects maintain out-of-date handles to other objects. Asserting here says the caller should check and
            // handle when an ID is no longer valid.
            assert(isIdValid(tId));

            return getRowImpl(std::integer_sequence<size_t, I...>{}, static_cast<size_t>(getIndex(tId.get())));
        }

        template <size_t... I>
        auto getWritableByIndex(const IdType tId)
        {
            // NOTE(enlynn): Will need to see how this function gets used in-practice. I can imagine situations where
            // objects maintain out-of-date handles to other objects. Asserting here says the caller should check and
            // handle when an ID is no longer valid.
            assert(isIdValid(tId));

            return getRowImpl(std::integer_sequence<size_t, I...>{}, static_cast<size_t>(getIndex(tId.get())));
        }

        template <typename... Xs>
        auto getWritable(const IdType tId) {
            // NOTE(enlynn): Will need to see how this function gets used in-practice. I can imagine situations where
            // objects maintain out-of-date handles to other objects. Asserting here says the caller should check and
            // handle when an ID is no longer valid.
            assert(isIdValid(tId));

            const size_t index = getIndex(tId.get());
            return std::move(getWritableImpl<Xs...>(index));
        }

        template <typename... Xs>
        auto getReadable(const IdType tId) const {
            // NOTE(enlynn): Will need to see how this function gets used in-practice. I can imagine situations where
            // objects maintain out-of-date handles to other objects. Asserting here says the caller should check and
            // handle when an ID is no longer valid.
            assert(isIdValid(tId));

            const size_t index = getIndex(tId.get());
            return std::move(getReadableImpl<Xs...>(index));
        }

        void setMaxObjects(size_t size) {
            // If this is false, then we've already set this value.
            assert(mIsResizable);
            mIsResizable = false;

            return reserveImpl(std::index_sequence_for<Args...>{}, size);
        }

#if 0 // Possible debug print implementation
        void dump(std::basic_ostream<char>& ss) const {
            constexpr size_t MAX_NUM_ELEMENTS_TO_PRINT = 25;
            size_t num_elements_to_print = size();
            if (num_elements_to_print > MAX_NUM_ELEMENTS_TO_PRINT) {
                num_elements_to_print = MAX_NUM_ELEMENTS_TO_PRINT;
            }
            ss << "soa {\n";
            for (size_t i = 0; i < num_elements_to_print; ++i) {
                const auto t = operator[](i);
                ss << "\t";
                dump_tuple(ss, t);
                ss << std::endl;
            }
            if (size() > MAX_NUM_ELEMENTS_TO_PRINT) {
                ss << "\t..." << std::endl;
            }
            ss << "}" << std::endl;
        }

        template <typename... Ts>
        std::ostream& operator<<(std::ostream& cout, const SparseStorage<Ts...>& tStorage) {
            tStorage.dump(cout);
            return cout;
        }
#endif

    private:
        // Internal Template Implementation
        //

        template<size_t cColIdx>
        const NthColType<cColIdx>& getColumn() const {
            return std::get<cColIdx>(mStorage);
        }

        template<size_t cColIdx>
        NthColType<cColIdx>& getColumn() {
            const Self* constThis = this;
            return const_cast<NthColType<cColIdx>&>(constThis->getColumn<cColIdx>());
        }

        template <typename... Xs>
        void insert(Xs&&... xs) {
            insertImpl(std::index_sequence_for<Args...>{}, std::forward_as_tuple(xs...));
        }

        // this should be private
        template <typename... Xs>
        void replace(const size_t tIndex, Xs&&... xs)
        {
            replaceImpl(tIndex, std::index_sequence_for<Args...>{}, std::forward_as_tuple(xs...));
        }

        template <typename T, size_t... I>
        void insertImpl(std::integer_sequence<size_t, I...>, T t) {
            // Gets the column vector [std::vec<Type0>, std::vec<Type1>, ...]
            // Push the value at (tIndex) with t (the element-list tuple - [Type0 value0, Type1 value1, ...])
            ((getColumn<I>().push_back(std::get<I>(t))), ...);
        }

        template <typename T, size_t... I>
        void replaceImpl(const size_t tIndex, std::integer_sequence<size_t, I...>, T t) {
            // Gets the column vector [std::vec<Type0>, std::vec<Type1>, ...]
            // Sets the value at (tIndex) with t (the element-list tuple - [Type0 value0, Type1 value1, ...])
            ((getColumn<I>()[tIndex] = (std::get<I>(t))), ...);
        }

        template <size_t... I>
        auto getRowImpl(std::integer_sequence<size_t, I...>, size_t row) const {
            return std::tie(getColumn<I>()[row]...);
        }

        template <size_t... I>
        auto getRowImpl(std::integer_sequence<size_t, I...>, size_t row) {
            return std::tie(getColumn<I>()[row]...);
        }

        template <size_t... I>
        void clearImpl(std::integer_sequence<size_t, I...>) {
            ((getColumn<I>().clear()), ...);
        }

        template <size_t... I, typename T>
        void resizeImpl(std::integer_sequence<size_t, I...>, T&& data, size_t new_size) {
            ((std::get<I>(data).resize(new_size)), ...);
        }

        template <size_t... I>
        void reserveImpl(std::integer_sequence<size_t, I...>, size_t res_size) {
            ((getColumn<I>().reserve(res_size)), ...);
        }

        template <typename... Xs>
        auto getWritableImpl(const size_t tRow) {
            return std::tie(std::get<std::vector<Xs>>(mStorage)[tRow]...);
        }

        template <typename... Xs>
        auto getReadableImpl(const size_t tRow) const {
            return std::tie(std::get<std::vector<Xs>>(mStorage)[tRow]...);
        }

    public: // Iterators
        class LiveIndicesIterator
        {
        public:
            using ValueType = std::vector<GenerationType>;
            using ValuePtr = ValueType*;

            explicit LiveIndicesIterator(ValuePtr tLiveIndices) : mGenerations(tLiveIndices) {}

            std::optional<size_t> next() {
                const size_t maxIndex = mGenerations->size();
                while(!mEnded && (mNextIndex < maxIndex)) {
                    const size_t index = mNextIndex;

                    // increment the next index if there are any remaining indices
                    if (index < maxIndex) {
                        mNextIndex += 1;
                    }
                    else {
                        mEnded = true;
                    }

                    // if the current index is live, then return it, else continue
                    if (isLiveGeneration(mGenerations->at(index))) {
                        return index;
                    }
                }

                // failed to find an index, likely at the end of the list
                return std::nullopt;
            }

            ValuePtr  mGenerations{};
            IndexType mNextIndex{0};
            bool      mEnded{false};
        };

        class LiveHandleIterator
        {
        public:
            using ValueType = std::vector<GenerationType>;
            using ValuePtr = ValueType*;

            explicit LiveHandleIterator(ValuePtr tLiveIndices) : mLiveIndices(LiveIndicesIterator(tLiveIndices)) {}

            std::optional<IdType> next()
            {
                auto optIndex = mLiveIndices.next();
                if (optIndex.has_value()) {
                    GenerationType gen = mLiveIndices.mGenerations->at(optIndex.value());

                    IdMask mask{};
                    mask = setIndex(mask, optIndex.value());
                    mask = setGeneration(mask, gen);
                    return IdType(mask);
                }

                return std::nullopt;
            }

            LiveIndicesIterator mLiveIndices;
        };

        template<size_t... ColumnIndices>
        class ColumnIterator
        {
        public:
            explicit ColumnIterator(Self* tStorage) : mStorage(tStorage), mLiveHandles(LiveHandleIterator(&tStorage->mGenCycles)) {}

            auto next()
            {
                auto id = mLiveHandles.next();
                if (id.has_value())
                {
                    return mStorage->getWritable<ColumnIndices...>(id.value());
                }
                return std::make_tuple();
            }

        private:
            Self*              mStorage;
            LiveHandleIterator mLiveHandles;
        };

        LiveHandleIterator getHandleIterator() {
            return LiveHandleIterator(mGenCycles);
        }

        template<size_t... ColumnIndices>
        ColumnIterator<ColumnIndices...> getValueIterator() {
            return ColumnIterator(mGenCycles);
        }
    };
} // util
