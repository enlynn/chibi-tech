# C++ Style Guide

## Naming Conventions

Types should be PascalCase:
```c++
class YourClassName {};
```

Functions and Variables are camelCase:
```c++
void myFunction() {
    int myVariable = 1;
}
```

Member variables should be prefixed with "m"
```c++
int mMyMemberVariable;  
```

Constexpr variables should be prefixed with "c"
```c++
constexpr int cMyConstant = 10;
```

Paramter variables should be prefixed with "t", short for "the variable"
```c++
void myFunction(int tValue) {}
```

If a paramter is a pointer, then it should be prefixed with "tp", short for "the pointer variable". This is to make it obvious which variables are pointers.
```c++
void myFunction(int* tpValue) {}
```

Macros should be all caps
```c++
#define MY_MACRO
```

## Best Practices

### Const "Correctness"
- If a variable is not mutated, prefer marking it as `const`
- If the variable can be a compile-time constant, prefer `constexpr` over `const`
- If a function does not mutate class state then mark the function as `const`.
- If a function can be determined at compile-time, mark it as `constexpr`.

### Memory Management
Memory Management can be hard, so the goal is to reduce how much of it we have to do. This is primarily done by reducing small allocations and favoring allocation "chunks" like `std::vector` and `std::array`. If data from one of these "chunks" need to be shared, then prefer immutable "handles" (strongly types `uint64_t`) over storing the pointer. Handles are easier to validate when data from the type is queried.

- If a type can be stack allocated, declare it on the stack.
- If there are going to be many of a type, prefer arrays and share via pool handles.
- If there is going to be one type, then use `std::unique_ptr`
- If the type is going to be shared, then use `std::shared_ptr`
- If an array is going to be iterated over with a known size, prefer `std::array`.
- Avoid storing pointers to objects that have don't have a static lifetime
    - A "static" lifetime refers to an object that lives through the duration of the application.
    - An example might be `gpu::Device`, which is initialized with the graphics backend.

### Smart Pointers
- Prefer allocating memory through smart pointers (`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`)
- If passing a smart pointer to a function, prefer using a raw pointer unless the ownership of the pointer is being passed on
```c++
void example() {
    std::unique_ptr<int> someInt = std::make_unique<int>(5);

    MyClass foo{};
    foo.borrow(someInt);
    foo.passOwnership(someInt);
}

void MyClass::passOwnership(std::unique_ptr<int> tpMyPtr) {
    mSomeMember = std::move(tpMyPtr);
}

void MyClass::borrow(const int* tpMyPtr) {
    // do something
}
```
- If the smart pointer is a commonly used type, then alias the type:
```c++
using MyTypeUnique = std::unique_ptr<MyType>;
using MyTypeShared = std::shared_ptr<MyType>;
using MyTypeWeak   = std::weak_ptr<MyType>;
```
- Prefer unique pointers over shared pointers.

### Errors and Exceptions
- Exceptions are disallowed.
- If a function returns a Success/Fail error or a value, prefer to use `std::option`
- If a function returns a general error or a value, prefer to use `std::expected`
    - Assuming I can get that integrated.
- If a function enters a "bad" state that it should not ever be in, prefer `ASSERT` or `ASSERT_CUSTOM`.

### Templates


### RAII
In C++, RAII can be very tricky to work with since there is a lot of implicit and nasty behavior. A lot of this revolves around Copy-Move semantics.

- A Class's Copy Constructor and Operator should be deleted if a copy is non-trivial.
- If a type is being moved and the move is non-trivial, prefer to use `std::move`.
- If a type needs a non-trivial copy, prefer a custom `clone()` function over using the copy constructor/move. This is to make copies explicit and easy to find in code.

### Initialization


### Macros vs constexpr

