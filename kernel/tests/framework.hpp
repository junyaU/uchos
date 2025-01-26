#pragma once

using test_func_t = void (*)();

void test_init();
void test_register(const char* name, test_func_t func);
void test_run();
