#include <AUnit.h>
#include "Views.h"

// --- MemoryView Tests ---

test(MemoryView, sliceAndLength) {
  int data[] = {10, 20, 30, 40, 50};
  MemoryView<int> view(data, 5);
  
  assertEqual((int)view.length(), 5);
  
  auto sub = view.slice(2, 2); // {30, 40}
  assertEqual((int)sub.length(), 2);
  assertEqual(sub[0], 30);
  assertEqual(sub[1], 40);
}

test(MemoryView, indexOfAndContains) {
  char data[] = {'a', 'b', 'c', 'd'};
  MemoryView<char> view(data, 4);
  
  assertEqual(view.indexOf('c'), 2);
  assertTrue(view.contains('a'));
  assertFalse(view.contains('z'));
}

// --- StringView Tests ---

test(StringView, trimming) {
  StringView s = "  hello world  ";
  StringView trimmed = s.trim();
  
  assertTrue(trimmed.equals("hello world"));
  assertEqual((int)trimmed.length(), 11);
}

test(StringView, tokenization) {
  StringView csv = "val1,val2,val3";
  size_t offset = 0;
  
  StringView t1 = csv.nextToken(',', offset);
  assertTrue(t1.equals("val1"));
  
  StringView t2 = csv.nextToken(',', offset);
  assertTrue(t2.equals("val2"));
  
  StringView t3 = csv.nextToken(',', offset);
  assertTrue(t3.equals("val3"));
  assertEqual(offset, (size_t)14);
}

test(StringView, numericConversion) {
  StringView vLong = "123456";
  StringView vFloat = "3.14";
  
  assertEqual(vLong.toLong(), 123456L);
  assertNear(vFloat.toFloat(), 3.14f, 0.01f);
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial on some boards
}

void loop() {
  aunit::TestRunner::run();
}
