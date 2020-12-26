//NIX_LICENSE_PLACE_HOLDER

#ifndef NIX_BUFFER_H__
#define NIX_BUFFER_H__

#include <nix/memory/heap.h>
#include <nix/range.h>
#include <nix/byte.h>

//STL
#include <algorithm>
#include <type_traits>
#include <array>
namespace nix
{
	/// this class is designed for performance, and the resize does not construct the elements.
	/// As a result, element type can only be of POD types.
	template<typename T>
	class buffer
	{
		static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, "T must be both trival and standard-layout");

		public:
		typedef heap heap_type;                                                 ///< type of the enclosed heap
		typedef T value_type;                                                   ///< type of stored elements
		typedef std::allocator<T> allocator_type;                               ///< type of allocator
		typedef typename allocator_type::size_type size_type;                   ///< type of size
		typedef typename allocator_type::difference_type difference_type;       ///< type of difference size

		typedef typename allocator_type::reference reference;                   ///< type of element reference
		typedef typename allocator_type::const_reference const_reference;       ///< type of element const reference
		typedef typename allocator_type::pointer pointer;                       ///< type of element pointer
		typedef typename allocator_type::const_pointer const_pointer;           ///< type of element const reference

		typedef pointer iterator;                                               ///< type of iterator
		typedef const_pointer const_iterator;                                   ///< type of const iterator

        /// default constructor that constructs an empty buffer object with heap initialized to the global heap
        /// \post empty()
        buffer() noexcept : buffer(get_global_heap())
        {}

        /// construct an empty buffer object with the specified heap as the underlying heap storage
        /// \param h the heap to be utilized in the newly constructed buffer object
        /// \post empty()
		explicit buffer(heap& h) noexcept
			: m_heap(&h)
			, m_capacity(0)
			, m_size(0)
			, m_data(0)
		{}

        /// move constructor
		buffer(buffer&& rhs) noexcept
			: m_heap(rhs.m_heap)
			, m_capacity(rhs.m_capacity)
			, m_size(rhs.m_size)
			, m_data(rhs.m_data)
		{
			rhs.m_capacity = 0;
			rhs.m_size = 0;
			rhs.m_data = 0;
		}

        /// copy constructor. The newly constructed buffer object will have the exact content as the original
        /// one with no extra capacity. i.e. the capacity of the new buffer object will be equal to its size.
        /// Heap from the original object will be utilized in the newly created object.
        /// \param rhs the original buffer object to copy from
        buffer(const buffer& rhs) : buffer(rhs, *rhs.m_heap)
        {}

        /// constructor that works the same as buffer(const buffer& rhs) except that it accept an
        /// additional heap argument that you can specify for the newly constructed buffer object to use
        /// \see buffer(const buffer& rhs)
        /// \param rhs the original buffer object to copy from
        /// \param h the heap to be utilized in the newly constructed buffer object
		buffer(const buffer& rhs, heap& h)
			: m_heap(&h)
			, m_capacity(0)
			, m_size(rhs.m_size)
			, m_data(0)
		{
			if (m_size > 0)
			{
				NIX_EXPECTS(rhs.m_data != nullptr);

				m_capacity = m_size;
				m_data = malloc_(m_capacity);
				std::copy(rhs.m_data, rhs.m_data + m_size, m_data);
			}
		}

        /// ctor that constructs a buffer object with size and capacity equal to count, with each
        /// slot filled with the specified object ch. If no heap is specified, the global heap would
        /// be utilized in the newly constructed object, otherwise, the heap specified is used.
        /// \param count the number of object that the newly created buffer object will contain
        /// \param ch the object to fill the buffer
        /// \param h the heap to be utilized in the newly constructed buffer object
		buffer(size_type count, T ch = {}, heap& h = get_global_heap())
			: m_heap(&h)
			, m_capacity(0)
			, m_size(count)
			, m_data(0)
		{
			if (m_size > 0)
			{
				m_capacity = m_size;
				m_data = malloc_(m_capacity);
				std::fill(m_data, m_data + m_size, ch);
			}
		}

        /// ctor that contructs a buffer object fill with a copy a range of objects. If no heap is
        /// specified, the global heap would be utilized in the newly constructed object, otherwise, the heap specified is used
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to copy from
        /// \param h the heap to be utilized in the newly constructed buffer object
        /// \post size() == r.size()
		template<typename Range, typename=stl::enable_if_t<stl::is_container<Range>::value> >
		buffer(const Range& r, heap& h = get_global_heap())
			: m_heap(&h)
			, m_capacity(0)
			, m_size(0)
			, m_data(0)
		{
			difference_type len = std::distance(r.begin(), r.end());
			NIX_EXPECTS(len >= 0);
			if (len >0)
			{
				m_size = static_cast<size_type>(len);
				m_capacity = m_size;
				m_data = malloc_(m_capacity);
				std::copy(r.begin(), r.end(), m_data);
			}
		}

