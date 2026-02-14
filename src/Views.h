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
  MemoryView() : _data(nullptr), _len(0) {}

  /** @brief Constructs a view from a pointer and a specific length. */
  MemoryView(const T* data, size_t len) : _data(data), _len(len) {}

  /** @brief Constructs a view directly from a fixed-size C-style array. */
  template<size_t N>
  MemoryView(const T (&arr)[N]) : _data(arr), _len(N) {}

  /** @brief Gets the underlying pointer. */
  const T* data() const { return _data; }

  /** @brief Gets the number of elements. */
  size_t length() const { return _len; }

  /** @brief Checks if the view is empty. */
  bool isEmpty() const { return _len == 0; }

  /** @brief Accesses an element by index. */
  const T& operator[](size_t index) const { return _data[index]; }

  /** @brief Calculates total size in bytes. */
  size_t sizeBytes() const { return _len * sizeof(T); }

  /** @brief Iterator support (start). */
  const T* begin() const { return _data; }

  /** @brief Iterator support (end). */
  const T* end() const { return _data + _len; }

  /**
   * @brief Creates a sub-view of the current data.
   * @param start Starting index.
   * @param length Number of elements (defaults to remaining length).
   */
  MemoryView<T> slice(size_t start, size_t length = 0xFFFFFFFF) const {
    if (start >= _len) return MemoryView<T>();
    size_t avail = _len - start;
    return MemoryView<T>(_data + start, (length > avail) ? avail : length);
  }

  /** @brief Finds the first occurrence of a value. */
  int indexOf(const T& value, size_t from = 0) const {
    for (size_t i = from; i < _len; i++)
      if (_data[i] == value) return (int) i;
    return -1;
  }

  /** @brief Finds the first occurrence of a pattern (sub-view). */
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
      if (match) return (int) i;
    }
    return -1;
  }

  /** @brief Checks if a value exists. */
  bool contains(const T& value) const { return indexOf(value) != -1; }

  /** @brief Reinterprets data as a different type. */
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

  /** @brief Expose base class search methods to avoid shadowing. */
  using MemoryView<char>::indexOf;
  using MemoryView<char>::contains;

  /** @brief Promote MemoryView<char> to StringView. */
  StringView(const MemoryView<char>& base)
    : MemoryView<char>(base.data(), base.length()) {}

  /** @brief Construct from C-string literal. */
  StringView(const char* str)
    : MemoryView<char>(str, str ? strlen(str) : 0) {}

  /** @brief Construct from Arduino String (points to internal buffer). */
  StringView(const String& s)
    : MemoryView<char>(s.c_str(), s.length()) {}

  // --- Search Overloads ---

  /** @brief Finds position of a C-string pattern. */
  int indexOf(const char* s, size_t from = 0) const {
    return indexOf(StringView(s), from);
  }

  /** @brief Checks if a StringView pattern exists within this view. */
  bool contains(const StringView& pattern) const {
    return indexOf(pattern) != -1;
  }

  /** @brief Checks if a C-string exists within this view. */
  bool contains(const char* s) const {
    return indexOf(StringView(s)) != -1;
  }

  /** @brief Checks if an Arduino String exists within this view. */
  bool contains(const String& s) const {
    return indexOf(StringView(s)) != -1;
  }

  // --- Numeric Conversions ---

  /** @brief Parses view as a long integer. */
  long toLong() const {
    char buf[_len + 1];
    memcpy(buf, _data, _len);
    buf[_len] = '\0';
    return atol(buf);
  }

  /** @brief Parses view as a float. */
  float toFloat() const {
    return (float)toDouble();
  }

  /** @brief Parses view as a double using strtod for precision. */
  double toDouble() const {
    char buf[32];
    size_t copyLen = (_len > 31) ? 31 : _len;
    memcpy(buf, _data, copyLen);
    buf[copyLen] = '\0';
    return strtod(buf, nullptr);
  }

  // --- Utilities & Operators ---

  /** @brief Content equality check. */
  bool equals(const StringView& other) const {
    if (_len != other._len) return false;
    return memcmp(_data, other._data, _len) == 0;
  }

  bool operator==(const StringView& other) const { return equals(other); }

  /** @brief Equality check with C-string. */
  bool operator==(const char* s) const {
    if (!s) return _len == 0;
    size_t sLen = strlen(s);
    if (_len != sLen) return false;
    return memcmp(_data, s, _len) == 0;
  }

  /** @brief Equality check with Arduino String. */
  bool operator==(const String& s) const {
    if (_len != s.length()) return false;
    return memcmp(_data, s.c_str(), _len) == 0;
  }

  /** @brief Global operator for ("literal" == stringView). */
  friend bool operator==(const char* lhs, const StringView& rhs) {
    return rhs == lhs;
  }

  /** @brief Prefix check. */
  bool startsWith(const StringView& prefix) const {
    if (prefix._len > _len) return false;
    return memcmp(_data, prefix._data, prefix._len) == 0;
  }

  /** @brief Returns a new view with whitespace removed from ends. */
  StringView trim() const {
    size_t s = 0, e = _len;
    while (s < e && isspace(_data[s])) s++;
    while (e > s && isspace(_data[e - 1])) e--;
    return StringView(slice(s, e - s));
  }

  /** @brief Tokenize by char delimiter. Updates offset for next call. */
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

  /** @brief Tokenize by string delimiter. Updates offset for next call. */
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

  /** @brief Create a copy as an Arduino String. */
  String toString() const {
    if (_len == 0 || !_data) return String("");
    String s;
    s.reserve(_len + 1);
    for (size_t i = 0; i < _len; i++) s += _data[i];
    return s;
  }

  /** @brief Printable interface implementation. */
  size_t printTo(Print& p) const override {
    return p.write(reinterpret_cast<const uint8_t*>(_data), sizeBytes());
  }
};

#endif
