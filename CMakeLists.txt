cmake_minimum_required(VERSION 3.22)
project(gerenciador_memoria_virtual)

set(CMAKE_CXX_STANDARD 14)

add_executable(gerenciador_memoria_virtual
        src/main.c
        src/web_server.c
        src/main_menu.c
        src/web_server.h
        src/main_menu.h
        src/simulador_page.c
        src/simulador_page.h
        src/nalloc.c
        src/nalloc.h
        src/parser.c
        src/TL.h
)
