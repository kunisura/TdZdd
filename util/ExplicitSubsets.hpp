/*
 * Top-Down ZDD Construction Library for Frontier-Based Search
 * by Hiroaki Iwashita <iwashita@erato.ist.hokudai.ac.jp>
 * Copyright (c) 2013 Japan Science and Technology Agency
 * $Id: ExplicitSubsets.hpp 485 2013-09-06 11:19:03Z iwashita $
 */

#pragma once

#include <algorithm>
#include <climits>

#include "MessageHandler.hpp"
#include "MyVector.hpp"

namespace tdzdd {

class ExplicitSubsets {
    int beg_;
    int end_;
    MyVector<uint64_t> words;

    static uint64_t bit(int i) {
        assert(0 <= i && i <= 63);
        return uint64_t(1) << (63 - i);
    }

public:
    /**
     * Default constructor.
     */
    ExplicitSubsets()
            : beg_(INT_MIN), end_(INT_MIN) {
    }

    /**
     * Constructor with parameters.
     */
    ExplicitSubsets(int first, int last)
            : beg_(first), end_(last) {
    }

    /**
     * Returns the number of bits for each bit-vector.
     */
    int vectorBits() const {
        return end_ - beg_;
    }

    /**
     * Returns the number of words for each bit-vector.
     */
    int vectorSize() const {
        return (vectorBits() + 63) / 64;
    }

    /**
     * Returns the number of bit-vectors.
     */
    size_t size() const {
        return words.size() / vectorSize();
    }

    /**
     * Changes the number of bit-vectors.
     */
    void resize(size_t n) {
        return words.resize(n * vectorSize());
    }

    /**
     * Removes all elements.
     */
    void clear() {
        words.clear();
    }

    template<typename T>
    struct Element_ {
        int const size;
        T* data;

    public:
        Element_(int size, T* data)
                : size(size), data(data) {
        }

        Element_& operator=(Element_ const& o) {
            assert(size == o.size);
            for (int i = 0; i < size; ++i) {
                data[i] = o.data[i];
            }
            return *this;
        }

        void swap(Element_& o) {
            assert(size == o.size);
            for (int i = 0; i < size; ++i) {
                std::swap(data[i], o.data[i]);
            }
        }

        bool operator==(Element_ const& o) const {
            assert(size == o.size);
            for (int i = 0; i < size; ++i) {
                if (data[i] != o.data[i]) return false;
            }
            return true;
        }

        bool operator!=(Element_ const& o) const {
            return !(*this == o);
        }

        bool operator<(Element_ const& o) const {
            assert(size == o.size);
            for (int i = 0; i < size; ++i) {
                if (data[i] != o.data[i]) return data[i] < o.data[i];
            }
            return false;
        }

        /**
         * Sets the @p k-th bit.
         */
        void set(int k) {
            assert(0 <= k && k < size * 64);
            data[k / 64] |= bit(k % 64);
        }

        /**
         * Clears the @p k-th bit.
         */
        void clear(int k) {
            assert(0 <= k && k < size * 64);
            data[k / 64] &= ~bit(k % 64);
        }

        /**
         * Returns the @p k-th bit.
         */
        bool get(int k) const {
            assert(0 <= k && k < size * 64);
            return data[k / 64] & bit(k % 64);
        }

        /**
         * Returns a 64-bit word beggining from the @p k-th bit.
         */
        uint64_t getWord(int k) const {
            if (size == 0) return 0;
            uint64_t v = 0;
            if (0 <= k && k < size * 64) {
                v = data[k / 64] << (k % 64);
            }
            int l = k + 63;
            if (0 <= l && l < size * 64 && k % 64) {
                v |= data[l / 64] >> (63 - l % 64);
            }
            return v;
        }

        /**
         * Computes a hash code.
         * @param k bit position to start computation.
         */
        size_t hash(int k = 0) const {
            assert(0 <= k && k < size * 64);
            int i0 = k / 64;
            int j0 = k % 64;
            size_t h = data[i0] << j0;
            for (int i = i0 + 1; i < size; ++i) {
                h = h * 31 + data[i];
            }
            return h;
        }

