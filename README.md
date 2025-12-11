# GitHasher (C++26)

A high-performance, asynchronous web server that streams, filters, and hashes GitHub repositories in memory. Built with **C++26**, **Drogon**, and **HTMX**.

## Prerequisites

* **Linux/macOS** (Windows users may need WSL2)
* **C++ Compiler** supporting C++26 (GCC 14+ or Clang 18+)
* **CMake** (3.27 or newer)
* **Git**, **curl**, **zip**, **unzip**, **tar** (required for vcpkg)

## Installation

### 1. Clone the Repository
```bash
git clone [https://github.com/zampini28/githasher.git](https://github.com/zampini28/githasher.git)
cd githasher
````

### 2\. Install Dependencies (Vcpkg)

This project uses `vcpkg` for package management. We will bootstrap it and install libraries locally.

```bash
# Download vcpkg
git clone --depth=1 [https://github.com/microsoft/vcpkg.git](https://github.com/microsoft/vcpkg.git)
./vcpkg/bootstrap-vcpkg.sh

# Install required libraries (Drogon, OpenSSL, LibArchive, etc.)
./vcpkg/vcpkg install
```

## Building

Use CMake to configure and compile the project.

```bash
mkdir build
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)
```

## Running

1.  **Create Logs Directory** (Required for the logger)

    ```bash
    mkdir -p logs
    ```

2.  **Start the Server**

    ```bash
    ./build/GitHasher
    ```

3.  **Access the Application**
    Open your browser and navigate to:
    [http://localhost:8080](https://www.google.com/search?q=http://localhost:8080)

