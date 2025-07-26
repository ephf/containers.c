# Containers

- Rust-like containers in C
- Creates a `panic()` and `Error` system with `Option<T>`s and `Result<T>`s

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