#ifndef VIEWS_H
#define VIEWS_H

#include <Arduino.h>
#include <Printable.h>

/**
 * @class MemoryView
 * @brief A generic, non-owning window into any contiguous data array.
 * @tparam T The type of the data elements.
 */
template<typename T>
class MemoryView {
public:
  /** @brief Default constructor creating an empty view. */
  MemoryView()
    : _data(nullptr), _len(0) {}

  /** * @brief Constructs a view from a pointer and a specific length. 
   * @return A MemoryView pointing to the provided memory.
   */
  MemoryView(const T* data, size_t len)
    : _data(data), _len(len) {}

  /** * @brief Constructs a view directly from a fixed-size C-style array. 
   * @return A MemoryView covering the entire array.
   */
  template<size_t N>
  MemoryView(const T (&arr)[N])
    : _data(arr), _len(N) {}

  /** @brief Gets the underlying pointer. @return Pointer to the data. */
  const T* data() const { return _data; }

  /** @brief Gets the number of elements. @return The element count. */
  size_t length() const { return _len; }

  /** @brief Checks if the view is empty. @return True if length is 0. */
  bool isEmpty() const { return _len == 0; }

  /** @brief Accesses an element by index. @return Reference to the element. */
  const T& operator[](size_t index) const { return _data[index]; }

  /** @brief Calculates total size in bytes. @return size in bytes (len * sizeof(T)). */
  size_t sizeBytes() const { return _len * sizeof(T); }

  /** @brief Iterator support. @return Pointer to the start of the data. */
  const T* begin() const { return _data; }

  /** @brief Iterator support. @return Pointer to the end of the data. */
  const T* end() const { return _data + _len; }

  /**
   * @brief Creates a sub-view of the current data.
   * @param start The starting index.
   * @param length The number of elements (defaults to end of view).
   * @return A new MemoryView representing the slice.
   */
  MemoryView<T> slice(size_t start, size_t length = 0xFFFFFFFF) const {
    if (start >= _len) return MemoryView<T>();
    size_t avail = _len - start;
    return MemoryView<T>(_data + start, (length > avail) ? avail : length);
  }

  /**
   * @brief Finds the first occurrence of a single value.
   * @return The index of the value, or -1 if not found.
   */
  int indexOf(const T& value, size_t from = 0) const {
    for (size_t i = from; i < _len; i++)
      if (_data[i] == value) return (int)i;
    return -1;
  }

  /**
   * @brief Finds the first occurrence of a pattern (sub-view).
   * @return The starting index of the pattern, or -1 if not found.
   */
  int indexOf(const MemoryView<T>& pattern, size_t from = 0) const {
    if (pattern._len == 0 || pattern._len > (_len - from)) return -1;
    for (size_t i = from; i <= _len - pattern._len; i++) {
      bool match = true;
      for (size_t j = 0; j < pattern._len; j++) {
        if (_data[i + j] != pattern._data[j]) {
          match = false;
          break;
        }
      }
      if (match) return (int)i;
    }
    return -1;
  }

  /** @brief Checks if a value exists in the view. @return True if found. */
  bool contains(const T& value) const { return indexOf(value) != -1; }

  /**
   * @brief Reinterprets the underlying data as a different type.
   * @tparam U The target type for the cast.
   * @return A new MemoryView of type U.
   */
  template<typename U>
  MemoryView<U> castTo() const {
    return MemoryView<U>(reinterpret_cast<const U*>(_data), sizeBytes() / sizeof(U));
  }

protected:
  const T* _data;
  size_t _len;
};

/**
 * @class StringView
 * @brief Specialized MemoryView for zero-copy string manipulation and parsing.
 */
class StringView : public MemoryView<char>, public Printable {
public:
  using MemoryView<char>::MemoryView;

  /** @brief Promotes a char MemoryView to a StringView. @return A StringView. */
  StringView(const MemoryView<char>& base)
    : MemoryView<char>(base.data(), base.length()) {}

  /** @brief Constructs from a C-string. @return A StringView (length calculated via strlen). */
  StringView(const char* str)
    : MemoryView<char>(str, str ? strlen(str) : 0) {}

  /** @brief Constructs from an Arduino String. @return A StringView pointing to the String's buffer. */
  StringView(const String& s)
    : MemoryView<char>(s.c_str(), s.length()) {}

  /** @brief Compares two views for equality. @return True if contents match. */
  bool equals(const StringView& other) const {
    if (_len != other._len) return false;
    return memcmp(_data, other._data, _len) == 0;
  }

  /** @brief Equality operator. @return True if contents match. */
  bool operator==(const StringView& other) const { return equals(other); }

  /** @brief Checks if the view starts with a specific prefix. @return True if match found. */
  bool startsWith(const StringView& prefix) const {
    if (prefix._len > _len) return false;
    return memcmp(_data, prefix._data, prefix._len) == 0;
  }

  /**
   * @brief Removes leading and trailing whitespace.
   * @return A new StringView pointing to the trimmed section.
   */
  StringView trim() const {
    size_t s = 0, e = _len;
    while (s < e && isspace(_data[s])) s++;
    while (e > s && isspace(_data[e - 1])) e--;
    return StringView(slice(s, e - s));
  }

  /**
   * @brief Extracts a token based on a single character delimiter.
   * @param offset Reference to the current position; updated to the next start.
   * @return A StringView of the token found.
   */
  StringView nextToken(char delim, size_t& offset) const {
    if (offset >= _len) return StringView();
    int pos = indexOf(delim, offset);
    if (pos == -1) {
      StringView token = StringView(slice(offset));
      offset = _len;
      return token;
    }
    StringView token = StringView(slice(offset, pos - offset));
    offset = pos + 1;
    return token;
  }

  /**
   * @brief Extracts a token based on a string delimiter.
   * @param offset Reference to the current position; updated to the next start.
   * @return A StringView of the token found.
   */
  StringView nextToken(const StringView& delim, size_t& offset) const {
    if (offset >= _len) return StringView();
    int pos = indexOf(delim, offset);
    if (pos == -1) {
      StringView token = StringView(slice(offset));
      offset = _len;
      return token;
    }
    StringView token = StringView(slice(offset, pos - offset));
    offset = pos + delim.length();
    return token;
  }

  /** @brief Parses the view as a long integer. @return The parsed long value. */
  long toLong() const {
    char buf[_len + 1];
    memcpy(buf, _data, _len);
    buf[_len] = '\0';
    return atol(buf);
  }

  /** @brief Parses the view as a float. @return The parsed float value. */
  float toFloat() const {
    char buf[32];
    size_t copyLen = (_len > 31) ? 31 : _len;
    memcpy(buf, _data, copyLen);
    buf[copyLen] = '\0';
    return atof(buf);
  }

  /** @brief Converts the view to an actual Arduino String. @return A new String object. */
  String toString() const {
    if (_len == 0 || !_data) return String("");
    String s;
    s.reserve(_len + 1);
    for (size_t i = 0; i < _len; i++) s += _data[i];
    return s;
  }

  /** @brief Implements Printable interface for Serial.print(). @return Number of bytes printed. */
  size_t printTo(Print& p) const override {
    return p.write(reinterpret_cast<const uint8_t*>(_data), sizeBytes());
  }
};

#endif
