"""
The goal of this script is to generate empty markdown files with OpenVisus
functions headers for the purpose of documentation and annotation
"""

import ast
import OpenVisus # must have package installed to inspect functions
import os
from inspect import getargspec, getmembers, getsource, isfunction, signature

def filter(func): # filter functions so we document only needed user-facing functions
    func_source = getsource(func)
    has_return = any(isinstance(node, ast.Return) for node in ast.walk(ast.parse(func_source)))
    return has_return

def create_header(func_name, func_args, func_source):
    HEADER = '---\nlayout: default\nparent: Python OpenViSUS Functions\nnav_order: 2\n---\n\n# {}\n\nDescribe function here.\n\n# Function Definition\n\n```python\n{}\n```'
    return HEADER.format(func_name + func_args, func_source)

def main():
    functions_dict = dict([o for o in getmembers(OpenVisus) if isfunction(o[1])])
    for func_name in functions_dict:
        func = functions_dict[func_name]
        if not filter(func): continue
        md_filename = "{}.md".format(func_name)
        #if os.path.exists(md_filename): continue # don't overrite existing files that already are documented
        content=create_header(func_name, str(signature(func)), getsource(func))
        with open(md_filename, 'w') as file:
            file.write(content)
        

if __name__ == '__main__':
    THIS_DIR = os.getcwd()
    TARGET = os.path.join(THIS_DIR, 'docs', 'python-features', 'python-functions')
    os.chdir(TARGET)
    main()