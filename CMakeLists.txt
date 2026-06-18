cmake_minimum_required(VERSION 3.16)
project(TSE CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tse
    src/main.cpp
    src/assets/Stock.cpp
    src/assets/Crypto.cpp
    src/assets/ETF.cpp
    src/accounts/PlayerTrader.cpp
    src/accounts/AdminAccount.cpp
    src/engine/MarketEngine.cpp
    src/ui/Terminal.cpp
    src/ui/Menu.cpp
)

target_include_directories(tse PRIVATE src)
target_compile_options(tse PRIVATE -Wall -Wextra -O2)
