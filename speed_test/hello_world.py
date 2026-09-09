
# The same program as hello_world.satl and hello_world.cpp: 100,000
# iterations of compare, add, assign. It printed "Hello, World!" until
# 2026-09-09, which made the py column a comparison between a loop and a
# print -- the two numbers were never measuring the same thing.

current_number = 0
target_number = 100000

while current_number < target_number:
    current_number = current_number + 1