		/// destructor
		~buffer() noexcept {
			if (m_data)
				free_(m_data, m_capacity);
		}

		/// copy assignment operator
		buffer& operator=(const buffer& rhs)
		{
			if (this != &rhs)
				buffer(rhs).swap(*this);
			return *this;
		}

        /// move assignment operator
		buffer& operator=(buffer&& rhs) noexcept
		{
			buffer(std::move(rhs)).swap(*this);
			return *this;
		}

        /// change the content of the buffer object to be count number of ch object
        /// \param count the number of object that the buffer object will have
        /// \param ch the object that the buffer object will contain
        /// \return reference to the buffer object itself
		buffer& assign(size_type count, T ch)
		{
			buffer(count, ch, *m_heap).swap(*this);
			return *this;
		}

        /// change the content of the buffer object to be a copy of a range of objects
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to copy from
        /// \return reference to the buffer object itself
		template<typename Range>
        stl::enable_if_t<stl::is_container<Range>::value, buffer&> assign(const Range& r)
		{
			buffer(r, *m_heap).swap(*this);
			return *this;
		}

		//memory

        /// get the capacity of the buffer object
        /// \return the capacity
		size_type capacity() const noexcept	{ return m_capacity;}

        /// get the size of the buffer object
        /// \return the size
		size_type size() const noexcept { return m_size;}

        /// reserve space for storing exactly n objects. This method does nothing if the capacity is already
        /// large enough for n object. Otherwise, space for storing n objects will be allocated, old objects
        /// will be copied to the newly allocated space and the old space will be freed.
        /// \param n number of objects to allocate space for
		void reserve(size_type n)
		{
			if (n > m_capacity)
			{
				pointer np = malloc_(n);
				if (m_data != 0)
				{
					std::copy(m_data, m_data + m_size, np);
					free_(m_data, m_capacity);
				}
				m_data = np;
				m_capacity = n;
			}
		}

        /// resize the buffer to store n objects. It will reserve space large enough for storing n objects.
        /// This method does nothing if the capacity is already large enough for n object.
        /// Otherwise, space large enough for storing n objects will be allocated, old objects
        /// will be copied to the newly allocated space and old space will be freed.
        /// \param n new size of the buffer object
        /// \note if resize() extended the buffer, the new elements will not be initialized.
		void resize(size_type n)
		{
			if (n > m_capacity)
			{
				reserve(new_cap_(n));
			}
			m_size = n;
		}

        /// resize the buffer to store n objects. If the new size if larger then the original, the increased
        /// space will be filled with the specified object ch. Otherwise, it does nothing.
        /// \param n new size of buffer
        /// \param ch the object to fill the extra space, if any
		void resize(size_type n, T ch)
		{
			if (n < m_size)
			{
				m_size = n;
			}
			else if (n > m_size)
			{
				if (n > m_capacity)
					reserve(new_cap_(n));
				std::fill(m_data + m_size, m_data + n, ch);
				m_size = n;
			}
		}


		//insert

        /// insert a range of objects at the specified position
        /// \tparam Range the range type that specify a range of objects
        /// \param pos the position in the buffer to insert the new objects
        /// \param r the range of objects to copy from
        /// \pre pos is between [begin(), end()]
        /// \return reference to the buffer object itself
		template<typename Range>
		stl::enable_if_t<stl::is_container<Range>::value, buffer&> insert(iterator pos, const Range& r)
		{
			difference_type len = std::distance(r.begin(), r.end());
			NIX_EXPECTS(len >= 0);
			if (m_size + len > m_capacity)
			{
				size_type ncap = new_cap_(m_size + len);

				pointer np = malloc_(ncap);
				pointer ni = std::copy(begin(), pos, np);
				ni = std::copy(r.begin(), r.end(), ni);
				std::copy(pos, end(), ni);
				if (m_data)
					free_(m_data, m_capacity);
				m_capacity = ncap;
				m_data = np;
			}
			else
			{
				std::copy_backward(pos, end(),  m_data + m_size + len);
				std::copy(r.begin(), r.end(),  pos );
			}
			m_size += len;
			return *this;
		}

