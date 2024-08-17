# python3 rename_libfmt.py ~/git/quill/include/quill/bundled/fmt FMTQUILL fmtquill

# Update bundled libfmt 11.0.2:
# 1) run python3 rename_libfmt.py ~/git/quill/include/quill/bundled/fmt FMTQUILL fmtquill
# 2) Add to base.h
#     #if !defined(FMTQUILL_HEADER_ONLY)
#       #define FMTQUILL_HEADER_ONLY
#     #endif
# 3) in base.h remove format.h always included for FMTQUILL_HEADER_ONLY at the end of file
# 4) Change private to protected in fmt::buffer class
# 5) Change append to use memcpy,       if constexpr (std::is_same<T, U>::value) {
#         memcpy(ptr_ + size_, begin, count * sizeof(T));
#       } else

import sys
import os
import re

def rename(file_path, macro_replace, namespace_replace):
    # Read the source file
    with open(file_path, 'r') as file:
        content = file.read()

    # Use regular expressions to find and replace macros and namespace
    macro_pattern = r'\bFMT_'
    namespace_pattern = r'namespace\s+fmt\b'

    updated_content = re.sub(macro_pattern, macro_replace + '_', content)
    updated_content = re.sub(r'fmt::', f'{namespace_replace}::', updated_content)
    updated_content = re.sub(namespace_pattern, f'namespace {namespace_replace}', updated_content)

    # Write the updated content back to the source file
    with open(file_path, 'w') as file:
        file.write(updated_content)

def process_directory(directory, macro_replace, namespace_replace):
    for root, _, files in os.walk(directory):
        for file_name in files:
            if file_name.endswith('.cc') or file_name.endswith('.h'):
                file_path = os.path.join(root, file_name)
                rename(file_path, macro_replace, namespace_replace)
                print(f"Macros, 'fmt::', and 'namespace fmt' replaced successfully in '{file_path}'.")

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: python rename.py <directory> <macro_replace> <namespace_replace>")
    else:
        directory = sys.argv[1]
        macro_replace = sys.argv[2]
        namespace_replace = sys.argv[3]
        process_directory(directory, macro_replace, namespace_replace)