        /**
         * Checks equality with another object.
         * @param o another object.
         * @param k bit position to start comparison.
         */
        bool equal(Element_ const& o, int k = 0) const {
            assert(0 <= k && k < size * 64);
            assert(size == o.size);
            int i0 = k / 64;
            int j0 = k % 64;
            if ((data[i0] << j0) != (o.data[i0] << j0)) return false;
            for (int i = i0 + 1; i < size; ++i) {
                if (data[i] != o.data[i]) return false;
            }
            return true;
        }

        friend std::ostream& operator<<(std::ostream& os, Element_ const& o) {
            int k = 0;
            for (int i = 0; i < o.size; ++i) {
                if (i != 0) os << '_';
                for (int j = 0; j <= 63; ++j) {
                    os << (o.get(k++) ? '1' : '0');
                }
            }
            return os;
        }
    };

    template<typename T>
    class Iterator_ {
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef Element_<T> value_type;
        typedef size_t difference_type;
        typedef difference_type distance_type;
        typedef Element_<T>* pointer;
        typedef Element_<T>& reference;

    private:
        value_type elem;

    public:
        Iterator_(int size, T* data)
                : elem(size, data) {
        }

        reference operator*() {
            return elem;
        }

        pointer operator->() {
            return &elem;
        }

        Iterator_& operator+=(int n) {
            elem.data += elem.size * n;
            return *this;
        }

        Iterator_& operator-=(int n) {
            elem.data -= elem.size * n;
            return *this;
        }

        Iterator_& operator++() {
            elem.data += elem.size;
            return *this;
        }

        Iterator_& operator--() {
            elem.data -= elem.size;
            return *this;
        }

        Iterator_ operator+(size_t n) const {
            Iterator_ o(*this);
            o += n;
            return o;
        }

        Iterator_ operator-(size_t n) const {
            Iterator_ o(*this);
            o -= n;
            return o;
        }

        size_t operator-(Iterator_ const& o) const {
            return elem.data - o.elem.data;
        }

        bool operator==(Iterator_ const& o) const {
            return elem.data == o.elem.data;
        }

        bool operator!=(Iterator_ const& o) const {
            return elem.data != o.elem.data;
        }

        bool operator<(Iterator_ const& o) const {
            return elem.data < o.elem.data;
        }

        bool operator>(Iterator_ const& o) const {
            return elem.data > o.elem.data;
        }

        bool operator<=(Iterator_ const& o) const {
            return elem.data <= o.elem.data;
        }

        bool operator>=(Iterator_ const& o) const {
            return elem.data >= o.elem.data;
        }
    };

    typedef Element_<uint64_t> Element;
    typedef Element_<uint64_t const> ConstElement;
    typedef Iterator_<uint64_t> iterator;
    typedef Iterator_<uint64_t const> const_iterator;

    /**
     * Returns the @p i-th bitvector.
     * @param i index of the vector.
     */
    Element element(size_t i) {
        int w = vectorSize();
        return Element(w, &words[i * w]);
    }

    /**
     * Returns the @p i-th bitvector.
     * @param i index of the vector.
     */
    ConstElement element(size_t i) const {
        int w = vectorSize();
        return ConstElement(w, &words[i * w]);
    }

    /**
     * Returns an iterator to the beginning.
     */
    iterator begin() {
        return iterator(vectorSize(), words.begin());
    }

    /**
     * Returns an iterator to the beginning.
     */
    const_iterator begin() const {
        return const_iterator(vectorSize(), words.begin());
    }

    /**
     * Returns an iterator to the end.
     */
    iterator end() {
        return iterator(vectorSize(), words.end());
    }

    /**
     * Returns an iterator to the end.
     */
    const_iterator end() const {
        return const_iterator(vectorSize(), words.end());
    }

    /**
     * Adds a collection of integers as the last elem.
     * @param first pointer to the beginning of the integers.
     * @param last pointer to the end of the integers.
     */
    template<typename Iter>
    void add(Iter first, Iter last) {
        int w = vectorSize();
        for (int j = 0; j < w; ++j) {
            words.push_back(0);
        }
        Element e(w, words.end() - w);
        for (Iter t = first; t != last; ++t) {
            e.set(*t - beg_);
        }
    }

