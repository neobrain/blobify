# blobify

blobify is a header-only C++17 library to handle binary de-/serialization in your project. Given a user-defined C++ struct, blobify can encode its data into a compact binary stream and, conversely, load a structure from its binary encoding. Unlike other serialization frameworks, blobify requires no boilerplate code and instead infers the serialized layout from the structure definition alone. Customizations to the default behavior are enabled by specifying *properties* using an embedded domain specific language.

Common applications of blobify include parsing file formats, network communication, and device drivers.

## What is it?

Consider [bitmap](https://en.wikipedia.org/wiki/BMP_file_format) loading: You'd typically represent bitmap header elements using C++ structs and enums, such as:

```cpp
enum class Compression : uint32_t {
    None      = 0,
    RLE8      = 1,
    RLE4      = 2,
    // ...
};

struct BMPHeader {
  uint16_t signature;

  uint32_t size_bytes;
  uint32_t reserved;
  uint32_t data_offset;

  // (other members omitted for simplicity)

  Compression compression;
};
```

Parsing a `.bmp` file into a such a `BMPHeader` value is as simple as it gets with blobify:

```cpp
std::ifstream file("/path/to/bitmap.bmp");
blob::istream_storage storage { file };

auto header = blob::load<BMPHeader>(storage);
```

The first two lines open a file and prepare it for input to blobify, and the last line performs the actual read. Not only does this provide a convenient interface, but blobify takes care of the subtle details for you too, such as removing compiler-inserted *padding bytes* between struct members.

More elaborate usage examples can be found in the [examples](examples/) directory.

## Customization via properties

Reliable binary deserialization often requires extensive data validation: Your code might expect *version* fields to have a certain value, and `enum`-types should often only have one of the enumerated values. Some data might require postprocessing steps, such as fields encoded in different endianness or different units than the one used at runtime. Blobify allow the user to specify such properties and validation requirements using a built-in DSL.

Consider the `signature` member of `BMPHeader`, which identifies a bitmap file as such. If it's not 0x4d42 (an encoding of the string "BM"), a validation error should be triggered. In blobify this can be implemented by defining a `properties` function:

```cpp
constexpr auto properties(blob::tag<BMPHeader>) {
    blob::properties_t<BMPHeader> props { };

    // Throw exception if the loaded signature is not 0x4d42
    props.member<&BMPHeader::signature>().expected_value = uint16_t { 0x4d42 };

    // Throw exception if the loaded compression value is none of None, RLE4, or RLE8
    props.member<&BMPHeader::compression>().validate_enum = true;

    return props;
}
```

Crucially, the `properties` function must be `constexpr` and must reside in the same namespace as the definition of `BMPHeader`. If no such function is defined, blobify will apply a set of default properties that describe a compact binary encoding in native endianness without any validation.

## Error handling

Coarse error handling can be done by catching blobify's base exception type `blob::exception`:

```cpp
try {
    auto header = blob::load<BMPHeader>(storage);
} catch (blob::exception&) {
    std::cerr << "Failed to load BMPHeader" << std::endl;
}
```

However, especially in data validation you might want to dynamically handle errors depending on the specific error condition. Hence, blobify's [exception hierarchy](include/blobify/exceptions.hpp) allows to distinguish between different error causes (end-of-file/unexpected value/invalid enum/...) as well as the specific data member that triggered the error.

```cpp
try {
    auto header = blob::load<BMPHeader>(storage);
} catch (blob::storage_exhausted_exception&) {
    std::cerr << "Unexpected early end-of-file" << std::endl;
} catch (blob::unexpected_value_exception_for<BMPHeader::signature>&) {
    std::cerr << "Invalid BMP signature" << std::endl;
} catch (blob::invalid_enum_value_exception_for<BMPHeader::compression>&) {
    std::cerr << "Invalid compression value" << std::endl;
}
```

## Usage

Setting up blobify is easiest if your project is built on CMake. Just put a copy of blobify in your project tree (e.g. using a Git submodule) and `add_subdirectory` it from your main CMakeLists.txt. This will register the `blobify` target that you can `target_link_libraries` against.

If your project does not use CMake, setting up blobify is still easy: Just point your compiler to blobify's main include directory as well as the magic_get and magic_enum header paths and you should be good to go.

## Credits

Blobify's API significantly benefits from the marvelous work done by Antony Polukhin and Daniil Goncharov on their respective libraries [PFR](https://github.com/apolukhin/magic_get) (aka magic_get) and [magic_enum](https://github.com/Neargye/magic_enum).