        /// insert specified number of a particular object at a specific position
        /// \param pos the position in the buffer to insert the new objects
        /// \param n number of objects to insert
        /// \param ch the specific object to insert
        /// \return reference to the buffer object itself
		buffer& insert(iterator pos, size_type n, T ch)
		{
			if (m_size + n > m_capacity)
			{
				size_type ncap = new_cap_(m_size + n + 1);

				pointer np = malloc_(ncap);
				pointer ni = std::copy(begin(), pos, np);
				std::fill_n(ni, n, ch);
                ni += n;
				std::copy(pos, end(), ni);
				if (m_data)
					free_(m_data, m_capacity);
				m_capacity = ncap;
				m_data = np;
			}
			else
			{
				std::copy_backward(pos, end(),  m_data + m_size + n);
				std::fill_n(pos, n, ch);
			}
			m_size += n;
			return *this;
		}

		//append

        /// append count number of object ch at the end of the buffer object
        /// \param count number of objects to insert
        /// \param ch the specific object to insert
        /// \return reference to the buffer object itself
		buffer& append(size_type count, T ch)
		{
			return insert(end(), count, ch);
		}

        /// append a range of objects at the end of the buffer object
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to insert
        /// \return reference to the buffer object itself
		template<typename Range>
		stl::enable_if_t<stl::is_container<Range>::value, buffer&> append(const Range& r)
		{
			return insert(end(), r);
		}

        /// append a single object to the end of the buffer object
        /// \param ch the object to append
        /// \return reference to the buffer object itself
		buffer& push_back(T ch)
		{
			if (m_size < m_capacity)
			{
				m_data[m_size++] = ch;
			}
			else
				append(1, ch);
			return *this;
		}

		//erase

        /// remove the object referenced by p from the buffer object
        /// \pre p >= begin() && p < end()
        /// \param p the iterator that points to the object to remove
        /// \return the iterator that point to the original position inside the buffer
		iterator erase(iterator p) noexcept
		{
			NIX_EXPECTS(p >= begin() && p < end());
			std::copy(p + 1, end(), p);
			--m_size;
			return p;
		}

        /// remove a range of objects from the buffer object
        /// \pre the specified range must be within the allocated space of the buffer
        /// \param r the range to remvove
        /// \return the beginning position of the range of objects to remove
		iterator erase(const range<iterator>& r) noexcept
		{
            NIX_EXPECTS(r.begin() >= begin() && r.end() <= end());
			if (!empty())
			{
				std::copy(r.end(), end(), r.begin());
				m_size -= std::distance(r.begin(), r.end());
			}
			return r.begin();
		}

        /// replace one range of objects by another range of objects
        /// \tparam Range the range type that specify a range of objects
        /// \param r the range of objects to be replaced
        /// \param f the range of objects to replace
        /// \pre r belongs to this buffer
        /// \return reference to the buffer object itself
		template<typename Range>
		stl::enable_if_t<stl::is_container<Range>::value, buffer&> replace(const range<iterator>& r,
				const Range& f)
		{
            NIX_EXPECTS(r.begin() >= begin() && r.end() <= end());

			difference_type len1 = std::distance(r.begin(), r.end());
			difference_type len2 = std::distance(f.begin(), f.end());
			if (m_size - len1 + len2 < m_capacity)
			{
				if (len1 < len2) //insert
					std::copy_backward(r.end(), end(), end() - len1 + len2);
				else if (len1 > len2)
					std::copy(r.end(), end(), r.begin() + len2);
				std::copy(f.begin(), f.end(), r.begin());
				m_size = m_size + len2 - len1;
			}
			else //insert
			{
				size_type ncap = new_cap_(m_size - len1 + len2 + 1);
				pointer np = malloc_(ncap);
				pointer ni = std::copy(begin(), r.begin(), np);
				ni = std::copy(f.begin(), f.end(), ni);
				std::copy(r.end(), end(), ni);

				free_(m_data, m_capacity);
				m_data = np;
				m_capacity = ncap;
				m_size = m_size + len2 - len1;
			}
			 return *this;
		}

        /// reduce buffer to a sub range of objects
        /// \param r the sub range of objects to reduce to
        /// \return reference to the buffer object itself
		buffer& sub(const range<iterator>& r) noexcept
		{
            NIX_EXPECTS(r.begin() >= begin() && r.end() <= end());

			if (!empty())
			{
				std::copy(r.begin(), r.end(), begin());
				m_size = r.size();
			}
			return *this;
		}

		//methods

        /// clear all the content of the buffer
		void clear() noexcept
		{
			if (!empty())
			{
				m_size = 0;
			}
		}

        /// swap the content of two buffers
		void swap(buffer& rhs) noexcept
		{
			using std::swap;
			swap(m_heap, rhs.m_heap);
			swap(m_capacity, rhs.m_capacity);
			swap(m_size, rhs.m_size);
			swap(m_data, rhs.m_data);
		}


		//queries

