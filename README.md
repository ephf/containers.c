# Containers

- Rust-like containers in C
- Creates a `panic()` and `Error` system with `Option<T>`s and `Result<T>`s
- [lib/containers.c](lib/containers.c)

## Example 1

```c
#include <containers.c>

Result(char*) read_file(char* path) {
    FILE* file = fopent(path, "r"); // line 4
    fseekt(file, 0, SEEK_END);

    long size = ftellt(file);
    rewind(file);

    char* buffer = malloct(size + 1);
    freadt(buffer, 1, size, file);
    buffer[size] = 0;

    fcloset(file);
    return Ok(buffer);
}

int main() {
    char* content = unwrap(read_file("test/text.txt")); // line 19
    puts(content);
}
```

> `test/text.txt` is missing

```
error at read_file:4: No such file or directory
panicked at main() [line 19]
```

> `test/text.txt` has `hello world`

```
hello world
```

## Example 2

```c
#include <containers.c>
#include <assert.h>

Option(char) first_letter(char* string) {
    if(string != NULL && string[0] >= 'a' && string[0] <= 'z') {
        return Some(string[0]);
    }
    return None;
}

int main(int argc, char** argv) {
    assert(argc == 2);

    Option(char) opt_letter = rescope(first_letter(argv[1]));

    if_let(Some(letter), opt_letter) {
        printf("the first letter is %c\n", letter);
    } else {
        printf("the first letter is not a lowercase letter\n");
    }
}
```

> `./a.out hello`

```
the first letter is h
```

> `./a.out 35`

```
the first letter is not a lowercase letter
```

## How it works

Whenever a container is created, it is defined as a dangling pointer that is bounded by the scope it is created in. We can see how this works by expanding a macro that creates a container.

```c
Some(42)
```

This macro creates an `Option(int)`

```c
((Option) (typeof(42)[1]) { 42 }) // expanded
```

Looking at this at first may seem a bit confusing, but expanding the `typeof(42)` and the `Option` type might make it a bit clearer

```c
((void*) (int[1]) { 42 })
```

The main part of this segment `(int[1]) { 42 }` creates an array on the stack of size `1` and sets the first value to `42`. When converted to a value or a pointer, this part of the code will return the address of that heap memory (or a pointer to the value `42`). The `void*` cast is there to keep the compiler from warning you about then casting this type to an `Option(T)`

But why go through all of this trouble when there are simpler ways to create containers? Well here are the reason why two of these *simpler* ways would not work.

1. One way you may think of is using a struct like this

```c
struct Option__int {
    bool is_some;
    int value;
};
```

While that may seem like it works, you then have to think about how you are going to design a macro that does this. At first you could try just creating an unnamed struct everytime you call the `Option(T)` macro, something like this

```c
#define Option(T) struct { bool is_some; T value; }
```

This approach has a major flaw though: C compilers do not like when you try to cast two anonymous struct types to eachother

```c
// in code like this, you will get an error that these two variables
// don't have the same type
Option(int) a;
Option(int) b = a; // error!
```

2. Ok, so why go through the trouble of stack allocating values and leaving dangling pointers? I mean you can just use `malloc` and the pointer will be alive as long as you need.

The main issue I have with `malloc` is that it is slow. Especially for a data type that will be created and destroyed many times during a programs life-cycle, and often only holds a few bytes of data, it seems overkill to allocate memory on the heap. Not only is it slow, but its also anoying to then have to free these types which you will have so many of.

Another concern I have is what if `malloc` fails? Maybe I create another container for `malloc` inside said container, but won't I need `malloc` for that? And `malloc` has already failed, won't it just fail again? Overall, the idea just seems too tedious to deal with.

#### Dangling Pointers...

I know the name sounds scary, and it leaves a lot to be scared about, especially with undefined behaviour, but they have their upsides.

For one, these pointers are to memory on the stack, which is where most of your variables are stored. Compared to the heap allocated pointers from `malloc`, these are a lot faster. The main issue with them is their volatility, which often leads to undefined behaviour.

So how does one get around this? Well lets start with an example

```c
int* create_dangling_pointer() {
    int scoped_variable = 67;
    return &scoped_variable;
}
```

When this function is run, it puts the value `67` on the stack, then returns a pointer to this value, then the memory where that value is stored is "freed", allowing the next called function to use that memory. We can see this undefined behaviour if we call this function, then another, then print out the value

```c
int main() {
    int* dangling_pointer = create_dangling_pointer();
    puts("messing with dangling_pointer");
    printf("dangling_pointer's value: %d\n", *dangling_pointer);
}
```

The program should in theory print out `67`, but this is the result instead

```
messing with dangling_pointer
dangling_pointer's value: 1
```

#### rescope

If you remember, there is a line in [Example 2](#example-2) like this

```c
Option(char) opt_letter = rescope(first_letter(argv[1]));
```

This line uses the `rescope()` macro. What this does is it clones the dangling pointer's value and creates a new pointer attached to the current scope. This is the equivalent (in our previous code) of doing something like this

```c
int main() {
    int new_pointer_value = *create_dangling_pointer();
    int* new_pointer = &new_pointer_value;
}
```

Because the dangling pointer was dereferenced right after the function was called, and before another function could take over that memory on the stack, the value was able to be taken safely (ish) from the dangling pointer.

## Thank you

If you want to know more about the code, read it yourself in [lib/containers.c](lib/containers.c). If you are using this and you encounter any issues, please feel free to create an issue on the [Github repository](https://github.com/ephf/containers.c/issues).