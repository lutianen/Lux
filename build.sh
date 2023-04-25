# 默认使用 ninja 
# -G "Ninja"
cmake -B build -G "Ninja"

cmake --build build/ --parallel 4
