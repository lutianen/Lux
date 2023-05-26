#!/bin/bash

# Format
clang-format -i `find app/http/include/http/ -type f -name *.h`
clang-format -i `find app/http/src/ -type f -name *.cc`

clang-format -i `find LuxLog/include/LuxLog/ -type f -name *.h`
clang-format -i `find LuxLog/src/ -type f -name *.cc`
clang-format -i `find LuxLog/test/ -type f -name *.cc`

clang-format -i `find LuxMySQL/include/LuxMySQL/ -type f -name *.h`
clang-format -i `find LuxMySQL/src/ -type f -name *.cc`
clang-format -i `find LuxMySQL/test/ -type f -name *.cc`

clang-format -i `find LuxUtils/include/LuxUtils/ -type f -name *.h`
clang-format -i `find LuxUtils/src/ -type f -name *.cc`
clang-format -i `find LuxUtils/test/ -type f -name *.cc`

clang-format -i `find polaris/include/polaris/ -type f -name *.h`
clang-format -i `find polaris/src/ -type f -name *.cc`
clang-format -i `find polaris/test/ -type f -name *.cc`

# Count
cloc --git `git branch --show-current`
