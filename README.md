![Build Status](https://github.com/jit89/embedded-view-utils/actions/workflows/compile.yml/badge.svg)
![Latest Release](https://img.shields.io/github/v/release/jit89/embedded-view-utils?color=blue)
[ ![License](https://img.shields.io/badge/license-MIT-green) ](LICENSE)

A zero-copy utility library for Arduino and microcontrollers.

## MemoryView & StringView

A fast, Zero-Copy Data Handling Library for Embedded C++. A lightweight toolkit for safe binary casting and non-destructive tokenization, designed to make parsing strings easier without the overhead of the standard Arduino String class.

## 💡 Introduction
Standard Arduino `String` objects are convenient but dangerous for microcontrollers. They use **Heap Memory**, which leads to **Memory Fragmentation** and random crashes in long-running projects. On the other hand, raw C-pointers (`char*`, `uint8_t*`) are fast but dangerous and are prone to buffer overflows.

The library provides "Views." A View is a small object (a pointer and a length) that sits on the **Stack**. It acts as a window into existing data without ever copying it.

* **Zero Allocation:** No `malloc` or `new` calls.
* **Non-Destructive:** Views never modify the original source.
* **Type Safe:** Prevents treating a float array as an integer array by accident.
* **Universal:** Works on any microcontroller supporting C++ templates (AVR, ESP32, STM32, ARM).

## 🏗️ Design
The library uses a two-tier hierarchy to separate raw memory logic from human-readable text logic.

1.  **`MemoryView<T>`**: The foundation. Handles pointer arithmetic, slicing, and generic searching for any data type (`int`, `struct`, `float`, etc.).
2.  **`StringView`**: Inherits from `MemoryView<char>`. Adds text-specific logic like `trim()`, `startsWith()`, and `nextToken()`.

## 🚀 Quick Start

### 1. Parsing Strings ("Scanner" Approach)
Instead of destructive `strtok`, use `nextToken` to slide through a string.

```cpp
StringView csv = "DATA:22.5:OK";
size_t offset = 0;

StringView header = csv.nextToken(':', offset); // "DATA"
StringView value  = csv.nextToken(':', offset); // "22.5"
float val = value.toFloat();
```

### 2. Interpreting Binary Data (Casting Bytes)

```cpp
struct __attribute__((packed)) Telemetry {
    uint32_t ts;
    float val;
};

uint8_t buffer[8] = { ... };
MemoryView<uint8_t> bytes(buffer, 8);

// Cast bytes to a Telemetry struct view
auto teleView = bytes.castTo<Telemetry>(); 
Serial.println(teleView[0].val);
```

## 📜 Method Cheatsheet

`MemoryView<T>` (Base Class)

| Method | Return Type | Description |
| -- | -- | -- |
| `data()` | `const T*` | Returns the raw pointer to the start of the data. |
| `length()`	| `size_t` |	Returns the number of elements (T) in the view. |
| `isEmpty()` |	`bool` | Returns true if the length is 0. |
| `sizeBytes()` |	`size_t` |	Returns the total memory size in bytes. |
| `slice(start, len)` |	`MemoryView<T>` |	Creates a sub-window of the current data without copying. |
| `indexOf(val, from)` | `int` | Finds the first index of a value; returns -1 if not found. |
| `contains(val)`	| `bool` | Convenience method to check if a value exists in the view. |
| `castTo<U>()` | `MemoryView<U>` | Reinterprets the underlying memory as a different type (e.g., bytes to struct). |
|`begin() / end()` | `const T*` | Standard iterators to support for (auto& i : view) loops. |

`StringView` (Inherits from `MemoryView<char>`)

| Method | Return Type | Description |
| -- | -- | -- |
|`equals(other)` | `bool` |	Performs a memory-safe comparison with another StringView. |
|`operator==`	| `bool` | Shorthand for the equals() method. |
|`startsWith(pre)` | `bool` | Checks if the view begins with the specified prefix. |
| `trim()` | `StringView` | Returns a new view with leading/trailing whitespace removed. |
|`nextToken(delim, offset)` | `StringView` | Makes parsing strings easier by extracting segments and updating the `offset`.|
|`toLong()` | `long` | Converts text to an integer using a temporary stack buffer.|
|`toFloat()` | `float` | Converts text to a float using a temporary stack buffer.|
|`toString()` | `String` | Creates an Arduino String object. **Note: Uses heap memory**. |

## ⚠️ Safety

1. Lifetime: A View is a "window." If the original data (like a local array in a function) is destroyed, the View becomes invalid. Never return a View that points to a local function variable.

2. Null-Termination: StringView does not guarantee a null terminator at the end of its data() pointer. Always use toLong(), toFloat(), or toString() if you need to pass data to a function requiring a C-string (const char*).

3. Alignment: When using castTo<T>, ensure your source buffer is aligned correctly for the target type (e.g., 4-byte alignment for float on 32-bit systems).

## 🛠️ Portability

This library is header-only and relies on standard C++11 templates. It can be compiled by any modern toolchain:

- Arduino IDE (GCC)
- PlatformIO
- ESP-IDF / STM32Cube