        /// check whether the buffer is empty
        /// \return if or not the buffer is empty
		bool empty() const noexcept
		{
			return m_size == 0;
		}

		//iterator

        /// \return iterator to the beginning position of buffer storage
		iterator begin() noexcept
		{
			return m_data;
		}

        /// \return iterator to the end position of buffer storage
		iterator end() noexcept
		{
			return  m_data + m_size;
		}

        /// \return const iterator to the beginning position of buffer storage
		const_iterator begin() const noexcept
		{
			return  m_data;
		}

        /// \return const iterator to the end position of buffer storage
		const_iterator end() const noexcept
		{
			return  m_data + m_size;
		}

		//content access

        /// \return the pointer to the buffer storage
		pointer data() noexcept
		{
			return m_data;
		}

		//content access

        /// \return the const pointer to the buffer storage
        /// \note it returns nullptr if capacity() == 0;
		const_pointer data() const noexcept
		{
			return m_data;
		}

        /// access object in a specific position within a buffer
        /// \param n the index of the object to access
        /// \return reference to the requested object
		reference operator[](size_type n) noexcept
		{
			NIX_EXPECTS(m_data != nullptr &&  n < m_size);
			return m_data[n];
		}

        /// access object in a specific position within a const buffer
        /// \param n the index of the object to access
        /// \return const reference to the requested object
		const_reference operator[](size_type n) const noexcept
		{
			NIX_EXPECTS(m_data != nullptr &&  n < m_size);
			return m_data[n];
		}

        /// get the underlying heap within the buffer object
        /// \return reference to the underlying heap
		heap& get_heap() const noexcept { return *m_heap; }

        /// check if two buffer objects are the same. Two buffers are considered same if
        /// they contain the same number of objects and each object is identical
        /// \param rhs buffer object to compare with
        /// \return whether two buffer objects are same or not
        bool operator == (const buffer<T>& rhs) const noexcept {
            return size() == rhs.size()
                && std::equal(begin(), end(), rhs.begin());
        }

        /// check if two buffer objects are different
        /// \see operator==(const buffer<T>& rhs)
        /// \param rhs buffer object to compare with
        bool operator != (const buffer<T>& rhs) const noexcept {
            return !(*this == rhs);
        }

        /// operator <
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically less than the rhs buffer
        bool operator < (const buffer<T>& rhs) const noexcept {
            return lexicographical_compare(to_range(*this), to_range(rhs));
        }

        /// operator <=
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically less than or equal to the rhs buffer
        bool operator <= (const buffer<T>& rhs) const noexcept {
            return !(*this > rhs);
        }

        /// operator >
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically greater than the rhs buffer
        bool operator > (const buffer<T>& rhs) const noexcept {
            return rhs < *this;
        }

        /// operator >=
        /// \param rhs buffer object to compare with
        /// \return whether the lhs buffer is lexicographically greater than or equal to the rhs buffer
        bool operator >= (const buffer<T>& rhs) const noexcept {
            return !(*this < rhs);
        }

	private:
		pointer malloc_(size_type ncap)
		{
			return reinterpret_cast<pointer>(m_heap->mallocate(ncap * sizeof(T), sizeof(T), 0));
		}

		void free_(pointer p, size_type size)
		{
			m_heap->mfree(p, sizeof(T) * size, sizeof(T));
		}

		size_type new_cap_(size_type n) const
		{
			return
				n < m_capacity
				? m_capacity
				: (n < 2 * m_capacity)
				 ? 2 * m_capacity
				 : n + n / 2;
		}

		heap* m_heap;
		size_type m_capacity;
		size_type m_size;
		T* m_data;
	};

    typedef buffer<byte> byte_buffer;

	template<typename T>
	inline range<byte*> to_byte_range(buffer<T>& buf){
		auto first = cast_to_byte_ptr(buf.data());
		return range<byte*>(first, first + sizeof(T) * buf.size());
	}
	template<typename T>
	inline range<const byte*> to_byte_range(const buffer<T>& buf){
		auto first = cast_to_byte_ptr(buf.data());
		return range<const byte*>(first, first + sizeof(T) * buf.size());
	}

    template<typename T, size_t N>
    inline range<byte*> to_byte_range(std::array<T, N>& buf) {
        auto first = cast_to_byte_ptr(buf.data());
        return range<byte*>(first, first + sizeof(T) * buf.size());
    }
    template<typename T, size_t N>
    inline range<const byte*> to_byte_range(const std::array<T, N>& buf) {
        auto first = cast_to_byte_ptr(buf.data());
        return range<const byte*>(first, first + sizeof(T) * buf.size());
    }
}

#endif //end NIX_BUFFER_H__

