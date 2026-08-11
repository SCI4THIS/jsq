mkdir -p build

for FILE in main.c mmap_file_nix.c
do
  gcc -o build/${FILE}.o -c src/${FILE}
done

gcc -o build/main build/*\.o