    /**
     * Imports all elements in another object at the end.
     * @param o another object.
     */
    void addAll(ExplicitSubsets const& o) {
        size_t n = o.size();
        int w = vectorSize();
        if (o.beg_ >= o.end_) {
            for (size_t i = 0; i < n; ++i) {
                for (int j = 0; j < w; ++j) {
                    words.push_back(0);
                }
            }
        }
        else {
            int d = beg_ - o.beg_;
            for (const_iterator t = o.begin(); t != o.end(); ++t) {
                for (int j = 0; j < w; ++j) {
                    words.push_back(t->getWord(d + j * 64));
                }
            }
        }
    }

    /**
     * Reads lines of bvv from an input stream.
     * Items must be non-negative integers.
     * @param is input stream.
     */
    void read(std::istream& is) {
        MessageHandler mh;
        mh.begin("reading") << " ...";
        std::string line;
        MyVector<int> items;
        MyVector<ExplicitSubsets> temp;
        ExplicitSubsets* bvv = 0;

        while (is) {
            int v = 0;
            bool valid = false;
            std::getline(is, line);

            for (typeof(line.begin()) p = line.begin(); p != line.end(); ++p) {
                int a = *p - '0';

                if (0 <= a && a <= 9) {
                    v = v * 10 + a;
                    valid = true;
                }
                else if (valid) {
                    items.push_back(v);
                    if (beg_ < 0 || beg_ > v) beg_ = v;
                    if (end_ <= v) end_ = v + 1;
                    v = 0;
                    valid = false;
                }
            }

            if (valid) {
                items.push_back(v);
                if (beg_ < 0 || beg_ > v) beg_ = v;
                if (end_ <= v) end_ = v + 1;
            }

            if (!items.empty()) {
                if (bvv == 0 || bvv->beg_ != beg_ || bvv->end_ != end_) {
                    temp.push_back(ExplicitSubsets(beg_, end_));
                    bvv = &temp.back();
                }
                bvv->add(items.begin(), items.end());
                items.resize(0);
            }
        }

        for (size_t i = 0; i < temp.size(); ++i) {
            addAll(temp[i]);
            temp[i].clear();
        }

        mh.end();
        if (beg_ < end_) {
            mh << "item_range = [" << beg_ << "," << (end_ - 1) << "]\n";
        }
        mh << "#itemset = " << size() << "\n";
    }

    class Less {
        ExplicitSubsets const& bvv;

    public:
        Less(ExplicitSubsets const& bvv)
                : bvv(bvv) {
        }

        bool operator()(size_t i1, size_t i2) const {
            int w = bvv.vectorSize();
            ConstElement e1(w, &bvv.words[i1 * w]);
            ConstElement e2(w, &bvv.words[i2 * w]);
            return e1 < e2;
        }
    };

    class Equal {
        ExplicitSubsets const& bvv;

    public:
        Equal(ExplicitSubsets const& bvv)
                : bvv(bvv) {
        }

        bool operator()(size_t i1, size_t i2) const {
            int w = bvv.vectorSize();
            ConstElement e1(w, &bvv.words[i1 * w]);
            ConstElement e2(w, &bvv.words[i2 * w]);
            return e1 == e2;
        }
    };

    /**
     * Sorts the elements and removes duplucations.
     */
    ExplicitSubsets& sortAndUnique() {
        MessageHandler mh;
        mh.begin("sorting") << " ...";
        int w = vectorSize();

        MyVector<size_t> ptoi(size());
        for (size_t i = 0; i < size(); ++i) {
            ptoi[i] = i;
        }
        std::sort(ptoi.begin(), ptoi.end(), Less(*this));
        size_t n = std::unique(ptoi.begin(), ptoi.end(), Equal(*this))
                - ptoi.begin();

        MyVector<size_t> itop(size());
        for (size_t p = 0; p < n; ++p) {
            itop[ptoi[p]] = p;
        }

        for (size_t pi = 0; pi < n; ++pi) {
            size_t ii = ptoi[pi];
            size_t pp = itop[pi];

            ptoi[pp] = ptoi[pi];
            itop[ii] = itop[pi];

            Element e1(w, &words[pi * w]);
            Element e2(w, &words[ii * w]);
            e1.swap(e2);
        }

        resize(n);
        mh.end();
        mh << "#itemset = " << size() << "\n";
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, ExplicitSubsets const& o) {
        os << "[" << o.beg_ << "," << (o.end_ - 1) << "]\n";
        for (const_iterator t = o.begin(); t != o.end(); ++t) {
            os << " " << *t << "\n";
        }
        return os;
    }
};

} // namespace tdzdd
